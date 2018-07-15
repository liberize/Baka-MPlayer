#ifndef GESTUREHANDLER_H
#define GESTUREHANDLER_H

#include <QObject>
#include <QElapsedTimer>
#include <QPoint>

class BakaEngine;

class GestureHandler : public QObject {
    Q_OBJECT
public:
    enum GESTURE_TYPE {
        MOVE,
        HSEEK_VVOLUME
    };

    enum GESTURE_STATE {
        NONE,
        SEEKING,
        ADJUSTING_VOLUME
    };

    explicit GestureHandler(QObject *parent = 0);
    ~GestureHandler();

public slots:
    bool Begin(int gesture_type, QPoint mousePos, QPoint windowPos);
    bool Process(QPoint mousePos);
    bool End();

private:
    BakaEngine *baka;

    QElapsedTimer *elapsedTimer;

    double hRatio;
    double vRatio;
    int timer_threshold;
    int gesture_threshold;

    int gesture_type;
    int gesture_state;
    struct {
        QPoint mousePos;
        QPoint windowPos;
        int    time;
        int    volume;
    } start;
};

#endif // GESTUREHANDLER_H
