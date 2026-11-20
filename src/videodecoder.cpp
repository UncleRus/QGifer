/************************************************************************
** QGifer
** Copyright (C) 2013 Lukasz Chodyla <chodak166@op.pl>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
************************************************************************/

#include "videodecoder.h"

#include <QDebug>
#include <QFile>

#include <algorithm>
#include <cmath>

namespace {

QString avErrorString(int error)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(error, buffer, sizeof(buffer));
    return QString::fromUtf8(buffer);
}

double rationalToDouble(AVRational value)
{
    return value.den != 0 ? static_cast<double>(value.num) / value.den : 0.0;
}

} // namespace

VideoDecoder::VideoDecoder() = default;

VideoDecoder::~VideoDecoder()
{
    close();
}

bool VideoDecoder::open(const QString &path)
{
    close();

    const QByteArray nativePath = QFile::encodeName(path);
    int ret = avformat_open_input(&formatCtx, nativePath.constData(), nullptr, nullptr);
    if (ret < 0) {
        qWarning("VideoDecoder: cannot open %s: %s", nativePath.constData(),
            qPrintable(avErrorString(ret)));
        reset();
        return false;
    }

    ret = avformat_find_stream_info(formatCtx, nullptr);
    if (ret < 0) {
        qWarning("VideoDecoder: cannot read stream info: %s", qPrintable(avErrorString(ret)));
        close();
        return false;
    }

    videoStream = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStream < 0) {
        qWarning("VideoDecoder: no video stream in %s", nativePath.constData());
        close();
        return false;
    }

    AVStream *stream = formatCtx->streams[videoStream];
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        qWarning(
            "VideoDecoder: unsupported codec %s", avcodec_get_name(stream->codecpar->codec_id));
        close();
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        close();
        return false;
    }

    ret = avcodec_parameters_to_context(codecCtx, stream->codecpar);
    if (ret < 0) {
        qWarning("VideoDecoder: cannot copy codec parameters: %s", qPrintable(avErrorString(ret)));
        close();
        return false;
    }

    ret = avcodec_open2(codecCtx, codec, nullptr);
    if (ret < 0) {
        qWarning("VideoDecoder: cannot open codec: %s", qPrintable(avErrorString(ret)));
        close();
        return false;
    }

    decodedFrame = av_frame_alloc();
    packet = av_packet_alloc();
    if (!decodedFrame || !packet) {
        close();
        return false;
    }

    frameSize = QSize(stream->codecpar->width, stream->codecpar->height);

    AVRational rate = stream->avg_frame_rate;
    if (rate.num <= 0 || rate.den <= 0 || rationalToDouble(rate) <= 0.0)
        rate = stream->r_frame_rate;
    if (rate.num <= 0 || rate.den <= 0 || rationalToDouble(rate) <= 0.0)
        rate = AVRational{25, 1};
    frameRate = rationalToDouble(rate);

    if (stream->nb_frames > 0) {
        totalFrames = stream->nb_frames;
    } else if (stream->duration > 0) {
        totalFrames = static_cast<long>(
            std::llround(rationalToDouble(stream->time_base) * stream->duration * frameRate));
    } else if (formatCtx->duration > 0) {
        totalFrames = static_cast<long>(
            std::llround(static_cast<double>(formatCtx->duration) / AV_TIME_BASE * frameRate));
    }

    lastIndex = -1;
    nextIndex = 0;
    hasPending = false;
    pendingIndex = -1;
    eof = false;

    return true;
}

void VideoDecoder::close()
{
    if (swsCtx) {
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }
    swsWidth = 0;
    swsHeight = 0;
    swsFormat = -1;
    if (decodedFrame) {
        av_frame_free(&decodedFrame);
    }
    if (packet) {
        av_packet_free(&packet);
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
    }
    if (formatCtx) {
        avformat_close_input(&formatCtx);
    }
    reset();
}

void VideoDecoder::reset()
{
    videoStream = -1;
    frameSize = QSize();
    frameRate = 0.0;
    totalFrames = 0;
    lastIndex = -1;
    nextIndex = -1;
    pendingFrame = QImage();
    pendingIndex = -1;
    hasPending = false;
    eof = false;
}

QString VideoDecoder::codecName() const
{
    if (!codecCtx)
        return QString();

    switch (codecCtx->codec_id) {
    case AV_CODEC_ID_MJPEG:
        return QStringLiteral("MJPG");
    case AV_CODEC_ID_RAWVIDEO:
        return QStringLiteral("rawvideo");
    default:
        return QString::fromLatin1(avcodec_get_name(codecCtx->codec_id));
    }
}

long VideoDecoder::frameIndex(const AVFrame *frame) const
{
    if (!formatCtx || videoStream < 0 || !frame)
        return -1;

    const AVStream *stream = formatCtx->streams[videoStream];
    if (frame->pts == AV_NOPTS_VALUE)
        return -1;

    const int64_t start = stream->start_time != AV_NOPTS_VALUE ? stream->start_time : 0;
    const double seconds = (frame->pts - start) * rationalToDouble(stream->time_base);
    return static_cast<long>(std::llround(seconds * frameRate));
}

