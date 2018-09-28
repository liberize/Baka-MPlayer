#include "seekbar.h"

#include <QTime>
#include <QToolTip>
#include <QPainter>
#include <QRect>
#include <QStyle>
#include <QStyleOptionSlider>

#include "util.h"

SeekBar::SeekBar(QWidget *parent):
    CustomSlider(parent),
    tickReady(false),
    totalTime(0)
{
}

void SeekBar::setTotalTime(double _totalTime)
{
    if (_totalTime != 0) {
        totalTime = _totalTime;
        // now that we've got totalTime, calculate the tick locations
        // we need to do this because totalTime is obtained after the LOADED event is fired--we need totalTime for calculations
        if (!ticks.empty() && !tickReady)
            setChapterTicks(ticks);
        setMouseTracking(true);
    } else
        setMouseTracking(false);
}

void SeekBar::setChapterTicks(const QList<double> &values)
{
    if (!totalTime) {
        ticks = values; // just set the values
        tickReady = false; // ticks need to be converted when totalTime is obtained
    } else {
        ticks = values;
        for (auto &tick : ticks)
            tick = (tick / totalTime) * maximum();
        tickReady = (ticks.length() > 0);
        repaint(rect());
    }
}

void SeekBar::setBufferedRanges(const QList<QPair<double, double>> &values)
{
    if (!totalTime)
        return;

    ranges = values;
    for (auto &range : ranges) {
        range.first = (range.first / totalTime) * maximum();
        range.second = (range.second / totalTime) * maximum();
    }
    repaint(rect());
}

void SeekBar::mousePressEvent(QMouseEvent *event)
{
    ranges.clear();
    repaint();
    CustomSlider::mousePressEvent(event);
}

void SeekBar::mouseMoveEvent(QMouseEvent* event)
{
    if (totalTime != 0) {
        QToolTip::showText(QPoint(event->globalX()-25, mapToGlobal(rect().topLeft()).y()-40),
                           Util::formatTime(QStyle::sliderValueFromPosition(minimum(), maximum(), event->x(), width())*(double)totalTime/maximum(), totalTime),
                           this, rect());
    }
    QSlider::mouseMoveEvent(event);
}

void SeekBar::paintEvent(QPaintEvent *event)
{
    CustomSlider::paintEvent(event);

    if (!isEnabled())
        return;
    if (ranges.empty() && !tickReady)
        return;

    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
    QRect grooveRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    int min = minimum(), max = maximum(), w = width();
    QPainter painter(this);

    if (!ranges.empty()) {
        for (const auto &range : ranges) {
            int x1 = QStyle::sliderPositionFromValue(min, max, qMax((int)range.first, min), w);
            int x2 = QStyle::sliderPositionFromValue(min, max, qMin((int)range.second, max), w);
            if (x2 > handleRect.right()) {
                x1 = qMax(x1, handleRect.right() + 1);
                painter.fillRect(x1, grooveRect.top(), x2 - x1 + 1, grooveRect.height(), QColor(96, 96, 96));
            }
        }
    }

    if (tickReady) {
        painter.setPen(QColor(180, 180, 180));
        for (auto &tick : ticks) {
            int x = QStyle::sliderPositionFromValue(min, max, tick, w);
            if (x > handleRect.right() || x < handleRect.left())
                painter.drawLine(x, grooveRect.top(), x, grooveRect.bottom());
        }
    }
}
