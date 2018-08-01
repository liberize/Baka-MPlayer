#include "seekbar.h"

#include <QTime>
#include <QToolTip>
#include <QPainter>
#include <QRect>
#include <QStyle>

#include "util.h"

SeekBar::SeekBar(QWidget *parent):
    CustomSlider(parent),
    tickReady(false),
    totalTime(0)
{
}

void SeekBar::setTracking(double _totalTime)
{
    if (_totalTime != 0) {
        totalTime = _totalTime;
        // now that we've got totalTime, calculate the tick locations
        // we need to do this because totalTime is obtained after the LOADED event is fired--we need totalTime for calculations
        if (!ticks.empty() && !tickReady)
            setTicks(ticks);
        setMouseTracking(true);
    } else
        setMouseTracking(false);
}

void SeekBar::setTicks(const QList<double> &values)
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

void SeekBar::updateBufferedRanges(const QVector<QPair<double, double>> &values)
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

void SeekBar::mouseMoveEvent(QMouseEvent* event)
{
    if (totalTime != 0) {
        QToolTip::showText(QPoint(event->globalX()-25, mapToGlobal(rect().topLeft()).y()-40),
                           Util::FormatTime(QStyle::sliderValueFromPosition(minimum(), maximum(), event->x(), width())*(double)totalTime/maximum(), totalTime),
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

    int min = minimum(), max = maximum(), val = value(), w = width();
    QRect region = event->rect();
    QPainter painter(this);

    if (!ranges.empty()) {
        painter.setPen(QColor(190, 190, 190));
        for (const auto &range : ranges) {
            if (range.second < val)
                continue;
            if (range.first < val)
                range.first = val;
            int x1 = QStyle::sliderPositionFromValue(min, max, range.first, w);
            int x2 = QStyle::sliderPositionFromValue(min, max, range.second, w);
            painter.drawRect(x1, region.top(), x2 - x1 + 1, region.height());
        }
    }

    if (tickReady) {
        painter.setPen(QColor(190, 190, 190));
        for (auto &tick : ticks) {
            int x = QStyle::sliderPositionFromValue(min, max, tick, w);
            painter.drawLine(x, region.top(), x, region.bottom());
        }
    }
}