bool VideoDecoder::convertFrame(const AVFrame *src, QImage &image)
{
    if (!src || src->width <= 0 || src->height <= 0)
        return false;

    if (!swsCtx || swsWidth != src->width || swsHeight != src->height || swsFormat != src->format) {
        if (swsCtx)
            sws_freeContext(swsCtx);
        swsCtx = sws_getContext(src->width, src->height, static_cast<AVPixelFormat>(src->format),
            src->width, src->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsCtx) {
            qWarning("VideoDecoder: cannot create scaling context");
            return false;
        }
        swsWidth = src->width;
        swsHeight = src->height;
        swsFormat = src->format;
    }

    const int width = src->width;
    const int height = src->height;
    // swscale may write a few bytes past the last pixel of a row (SIMD
    // stores are wider than one pixel), so the buffer needs aligned strides
    // and some slack at the end. A tightly packed width * 3 * height buffer
    // is not enough and results in heap corruption.
    const int align = 32;
    const int bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, align);
    if (bytes <= 0)
        return false;

    uint8_t *buffer = static_cast<uint8_t *>(av_malloc(bytes + AV_INPUT_BUFFER_PADDING_SIZE));
    if (!buffer)
        return false;

    uint8_t *dstData[4] = {nullptr, nullptr, nullptr, nullptr};
    int dstLinesize[4] = {0, 0, 0, 0};
    if (av_image_fill_arrays(dstData, dstLinesize, buffer, AV_PIX_FMT_RGB24, width, height, align)
        < 0) {
        av_free(buffer);
        return false;
    }

    sws_scale(swsCtx, src->data, src->linesize, 0, height, dstData, dstLinesize);

    image = QImage(dstData[0], width, height, dstLinesize[0], QImage::Format_RGB888).copy();
    av_free(buffer);
    return !image.isNull();
}

bool VideoDecoder::readDecodedFrame(QImage &image)
{
    if (!isOpened() || !decodedFrame || !packet)
        return false;

    for (;;) {
        int ret = avcodec_receive_frame(codecCtx, decodedFrame);
        if (ret == 0) {
            // Frame ordinals are counted sequentially; pts is only used to
            // re-synchronise the counter after a seek/flush.
            if (lastIndex < 0) {
                const long index = frameIndex(decodedFrame);
                lastIndex = index >= 0 ? index : 0;
            } else {
                ++lastIndex;
            }
            nextIndex = lastIndex + 1;
            const bool ok = convertFrame(decodedFrame, image);
            av_frame_unref(decodedFrame);
            return ok;
        }
        if (ret == AVERROR_EOF) {
            eof = true;
            return false;
        }
        if (ret != AVERROR(EAGAIN)) {
            qWarning("VideoDecoder: decode error: %s", qPrintable(avErrorString(ret)));
            return false;
        }

        if (eof)
            return false;

        bool sentPacket = false;
        while (!sentPacket) {
            ret = av_read_frame(formatCtx, packet);
            if (ret < 0) {
                eof = true;
                avcodec_send_packet(codecCtx, nullptr);
                break;
            }

            if (packet->stream_index == videoStream) {
                ret = avcodec_send_packet(codecCtx, packet);
                av_packet_unref(packet);
                if (ret < 0 && ret != AVERROR(EAGAIN)) {
                    qWarning(
                        "VideoDecoder: cannot send packet: %s", qPrintable(avErrorString(ret)));
                    return false;
                }
                sentPacket = true;
            } else {
                av_packet_unref(packet);
            }
        }
    }
}

bool VideoDecoder::readNext(QImage &image)
{
    if (!isOpened())
        return false;

    if (hasPending) {
        image = pendingFrame;
        lastIndex = pendingIndex;
        nextIndex = pendingIndex + 1;
        hasPending = false;
        pendingFrame = QImage();
        pendingIndex = -1;
        return true;
    }

    return readDecodedFrame(image);
}

bool VideoDecoder::seekTo(long frame)
{
    if (!isOpened() || frameRate <= 0.0)
        return false;

    if (frame < 0)
        frame = 0;
    if (totalFrames > 0 && frame >= totalFrames)
        frame = totalFrames - 1;

    if (hasPending && pendingIndex == frame)
        return true;
    if (!hasPending && nextIndex == frame)
        return true;

    const int64_t timestamp =
        static_cast<int64_t>(std::llround(static_cast<double>(frame) * AV_TIME_BASE / frameRate));

    const int ret = av_seek_frame(formatCtx, -1, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        qWarning(
            "VideoDecoder: seek to frame %ld failed: %s", frame, qPrintable(avErrorString(ret)));
        return false;
    }

    avcodec_flush_buffers(codecCtx);
    hasPending = false;
    pendingFrame = QImage();
    pendingIndex = -1;
    eof = false;
    lastIndex = -1;
    nextIndex = -1;

    QImage image;
    for (int guard = 0; guard < 100000; ++guard) {
        if (!readDecodedFrame(image))
            return false;

        const long index = lastIndex;
        if (index < 0)
            continue;

        if (index >= frame) {
            pendingFrame = image;
            pendingIndex = index;
            hasPending = true;
            nextIndex = index;
            lastIndex = index - 1;
            return true;
        }
    }

    return false;
}
