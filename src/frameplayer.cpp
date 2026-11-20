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

#include "frameplayer.h"

#include <algorithm>
#include <vector>

namespace {

int reflect101(int pos, int size)
{
    if (size <= 1)
        return 0;
    while (pos < 0 || pos >= size) {
        if (pos < 0)
            pos = -pos;
        if (pos >= size)
            pos = 2 * size - 2 - pos;
    }
    return pos;
}

uchar medianOf(std::vector<uchar> &values, int count)
{
    const int middle = count / 2;
    std::nth_element(values.begin(), values.begin() + middle, values.begin() + count);
    return values[middle];
}

/**
 * Per-channel median filter; mirrors cv::medianBlur(k) with the default
 * BORDER_REFLECT_101 border handling that FramePlayer relied on.
 */
void medianBlur(QImage &image, int kernel)
{
    if (image.isNull() || kernel < 3)
        return;
    if (image.format() != QImage::Format_RGB888)
        image = image.convertToFormat(QImage::Format_RGB888);

    const int width = image.width();
    const int height = image.height();
    const int radius = kernel / 2;
    const int window = kernel * kernel;
    const QImage source = image.copy();

    std::vector<uchar> red(window);
    std::vector<uchar> green(window);
    std::vector<uchar> blue(window);

    for (int y = 0; y < height; ++y) {
        uchar *dst = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            int n = 0;
            for (int dy = -radius; dy <= radius; ++dy) {
                const uchar *line = source.constScanLine(reflect101(y + dy, height));
                for (int dx = -radius; dx <= radius; ++dx) {
                    const uchar *pixel = line + 3 * reflect101(x + dx, width);
                    red[n] = pixel[0];
                    green[n] = pixel[1];
                    blue[n] = pixel[2];
                    ++n;
                }
            }
            dst[3 * x] = medianOf(red, n);
            dst[3 * x + 1] = medianOf(green, n);
            dst[3 * x + 2] = medianOf(blue, n);
        }
    }
}

} // namespace

FramePlayer::FramePlayer(QWidget *parent)
    : QWidget(parent), totalFrames(0), originalSize(QSize(0, 0)), currentPos(-1), timerId(-1),
      status(Stopped), statusbar(nullptr), interval(40), medianblur(0)
{
    setupUi(this);
    workspace = new Workspace(frame);
    workspace->enableBackground(true);
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
}

FramePlayer::~FramePlayer()
{
    // delete workspace;
}

bool FramePlayer::openSource(const QString &src)
{
    killTimer(timerId);
    timerId = -1;
    currentPos = -1;

    if (!decoder.open(src)) {
        qDebug() << "FramePlayer::openSource cannot load video: " << src;
        close();
        return false;
    }

    qDebug() << "FramePlayer::openSource new video loaded: " << src;

    filepath = src;
    originalSize = decoder.originalSize();
    totalFrames = decoder.frameCount();
    qDebug() << "total frames: " << totalFrames << ", codec: " << decoder.codecName();

    interval = estimateInterval();
    if (!interval) {
        interval = 40;
    }

    slider->setMaximum(countFrames() > 0 ? countFrames() - 1 : 0);
    slider->setVideoFps(fps());

    disconnect(slider, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
    slider->setValue(0);
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));

    nextFrame();
    return true;
}

void FramePlayer::close()
{
    stop();
    decoder.close();
    totalFrames = 0;
    filepath.clear();
    originalSize = QSize(0, 0);
    slider->setValue(0);
    slider->setVideoFps(0.0);
    showDefaultScreen();
    updateStatus(status);
}

void FramePlayer::nextFrame()
{
    if (!decoder.isOpened()) {
        return;
    }

    if (totalFrames > 0 && currentPos + 1 > totalFrames - 1) {
        return;
    }

    QImage frameImage;
    if (!decoder.readNext(frameImage)) {
        return;
    }

    if (decoder.currentIndex() >= 0) {
        currentPos = decoder.currentIndex();
    } else {
        ++currentPos;
    }
    if (totalFrames > 0 && currentPos >= totalFrames) {
        currentPos = totalFrames - 1;
    }

    qDebug() << "current pos: " << currentPos << "/" << totalFrames;

    if (medianblur) {
        medianBlur(frameImage, medianblur % 2 ? medianblur : medianblur + 1);
    }
    currentFrame = frameImage;

    workspace->setImage(currentFrame, frame->size());
    workspace->updateFrameIndex(currentPos);
    emit frameChanged(currentPos);
    updateSlider(currentPos);

    updateStatus(timerId == -1 ? Stopped : Playing);
}

