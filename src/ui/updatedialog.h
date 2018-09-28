#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>
#include <QTime>


namespace Ui {
class UpdateDialog;
}

class BakaEngine;

class UpdateDialog : public QDialog {
    Q_OBJECT

public:
    explicit UpdateDialog(BakaEngine *baka, QWidget *parent = 0);
    ~UpdateDialog();

    static void checkForUpdates(BakaEngine *baka, QWidget *parent = 0);

protected slots:
    void showInfo();

private:
    Ui::UpdateDialog *ui;
    BakaEngine *baka;

    QTime *timer;
    double avgSpeed = 1;
    double lastSpeed = 0;
    int lastProgress;
    int lastTime;
    int state;
    bool init;
};

#endif // UPDATEDIALOG_H
