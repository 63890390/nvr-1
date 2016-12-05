#include "nvr.h"

int record(Camera *camera, Settings *settings) {
    AVFormatContext *icontext = NULL;
    AVDictionary *ioptions = NULL;
    AVStream *istream = NULL;
    AVFormatContext *ocontext = NULL;
    AVPacket packet;
    int ret, video_index = -1;
    char got_init = 0, got_keyframe = 0, header_written = 0;
    int64_t last_mux_dts = AV_NOPTS_VALUE;
    char current_date[10], current_time[8], filename[256], dirname[256];
    double cur_time = 0.0, last_time = 0.0;
    time_t raw_time;
    struct tm *time_info;

    av_dict_set(&ioptions, "rtsp_transport", "tcp", 0);
    av_init_packet(&packet);

    printf("%s connecting\n", camera->name);
    icontext = avformat_alloc_context();
    if ((ret = avformat_open_input(&icontext, camera->uri, NULL, &ioptions))) {
        printf("%s avformat_open_input failed with code %d\n", camera->name, ret);
        return -1;
    }
    if ((ret = avformat_find_stream_info(icontext, NULL))) {
        printf("%s avformat_find_stream_info failed with code %d\n", camera->name, ret);
        return -1;
    }
    for (int i = 0; i < icontext->nb_streams; i++)
        if (icontext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    if (video_index < 0) {
        printf("%s video stream not found\n", camera->name);
        return -1;
    }

    avformat_alloc_output_context2(&ocontext, NULL, "mp4", NULL);

    av_read_play(icontext);
    while (av_read_frame(icontext, &packet) >= 0 && settings->running) {
        if (packet.stream_index != video_index)
            continue;

        if (packet.pts == 0 && packet.dts == 0) {
            got_init = 1;
            if (istream == NULL) {
                AVCodecContext *codec_context = avcodec_alloc_context3(NULL);
                avcodec_parameters_to_context(codec_context, icontext->streams[video_index]->codecpar);
                istream = avformat_new_stream(ocontext, codec_context->codec);
                avcodec_parameters_from_context(istream->codecpar, codec_context);
            }
        }

        if (got_init && packet.flags & AV_PKT_FLAG_KEY) {
            got_keyframe = 1;
            if (!header_written || cur_time - last_time >= settings->segment_length) {
                if (header_written) {
                    av_write_trailer(ocontext);
                    avio_close(ocontext->pb);
                    header_written = 0;
                }
                last_time = cur_time;
                time(&raw_time);
                time_info = localtime(&raw_time);
                strftime(current_date, 10, "%Y%m%d", time_info);
                strftime(current_time, 8, "%H%M%S", time_info);
                sprintf(dirname, "%s/%s/%s", settings->storage_dir, current_date, camera->name);
                sprintf(filename, "%s/%s.mp4", dirname, current_time);
                if ((ret = mkdirs(dirname, 0755)) != 0) {
                    printf("%s failed to create directory \"%s\" with code %d\n", camera->name, dirname, ret);
                    break;
                }
                if ((ret = avio_open2(&ocontext->pb, filename, AVIO_FLAG_WRITE, NULL, NULL)) != 0) {
                    printf("%s failed to open \"%s\" with code %d\n", camera->name, filename, ret);
                    break;
                }
                printf("%s recording to \"%s\"\n", camera->name, filename);
                if ((ret = avformat_write_header(ocontext, NULL)) != 0) {
                    printf("%s failed to write header to \"%s\" with code %d\n", camera->name, filename, ret);
                    break;
                }
                header_written = 1;
            }
        }

        if (!got_keyframe)
            continue;

        if (packet.dts != AV_NOPTS_VALUE && packet.pts != AV_NOPTS_VALUE && packet.dts > packet.pts) {
            packet.pts = packet.dts = packet.pts + packet.dts + last_mux_dts + 1
                                      - MIN3(packet.pts, packet.dts, last_mux_dts + 1)
                                      - MAX3(packet.pts, packet.dts, last_mux_dts + 1);
        }
        if (packet.dts != AV_NOPTS_VALUE && last_mux_dts != AV_NOPTS_VALUE) {
            int64_t max = last_mux_dts + 1;
            if (packet.dts < max) {
                if (packet.pts >= packet.dts)
                    packet.pts = MAX(packet.pts, max);
                packet.dts = max;
            }
        }
        last_mux_dts = packet.dts;

        packet.stream_index = istream->index;

        packet.pts = av_rescale_q(
                packet.pts,
                icontext->streams[video_index]->time_base,
                ocontext->streams[0]->time_base
        );
        packet.dts = av_rescale_q(
                packet.dts,
                icontext->streams[video_index]->time_base,
                ocontext->streams[0]->time_base
        );

        cur_time = av_stream_get_end_pts(ocontext->streams[0]) * av_q2d(ocontext->streams[0]->time_base);
        if ((ret = av_write_frame(ocontext, &packet)) != 0) {
            printf("%s failed to write frame to \"%s\" with code %d\n", camera->name, filename, ret);
            break;
        }
        av_packet_unref(&packet);
        av_init_packet(&packet);
    }
    av_packet_unref(&packet);
    if (header_written)
        av_write_trailer(ocontext);
    avio_close(ocontext->pb);
    avformat_free_context(ocontext);
    avformat_free_context(icontext);
    printf("%s finished\n", camera->name);

    return 0;
}
