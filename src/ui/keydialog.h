#ifndef KEYDIALOG_H
#define KEYDIALOG_H

#include <QDialog>

namespace Ui {
class KeyDialog;
}

class KeyDialog : public QDialog {
    Q_OBJECT

public:
    explicit KeyDialog(QWidget *parent = 0);
    ~KeyDialog();

    QPair<QString, QPair<QString, QString>> selectKey(bool add, QPair<QString, QPair<QString, QString>> init = (QPair<QString, QPair<QString, QString>>()));

protected:
    void setButtons();

private:
    Ui::KeyDialog *ui;
    bool add;
};

#endif // KEYDIALOG_H
