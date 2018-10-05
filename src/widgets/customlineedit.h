#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include <QLineEdit>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QIcon>

class CustomLineEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit CustomLineEdit(QWidget *parent = 0);

    void setIcon(const QIcon &icon, const QSize &size);

protected:
    void keyPressEvent(QKeyEvent *event);
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    bool event(QEvent *ev);

signals:
    void submitted(QString);
    void mouseMoved(QMouseEvent *event);

protected:
    int iconWidth() const;

private:
    QPixmap pixmap;
};

#endif // CUSTOMLINEEDIT_H
