#ifndef PLUGINCONFIGDIALOG_H
#define PLUGINCONFIGDIALOG_H

#include "plugintypes.h"

#include <QDialog>
#include <QString>
#include <QList>
#include <QVector>

#include <functional>

namespace Ui {
class PluginConfigDialog;
}

class PluginConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit PluginConfigDialog(QString name, QList<Pi::ConfigItem> &items, QWidget *parent = 0);
    ~PluginConfigDialog();

    static bool open(QString name, QList<Pi::ConfigItem> &items, QWidget *parent = 0);

private:
    Ui::PluginConfigDialog *ui;
    QVector<QWidget *> inputWidgets;
    QVector<bool> validatePass;
};

#endif // PLUGINCONFIGDIALOG_H
