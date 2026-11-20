//
// Created by apkawa on 31.05.15.
//

#include "QJumpSlider.h"

void QJumpSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPoint pos = event->position().toPoint();
        if (orientation() == Qt::Vertical)
            setValue(minimum() + ((maximum() - minimum()) * (height() - pos.y())) / height());
        else
            setValue(minimum() + ((maximum() - minimum()) * pos.x()) / width());

        event->accept();
    }
    QSlider::mousePressEvent(event);
}

void QJumpSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (video_fps) {
        const QPoint pos = event->position().toPoint();
        long hovered_frame = minimum() + ((maximum() - minimum()) * pos.x()) / width();
        uint seconds = hovered_frame / video_fps;
        QTime hover_time = QTime(0, 0, 0).addSecs(seconds);
        QTime max_time = QTime(0, 0, 0).addSecs(maximum() / video_fps);
        QString time_display = hover_time.toString("hh:mm:ss");
        QString max_time_display = max_time.toString("hh:mm:ss");
        QString toolTipText =
            QString("%1 / %2 [%3]").arg(time_display).arg(max_time_display).arg(hovered_frame);
        this->setToolTip(toolTipText);
    }
    QSlider::mouseMoveEvent(event);
}
