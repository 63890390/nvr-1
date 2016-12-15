#include "nvr.h"

#include <pthread.h>
#include <libavformat/avformat.h>
#include "utils.h"
#include "log.h"

void ffmpeg_init() {
    av_log_set_callback(nvr_log_ffmpeg);
    av_register_all();
    avcodec_register_all();
    avformat_network_init();
}

void ffmpeg_deinit() {
    avformat_network_deinit();
}

static int decode_interrupt_cb(void *arg) {
    NVRThreadParams *p = (NVRThreadParams *) arg;
    int check_conn_timeout = !p->connected && p->conn_timeout;
    int check_recv_timeout = p->connected && p->recv_timeout;

    if (*p->running == 0)
        return 1;

    if (check_conn_timeout || check_recv_timeout) {
        struct timeval t2;
        long delta_ms;
        gettimeofday(&t2, NULL);
        if (check_conn_timeout) {
            delta_ms = (t2.tv_sec - p->start_time.tv_sec) * 1000;
            delta_ms += (t2.tv_usec - p->start_time.tv_usec) / 1000;
            if (delta_ms >= *p->conn_timeout) {
                if (!p->failed) {
                    nvr_log(NVR_LOG_ERROR, "connection timeout");
                    p->failed = 1;
                }
                return 1;
            }
        } else /* if (check_recv_timeout) */ {
            delta_ms = (t2.tv_sec - p->last_packet_time.tv_sec) * 1000;
            delta_ms += (t2.tv_usec - p->last_packet_time.tv_usec) / 1000;
            if (delta_ms >= *p->recv_timeout) {
                if (!p->failed) {
                    nvr_log(NVR_LOG_ERROR, "packet timeout");
                    p->failed = 1;
                }
                return 1;
            }
        }
    }
    return 0;
}

