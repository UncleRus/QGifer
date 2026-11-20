#ifndef WORKSPACEOBJECT_H
#define WORKSPACEOBJECT_H

#include <QObject>
#include <QImage>
#include <QList>
#include <QRect>
#include <QDebug>

struct WOPos {
    WOPos(double ix = 0, double iy = 0) : x(ix), y(iy)
    {
    }
    double x;
    double y;
};

struct WOSize {
    WOSize(double iw = 1, double ih = 1) : w(iw), h(ih)
    {
    }
    double w;
    double h;
};

namespace WO {
enum Mode {
    Normal = 0,
    Movable = 1,
    Moving = 2,
    XLScalable = 3,
    XLScaling = 4,
    XRScalable = 5,
    XRScaling = 6,
    YTScalable = 7,
    YTScaling = 8,
    YBScalable = 9,
    YBScaling = 10,
    XYScalable = 11,
    XYScaling = 12
};
enum Direction { Further, Earlier };
} // namespace WO

class WorkspaceObject : public QObject
{
    Q_OBJECT;

public:
    WorkspaceObject(QObject *parent = NULL);
    virtual ~WorkspaceObject();
    virtual QString getTypeName() const
    {
        return "WorkspaceObject";
    }
    void setImage(const QString &imgpath)
    {
        img = QImage(imgpath);
        imagePath = imgpath;
    }
    void setImage(const QImage &i)
    {
        img = i;
        imagePath = "";
    }
    QImage *image()
    {
        return &img;
    }
    const QString &getImagePath() const
    {
        return imagePath;
    }
    const WOPos &posAt(int i) const
    {
        const int idx = i - start;
        if (idx < 0 || idx >= pos.size()) {
            static const WOPos empty;
            return empty;
        }
        return pos.at(idx);
    }
    void setPosAt(int i, double x, double y)
    {
        const int idx = i - start;
        if (idx < 0 || idx >= pos.size())
            return;
        pos[idx].x = x;
        pos[idx].y = y;
    }
    /* void setStartFrame(int s) {start = s;adjustPosList();} */
    /* void setStopFrame(int s) {stop = s;adjustPosList();} */

    /**
     * Ustawia nowy zakres wyświetlania obiektu. Jeżeli parametr przyjmuje
     * wartość negatywną, zostanie zastąpiony aktualną klatką startową/końcową.
     *
     * @param startFrame Pierwsza klatka na której wyświetlany będzie obiekt
     * @param stopFrame Pierwsza klatka na której wyświetlany będzie obiekt
     */
    void setRange(int startFrame = -1, int stopFrame = -1);

    int getStart() const
    {
        return start;
    }
    int getStop() const
    {
        return stop;
    }
    QSize originalSize() const
    {
        return img.size();
    }
    QSize sizeAt(int i) const
    {
        const WOSize &s = scaleAt(i);
        return QSize(img.size().width() * s.w, img.size().height() * s.h);
    }
    const WOSize &scaleAt(int i) const
    {
        const int idx = i - start;
        if (idx < 0 || idx >= scale.size()) {
            static const WOSize empty;
            return empty;
        }
        return scale.at(idx);
    }
    void setScaleAt(int i, float xs, float ys);
    void setPreviewRect(const QRect &r)
    {
        pwRect = r;
    }
    const QRect &updatePreviewRect(int frame, const QSize &previewSize, const QSize &workspaceSize,
        const QSize &origFrameSize, double zoom, int xv = 0, int yv = 0);
    const QRect &previewRect() const
    {
        return pwRect;
    }
    void setMode(WO::Mode m)
    {
        mode = m;
    }
    WO::Mode currentMode() const
    {
        return mode;
    }
    void clonePosAt(int i, WO::Direction d);
    void cloneScaleAt(int i, WO::Direction d);

private:
    WO::Mode mode;
    QRect pwRect;
    QImage img;
    QString imagePath;
    int start;
    int stop;
    QList<WOPos> pos;
    QList<WOSize> scale;
private slots:
};

#endif
