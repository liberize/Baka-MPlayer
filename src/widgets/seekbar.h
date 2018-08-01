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
    void setTracking(double _totalTime);
    void setTicks(const QList<double> &values);
    void updateBufferedRanges(const QList<QPair<double, double>> &values);

protected:
    void mouseMoveEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent *event);

private:
    QList<double> ticks;
    bool tickReady;
    double totalTime;
    QList<QPair<double, double>> ranges;
};

#endif // SEEKBAR_H
