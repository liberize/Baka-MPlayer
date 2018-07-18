#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include <QLineEdit>
#include <QKeyEvent>
#include <QIcon>

class CustomLineEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit CustomLineEdit(QWidget *parent = 0);

    void SetIcon(const QIcon &ic, const QSize &size);

protected:
    void keyPressEvent(QKeyEvent *event);
    void paintEvent(QPaintEvent *event);

signals:
    void submitted(QString);

private:
    QIcon icon;
    QSize iconSize;
};

#endif // CUSTOMLINEEDIT_H
