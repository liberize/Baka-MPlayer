#ifndef ONLINEWIDGET_H
#define ONLINEWIDGET_H

#include <QListWidget>

class BakaEngine;

class OnlineWidget : public QListWidget {
    Q_OBJECT

public:
    explicit OnlineWidget(QWidget *parent = 0);
    ~OnlineWidget();

    void AttachEngine(BakaEngine *baka);

signals:
    void mouseMoved();

protected:
    void mouseMoveEvent(QMouseEvent *event);

private:
    BakaEngine *baka;
};

#endif // ONLINEWIDGET_H
