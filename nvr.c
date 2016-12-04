#include "nvr.h"

int record(Camera *camera, Settings *settings) {
    AVFormatContext *input_context = NULL;
    AVDictionary *input_options = NULL;
    AVStream *input_stream = NULL;
    AVFormatContext *output_context = NULL;
    AVPacket packet;
    int ret, video_index = -1, got_keyframe = 0;
    long last_pts = 0;
    time_t raw_time;
    struct tm *time_info;
    char current_date[10], current_time[8], filename[256], dirname[256];
    double cur_time = 0.0, last_time = 0.0;

    av_dict_set(&input_options, "rtsp_transport", "tcp", 0);
    av_init_packet(&packet);

    printf("%s connecting\n", camera->name);
    input_context = avformat_alloc_context();
    if ((ret = avformat_open_input(&input_context, camera->uri, NULL, &input_options)))
        return ret;
    if ((ret = avformat_find_stream_info(input_context, NULL)))
        return ret;
    for (int i = 0; i < input_context->nb_streams; i++)
        if (input_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    if (video_index < 0) {
        printf("%s video stream not found\n", camera->name);
        return -1;
    }

    avformat_alloc_output_context2(&output_context, NULL, "mp4", NULL);

    av_read_play(input_context);
    while (av_read_frame(input_context, &packet) >= 0 && settings->running) {
        if (packet.stream_index == video_index) {
            if (input_stream == NULL) {
                AVCodecContext *codec_context = avcodec_alloc_context3(NULL);
                avcodec_parameters_to_context(codec_context, input_context->streams[video_index]->codecpar);
                input_stream = avformat_new_stream(output_context, codec_context->codec);
                avcodec_parameters_from_context(input_stream->codecpar, codec_context);
            }
            if (packet.flags & AV_PKT_FLAG_KEY & ~AV_PKT_FLAG_CORRUPT && packet.pts >= 0) {
                if (!got_keyframe || cur_time - last_time >= settings->segment_length) {
                    if (got_keyframe) {
                        av_write_trailer(output_context);
                        avio_close(output_context->pb);
                    }
                    last_time = cur_time;

                    time(&raw_time);
                    time_info = localtime(&raw_time);
                    strftime(current_date, 10, "%Y%m%d", time_info);
                    strftime(current_time, 8, "%H%M%S", time_info);
                    sprintf(dirname, "%s/%s/%s", settings->storage_dir, current_date, camera->name);
                    if (mkdirs(dirname, 0755) != 0) {
                        printf("failed to create directory: %s\n", dirname);
                        return -1;
                    }
                    sprintf(filename, "%s/%s.mp4", dirname, current_time);
                    printf("%s recording to %s\n", camera->name, filename);
                    avio_open2(&output_context->pb, filename, AVIO_FLAG_WRITE, NULL, NULL);

                    if (avformat_write_header(output_context, NULL) != 0)
                        printf("%s unable to write header to %s\n", camera->name, filename);
                }
                got_keyframe = 1;
            }
            if (!got_keyframe)
                continue;

            packet.stream_index = input_stream->index;

            packet.pts = av_rescale_q(
                    packet.pts,
                    input_context->streams[video_index]->time_base,
                    output_context->streams[0]->time_base
            );
            packet.dts = av_rescale_q(
                    packet.dts,
                    input_context->streams[video_index]->time_base,
                    output_context->streams[0]->time_base
            );

            if (packet.pts > last_pts) {
                cur_time = av_stream_get_end_pts(output_context->streams[0]) *
                           av_q2d(output_context->streams[0]->time_base);
                av_write_frame(output_context, &packet);
                last_pts = packet.pts;
            }
        }
        av_packet_unref(&packet);
        av_init_packet(&packet);
    }
    av_write_trailer(output_context);
    avio_close(output_context->pb);
    avformat_free_context(output_context);
    avformat_free_context(input_context);
    printf("%s done\n", camera->name);

    return 0;
}
