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

#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QImage>
#include <QSize>
#include <QString>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

/**
 * Sequential video decoder based on libav* (demuxer + decoder + swscale).
 *
 * The interface mirrors the subset of cv::VideoCapture that QGifer used:
 * sequential frame reading plus absolute seeking. Frames are converted to
 * RGB888 QImages so the rest of the application stays toolkit-agnostic.
 */
class VideoDecoder
{
public:
    VideoDecoder();
    ~VideoDecoder();

    VideoDecoder(const VideoDecoder &) = delete;
    VideoDecoder &operator=(const VideoDecoder &) = delete;

    bool open(const QString &path);
    void close();
    bool isOpened() const
    {
        return formatCtx != nullptr && codecCtx != nullptr;
    }

    QSize originalSize() const
    {
        return frameSize;
    }
    long frameCount() const
    {
        return totalFrames;
    }
    double fps() const
    {
        return frameRate;
    }
    QString codecName() const;

    /** Reads the next frame in display order. */
    bool readNext(QImage &image);

    /**
     * Positions the stream so that the next readNext() returns the given
     * frame index (or the first frame at/after it if the index cannot be
     * reproduced exactly).
     */
    bool seekTo(long frame);

    /** Index of the last frame returned by readNext(), or -1. */
    long currentIndex() const
    {
        return lastIndex;
    }

private:
    bool readDecodedFrame(QImage &image);
    bool convertFrame(const AVFrame *src, QImage &image);
    long frameIndex(const AVFrame *frame) const;
    void reset();

    AVFormatContext *formatCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    SwsContext *swsCtx = nullptr;
    int swsWidth = 0;
    int swsHeight = 0;
    int swsFormat = -1;
    AVFrame *decodedFrame = nullptr;
    AVPacket *packet = nullptr;

    int videoStream = -1;
    QSize frameSize;
    double frameRate = 0.0;
    long totalFrames = 0;
    long lastIndex = -1;
    long nextIndex = -1;

    QImage pendingFrame;
    long pendingIndex = -1;
    bool hasPending = false;
    bool eof = false;
};

#endif // VIDEODECODER_H
