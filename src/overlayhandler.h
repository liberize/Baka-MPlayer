#ifndef OVERLAYHANDLER_H
#define OVERLAYHANDLER_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QPoint>
#include <QHash>
#include <QFont>
#include <QColor>
#include <QLabel>
#include <QMutex>

class BakaEngine;
class Overlay;

class OverlayHandler : public QObject {
    Q_OBJECT
public:
    explicit OverlayHandler(QObject *parent = 0);
    ~OverlayHandler();

public slots:
    void showStatusText(const QString &text, int duration = 4000);
    void showInfoText(bool show = true);
    void showText(const QString &text, QFont font, QColor color, QPoint pos, int duration, int id = -1);
    void showImage(const QPixmap &pixmap, QPoint pos, int duration, int id = -1);

protected slots:
    void remove(int id);

private:
    BakaEngine *baka;

    QHash<int, Overlay*> overlays;
    QMutex overlay_mutex;

    QTimer *refresh_timer;
    int min_overlay;
    int max_overlay;
    int overlay_id;
};

#endif // OVERLAYHANDLER_H
