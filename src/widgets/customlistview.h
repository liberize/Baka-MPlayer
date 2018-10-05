#ifndef CUSTOMLISTVIEW_H
#define CUSTOMLISTVIEW_H

#include <QListView>
#include <QMouseEvent>

class CustomListView : public QListView {
    Q_OBJECT

public:
    explicit CustomListView(QWidget *parent = 0);

    void setPlaceholderText(QString text) { placeholderText = text; }

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    bool event(QEvent *ev);

signals:
    void mouseMoved(QMouseEvent *event);

private:
    QString placeholderText;
};

#endif // CUSTOMLISTVIEW_H
