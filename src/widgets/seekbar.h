#ifndef SEEKBAR_H
#define SEEKBAR_H

#include <QMouseEvent>
#include <QPaintEvent>
#include <QList>

#include "customslider.h"

class SeekBar : public CustomSlider {
    Q_OBJECT
public:
    explicit SeekBar(QWidget *parent = 0);

public slots:
    void setTotalTime(double _totalTime);
    void setChapterTicks(const QList<double> &values);
    void setBufferedRanges(const QList<QPair<double, double>> &values);
    void setMtspMode(bool mtsp);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent *event);

private:
    QList<double> ticks;
    bool tickReady;
    double totalTime;
    QList<QPair<double, double>> ranges;
    bool mtspMode = false;
};

#endif // SEEKBAR_H
