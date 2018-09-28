#include "overlayhandler.h"

#include "bakaengine.h"
#include "mpvhandler.h"
#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include "overlay.h"

#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QTimer>
#include <QFontMetrics>
#include <QThread>

#define OVERLAY_INFO 62
#define OVERLAY_STATUS 63
#define OVERLAY_ALBUM_ART 64
#define OVERLAY_REFRESH_RATE 1000


OverlayHandler::OverlayHandler(QObject *parent):
    QObject(parent),
    baka(static_cast<BakaEngine*>(parent)),
    refresh_timer(nullptr),
    min_overlay(1),
    max_overlay(60),
    overlay_id(min_overlay)
{
}

OverlayHandler::~OverlayHandler()
{
    for (auto o : overlays)
        delete o;
}

void OverlayHandler::showStatusText(const QString &text, int duration)
{
    if (text != QString())
        showText(text,
                 QFont(Util::monospaceFont(),
                       14, QFont::Bold), QColor(0xFFFFFF),
                 QPoint(20, 20), duration, OVERLAY_STATUS);
    else if (duration == 0)
        remove(OVERLAY_STATUS);
}

void OverlayHandler::showInfoText(bool show)
{
    if (show) { // show media info
        if (refresh_timer == nullptr) {
            refresh_timer = new QTimer(this);
            refresh_timer->setSingleShot(true);
            connect(refresh_timer, &QTimer::timeout, // on timeout
                    [=] { showInfoText(); });
        }
        refresh_timer->start(OVERLAY_REFRESH_RATE);
        showText(baka->mpv->getMediaInfo(),
                 QFont(Util::monospaceFont(),
                       14, QFont::Bold), QColor(0xFFFF00),
                 QPoint(20, 20), 0, OVERLAY_INFO);
    } else { // hide media info
        delete refresh_timer;
        refresh_timer = nullptr;
        remove(OVERLAY_INFO);
    }
}

void OverlayHandler::showText(const QString &text, QFont font, QColor color, QPoint pos, int duration, int id)
{
    overlay_mutex.lock();
    // increase next overlay_id
    if (id == -1) { // auto id
        id = overlay_id;
        if (overlay_id+1 > max_overlay)
            overlay_id = min_overlay;
        else
            ++overlay_id;
    }

    QFontMetrics fm(font);
    QRect bounds = fm.boundingRect(QRect(), 0, text);
    float xF = float(baka->window->ui->mpvContainer->width() - 2 * pos.x()) / bounds.width();
    float yF = float(baka->window->ui->mpvContainer->height() - 2 * pos.y()) / bounds.height();
    font.setPointSizeF(qMin(font.pointSizeF() * qMin(xF, yF), font.pointSizeF()));
    fm = QFontMetrics(font);
    bounds = fm.boundingRect(QRect(), 0, text);

    QImage *canvas = new QImage(bounds.width(), bounds.height(), QImage::Format_ARGB32);
    canvas->fill(QColor(0, 0, 0, 0));
    QPainter painter(canvas); // prepare to paint
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(canvas->rect(), Qt::AlignLeft | Qt::AlignTop, text);

    // add as mpv overlay
    baka->mpv->addOverlay(
        id == -1 ? overlay_id : id,
        pos.x(), pos.y(),
        "&" + QString::number(quintptr(canvas->bits())),
        0, canvas->width(), canvas->height());

    // add over mpv as label
    QLabel *label = new QLabel(baka->window->ui->mpvContainer);
    label->setPixmap(QPixmap::fromImage(*canvas));
#ifdef ENABLE_MPV_COCOA_WIDGET
    Util::setWantsLayer(label, true);
    Util::setLayerBackgroundColor(label, 41, 41, 41, 255);
    Util::setLayerCornerRadius(label, 5);
    label->setStyleSheet("background: #202020; padding: 10px;");
    label->setGeometry(pos.x() - 10, pos.y() - 10, canvas->width() + 20, canvas->height() + 20);
#else
    label->setStyleSheet("background-color: rgb(0,0,0,0); background-image: url();");
    label->setGeometry(pos.x(), pos.y(), canvas->width(), canvas->height());
#endif
    label->show();

    QTimer *timer;
    if (duration == 0)
        timer = nullptr;
    else {
        timer = new QTimer(this);
        timer->start(duration);
        connect(timer, &QTimer::timeout, [=] {
            remove(id);
        });
    }

    if (overlays.find(id) != overlays.end())
        delete overlays[id];
    overlays[id] = new Overlay(label, canvas, timer, this);
    overlay_mutex.unlock();
}

void OverlayHandler::showImage(const QPixmap &pixmap, QPoint pos, int duration, int id)
{
    overlay_mutex.lock();
    // increase next overlay_id
    if (id == -1) { // auto id
        id = overlay_id;
        if (overlay_id+1 > max_overlay)
            overlay_id = min_overlay;
        else
            ++overlay_id;
    }

    // add over mpv as label
    QLabel *label = new QLabel(baka->window->ui->mpvContainer);
    label->setPixmap(pixmap);
#ifdef ENABLE_MPV_COCOA_WIDGET
    Util::setWantsLayer(label, true);
#endif
    label->setGeometry(pos.x(), pos.y(), pixmap.width(), pixmap.height());
    label->show();

    QTimer *timer;
    if (duration == 0)
        timer = nullptr;
    else {
        timer = new QTimer(this);
        timer->start(duration);
        connect(timer, &QTimer::timeout, [=] {
            remove(id);
        });
    }

    if (overlays.find(id) != overlays.end())
        delete overlays[id];
    overlays[id] = new Overlay(label, nullptr, timer, this);
    overlay_mutex.unlock();
}

void OverlayHandler::remove(int id)
{
    overlay_mutex.lock();
    baka->mpv->removeOverlay(id);
    if (overlays.find(id) != overlays.end()) {
        delete overlays[id];
        overlays.remove(id);
    }
    overlay_mutex.unlock();
}
