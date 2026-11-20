#include "textrenderer.h"

#include <QPainterPath>

#include <algorithm>
#include <cmath>

QImage TextRenderer::renderText(const QString &text, const QFont &font, const QColor &textColor,
    const QColor &outlineColor, int outlineWidth)
{
    if (text.isEmpty())
        return QImage();
    QPen pen;
    pen.setColor(outlineColor);
    pen.setWidthF(outlineWidth);
    pen.setJoinStyle(Qt::RoundJoin);

    QPainterPath path;
    path.addText(0, 0, font, text);
    const QRectF bounds = path.boundingRect();
    if (bounds.isEmpty())
        return QImage();

    const qreal margin = std::max<qreal>(0.0, outlineWidth) + 2.0;
    const int width = static_cast<int>(std::ceil(bounds.width() + 2.0 * margin));
    const int height = static_cast<int>(std::ceil(bounds.height() + 2.0 * margin));

    QImage img(width, height, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing, true);
    p.translate(margin - bounds.left(), margin - bounds.top());

    p.setFont(font);
    p.setPen(pen);
    p.setBrush(textColor);
    p.drawPath(path);

    return img;
}

void TextRenderer::renderText(TextObject *o)
{
    o->setImage(renderText(
        o->getText(), o->getFont(), o->getTextColor(), o->getOutlineColor(), o->getOutlineWidth()));
}
