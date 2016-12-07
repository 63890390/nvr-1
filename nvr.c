#include "nvr.h"

int record(Camera *camera, Settings *settings) {
    AVFormatContext *icontext = NULL;
    AVDictionary *ioptions = NULL;
    AVStream *istream = NULL;
    AVStream *ostream = NULL;
    AVFormatContext *ocontext = NULL;
    AVPacket packet;
    int ret, i;
    int ivideo_stream_index = -1;
    int64_t o_start_dts = AV_NOPTS_VALUE;
    int64_t o_last_dts = AV_NOPTS_VALUE;
    int64_t i_last_dts = 0;
    char got_keyframe = 0, header_written = 0;
    char current_date[10], current_time[8], filename[256], dirname[256];
    time_t raw_time;
    struct tm *time_info;

    av_dict_set(&ioptions, "rtsp_transport", "tcp", 0);

    av_init_packet(&packet);
    icontext = avformat_alloc_context();

    printf("[%s] connecting\n", camera->name);
    if ((ret = avformat_open_input(&icontext, camera->uri, NULL, &ioptions)) != 0) {
        printf("[%s] avformat_open_input failed with code %d\n", camera->name, ret);
        return -1;
    }
    if ((ret = avformat_find_stream_info(icontext, NULL)) != 0) {
        printf("[%s] avformat_find_stream_info failed with code %d\n", camera->name, ret);
        return -1;
    }
    for (i = 0; i < icontext->nb_streams; i++)
        if (icontext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            ivideo_stream_index = i;
            istream = icontext->streams[ivideo_stream_index];
            break;
        }
    if (ivideo_stream_index < 0) {
        printf("[%s] video stream not found\n", camera->name);
        return -1;
    }

    while (av_read_frame(icontext, &packet) >= 0 && settings->running) {
        if (packet.stream_index == ivideo_stream_index) {
            if (packet.flags & AV_PKT_FLAG_KEY && packet.pts != AV_NOPTS_VALUE && packet.duration >= 0) {
                got_keyframe = 1;
                if (!header_written || o_last_dts >= settings->segment_length * 100000) {
                    if (header_written) {
                        av_write_trailer(ocontext);
                        avio_close(ocontext->pb);
                    }
                    header_written = 0;
                    avformat_free_context(ocontext);
                    avformat_alloc_output_context2(&ocontext, NULL, "mp4", NULL);
                    AVCodecContext *codec_context = avcodec_alloc_context3(NULL);
                    avcodec_parameters_to_context(codec_context, istream->codecpar);
                    ostream = avformat_new_stream(ocontext, codec_context->codec);
                    avcodec_parameters_from_context(ostream->codecpar, codec_context);
                    time(&raw_time);
                    time_info = localtime(&raw_time);
                    strftime(current_date, 10, "%Y%m%d", time_info);
                    strftime(current_time, 8, "%H%M%S", time_info);
                    sprintf(dirname, "%s/%s/%s", settings->storage_dir, current_date, camera->name);
                    sprintf(filename, "%s/%s.mp4", dirname, current_time);

                    if ((ret = mkdirs(dirname, 0755)) != 0) {
                        printf("[%s] failed to create directory \"%s\" with code %d\n", camera->name, dirname, ret);
                        break;
                    }
                    if ((ret = avio_open2(&ocontext->pb, filename, AVIO_FLAG_WRITE, NULL, NULL)) != 0) {
                        printf("[%s] failed to open \"%s\" with code %d\n", camera->name, filename, ret);
                        break;
                    }
                    printf("[%s] recording to \"%s\"\n", camera->name, filename);
                    if ((ret = avformat_write_header(ocontext, NULL)) != 0) {
                        printf("[%s] failed to write header to \"%s\" with code %d\n", camera->name, filename, ret);
                        av_write_trailer(ocontext);
                        avio_close(ocontext->pb);
                        break;
                    }
                    header_written = 1;
                    o_start_dts = av_rescale_q(
                            packet.dts,
                            istream->time_base,
                            ostream->time_base
                    );
                }
            }

            if (got_keyframe) {
                packet.stream_index = istream->index;

                if (packet.dts != AV_NOPTS_VALUE && packet.pts != AV_NOPTS_VALUE && packet.dts > packet.pts) {
                    packet.pts = packet.dts = packet.pts + packet.dts + i_last_dts + 1
                                              - MIN3(packet.pts, packet.dts, i_last_dts + 1)
                                              - MAX3(packet.pts, packet.dts, i_last_dts + 1);
                }
                if (packet.dts != AV_NOPTS_VALUE && i_last_dts != AV_NOPTS_VALUE) {
                    int64_t max = i_last_dts + 1;
                    if (packet.dts < max) {
                        if (packet.pts >= packet.dts)
                            packet.pts = MAX(packet.pts, max);
                        packet.dts = max;
                    }
                }
                i_last_dts = packet.dts;

                packet.pts = av_rescale_q(
                        packet.pts,
                        istream->time_base,
                        ostream->time_base
                ) - o_start_dts;
                packet.dts = av_rescale_q(
                        packet.dts,
                        istream->time_base,
                        ostream->time_base
                ) - o_start_dts;
                o_last_dts = packet.dts;

                if ((ret = av_interleaved_write_frame(ocontext, &packet)) != 0) {
                    printf("[%s] failed to write frame to \"%s\" with code %d\n", camera->name, filename, ret);
                    av_packet_unref(&packet);
                    break;
                }
            }
        }
        av_packet_unref(&packet);
        av_init_packet(&packet);
    }

    av_packet_unref(&packet);
    if (header_written) {
        av_write_trailer(ocontext);
        avio_close(ocontext->pb);
    }

    avformat_free_context(ocontext);
    avformat_free_context(icontext);

    printf("[%s] finished\n", camera->name);
    return 0;
}