int record(Camera *camera, Settings *settings, NVRThreadParams *params) {
    AVFormatContext *icontext = NULL;
    AVDictionary *ioptions = NULL;
    AVStream *istream = NULL;
    AVStream *ostream = NULL;
    AVFormatContext *ocontext = NULL;
    AVCodecContext *codec_context = NULL;
    AVPacket packet;
    int ret;
    int ivideo_stream_index = -1;
    int64_t o_start_dts = AV_NOPTS_VALUE;
    int64_t o_last_dts = AV_NOPTS_VALUE;
    int64_t i_last_dts = 0;
    int got_keyframe = 0, header_written = 0;
    char current_date[9], current_time[7], dirname[1024], filename[sizeof(dirname) + 64];
    time_t raw_time;
    struct tm *time_info;

    params->conn_timeout = &settings->conn_timeout;
    params->recv_timeout = &settings->recv_timeout;
    params->running = &camera->running;
    params->connected = 0;
    params->failed = 0;
    gettimeofday(&params->start_time, NULL);

    pthread_setspecific(0, camera->name);

    av_dict_set(&ioptions, "rtsp_transport", "tcp", 0);

    av_init_packet(&packet);
    icontext = avformat_alloc_context();
    icontext->interrupt_callback.callback = decode_interrupt_cb;
    icontext->interrupt_callback.opaque = params;
    icontext->protocol_whitelist = av_strdup("rtsp,rtp,tcp,udp");

    nvr_log(NVR_LOG_DEBUG, "connecting");
    ret = avformat_open_input(&icontext, camera->uri, NULL, &ioptions);
    if (ret != 0) {
        avformat_close_input(&icontext);
        av_dict_free(&ioptions);
        if (camera->running)
            nvr_log(NVR_LOG_ERROR, "avformat_open_input failed with code %d", ret);
        else
            nvr_log(NVR_LOG_INFO, "stopped");
        return -1;
    }
    nvr_log(NVR_LOG_DEBUG, "retrieving stream info");
    if ((ret = avformat_find_stream_info(icontext, NULL)) != 0) {
        avformat_close_input(&icontext);
        av_dict_free(&ioptions);
        if (camera->running)
            nvr_log(NVR_LOG_ERROR, "avformat_find_stream_info failed with code %d", ret);
        else
            nvr_log(NVR_LOG_INFO, "stopped");
        return -1;
    }
    for (unsigned int i = 0; i < icontext->nb_streams; i++)
        if (icontext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            ivideo_stream_index = i;
            istream = icontext->streams[ivideo_stream_index];
            break;
        }
    if (ivideo_stream_index < 0) {
        avformat_close_input(&icontext);
        av_dict_free(&ioptions);
        nvr_log(NVR_LOG_ERROR, "video stream not found");
        return -1;
    }

    if (params->failed) {
        avformat_close_input(&icontext);
        av_dict_free(&ioptions);
        nvr_log(NVR_LOG_ERROR, "failed");
        return -1;
    }

    if (camera->running && !params->failed) {
        params->connected = 1;
        gettimeofday(&params->last_packet_time, NULL);
        nvr_log(NVR_LOG_INFO, "connected");
    }

    while (av_read_frame(icontext, &packet) >= 0 && camera->running && !params->failed) {
        gettimeofday(&params->last_packet_time, NULL);
        if (packet.stream_index == ivideo_stream_index) {
            if (packet.flags & AV_PKT_FLAG_KEY && packet.pts != AV_NOPTS_VALUE && packet.duration >= 0) {
                got_keyframe = 1;
                if (!header_written ||
                    (settings->segment_length > 0 && o_last_dts >= settings->segment_length * 100)) {
                    if (header_written) {
                        av_write_trailer(ocontext);
                        avio_close(ocontext->pb);
                    }
                    header_written = 0;
                    avformat_free_context(ocontext);
                    avformat_alloc_output_context2(&ocontext, NULL, "mp4", NULL);
                    codec_context = avcodec_alloc_context3(NULL);
                    avcodec_parameters_to_context(codec_context, istream->codecpar);
                    ostream = avformat_new_stream(ocontext, codec_context->codec);
                    avcodec_parameters_from_context(ostream->codecpar, codec_context);
                    avcodec_free_context(&codec_context);
                    time(&raw_time);
                    time_info = localtime(&raw_time);
                    strftime(current_date, sizeof(current_date), "%Y%m%d", time_info);
                    strftime(current_time, sizeof(current_time), "%H%M%S", time_info);
                    snprintf(dirname, sizeof(dirname), "%s/%s/%s", settings->storage_dir, current_date, camera->name);
                    snprintf(filename, sizeof(filename), "%s/%s.mp4", dirname, current_time);

                    if ((ret = mkdirs(dirname, 0755)) != 0) {
                        nvr_log(NVR_LOG_ERROR,
                                "failed to create directory \"%s\" with code %d", dirname, ret);
                        break;
                    }
                    if ((ret = avio_open2(&ocontext->pb, filename, AVIO_FLAG_WRITE, NULL, NULL)) != 0) {
                        nvr_log(NVR_LOG_ERROR,
                                "failed to open \"%s\" with code %d", filename, ret);
                        break;
                    }
                    nvr_log(NVR_LOG_DEBUG, "writing to \"%s\"", filename);
                    if ((ret = avformat_write_header(ocontext, NULL)) != 0) {
                        nvr_log(NVR_LOG_ERROR,
                                "failed to write header to \"%s\" with code %d", filename, ret);
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
                    nvr_log(NVR_LOG_ERROR, "failed to write frame to \"%s\" with code %d", filename, ret);
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

    av_dict_free(&ioptions);

    avformat_close_input(&icontext);
    avformat_free_context(ocontext);

    params->connected = 0;

    if (camera->running)
        nvr_log(NVR_LOG_ERROR, "failed");
    else
        nvr_log(NVR_LOG_INFO, "stopped");

    return 0;
}
