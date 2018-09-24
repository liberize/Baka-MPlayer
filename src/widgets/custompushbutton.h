#ifndef CUSTOMPUSHBUTTON_H
#define CUSTOMPUSHBUTTON_H

#include <QPushButton>

class CustomPushButton : public QPushButton {
    Q_OBJECT

public:
    explicit CustomPushButton(QWidget *parent = 0);

    void SetIcon(const QIcon &icon, const QSize &size, int space);

protected:
    void paintEvent(QPaintEvent* e);

private:
    QPixmap pixmap;
};

#endif // CUSTOMPUSHBUTTON_H