void FramePlayer::reverse_play()
{
    qDebug() << "PLAY reversed:";
    is_reverse_play = true;
    if (!countFrames()) {
        return;
    }
    if (currentPos == countFrames() - 1) {
        qDebug() << "currentPos:" << currentPos << " countFrames():" << countFrames();
        seek(0);
    }
    if (timerId == -1) {
        timerId = startTimer(interval);
    }
    updateStatus(Playing);
}

void FramePlayer::play()
{
    qDebug() << "PLAY";
    is_reverse_play = false;
    if (!countFrames()) {
        return;
    }

    if (currentPos == countFrames() - 1) {
        qDebug() << "currentPos:" << currentPos << " countFrames():" << countFrames();
        seek(0);
    }

    if (timerId == -1) {
        timerId = startTimer(interval);
    }
    updateStatus(Playing);
}

void FramePlayer::stop()
{
    pause();
    seek(0);
}

void FramePlayer::pause()
{
    killTimer(timerId);
    timerId = -1;
    updateStatus(Stopped);
}

void FramePlayer::setPos(long pos)
{
    if (!decoder.isOpened())
        return;

    if (pos < 0) {
        pos = 0;
    }
    if (totalFrames > 0 && pos >= totalFrames) {
        pos = totalFrames - 1;
    }
    if (!decoder.seekTo(pos)) {
        return;
    }
    currentPos = pos - 1;
    nextFrame();
}

void FramePlayer::slowSetPos(long pos)
{
    // Seeking through libav only decodes from the nearest key frame, so the
    // extra "grab" loop that OpenCV needed is no longer necessary.
    setPos(pos);
}

void FramePlayer::seek(int pos)
{
    qDebug() << "seeking pos: " << pos;
    setPos(pos);
}

void FramePlayer::timerEvent(QTimerEvent *)
{
    if (is_reverse_play) {
        prevFrame();
    } else {
        nextFrame();
    }
    if ((currentPos >= countFrames() - 1) || (currentPos <= 0)) {
        stop();
    }
}

void FramePlayer::updateSlider(int pos)
{
    if (pos < 0)
        pos = 0;
    disconnect(slider, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
    slider->setValue(pos);
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
}

void FramePlayer::updateStatus(Status s)
{
    QString info = "";
    status = s;
    info = (status == Stopped ? "Stopped" : "Playing");

    long total = countFrames();
    if (total)
        info += ", frame: " + QString::number(currentPos) + "/" + QString::number(total - 1) + " ("
                + QString::number(total) + " in total)";
    else
        info += ", no frames";
    slider->setMaximum(total == 0 ? 0 : total - 1);
    if (statusbar) {
        statusbar->showMessage(info);
    }

    emit statusUpdated(s);
}

void FramePlayer::resizeEvent(QResizeEvent *)
{
    workspace->updateBackground();
    if (countFrames())
        workspace->setImage(currentFrame, frame->size());
    else
        showDefaultScreen();
    workspace->setFixedSize(frame->size());
}

void FramePlayer::setStatusBar(QStatusBar *sb)
{
    statusbar = sb;
}

QString FramePlayer::codecName()
{
    return decoder.codecName();
}

void FramePlayer::renderDefaultTextImage(const QString &text)
{
    QPixmap def(800, 450);
    def.fill(Qt::transparent);
    defaultImg = def.toImage();
    QFont f;
    f.setPointSize(26);
    f.setBold(true);
    QImage txt = TextRenderer::renderText(text, f, QColor(30, 30, 30), QColor(255, 255, 255), 1)
                     .scaledToWidth(defaultImg.width(), Qt::SmoothTransformation);
    QPainter p(&defaultImg);
    p.drawImage(
        (defaultImg.width() - txt.width()) / 2, (defaultImg.height() - txt.height()) / 2, txt);
}

QSize FramePlayer::getOriginalSize() const
{
    return originalSize;
}
