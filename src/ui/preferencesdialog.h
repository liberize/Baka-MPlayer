#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "plugintypes.h"

#include <QDialog>
#include <QString>
#include <QPair>
#include <QMutex>
#include <QTableWidget>
#include <QStandardItemModel>
#include <QSet>

namespace Ui {
class PreferencesDialog;
}

class BakaEngine;
class PluginItemDelegate;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(BakaEngine *baka, QWidget *parent = 0);
    ~PreferencesDialog();

    static void showPreferences(BakaEngine *baka, QWidget *parent = 0);

protected:
    void populateLangs();
    void populateShortcuts();
    void populatePlugins();
    void updatePlugins();
    void addRow(QString first, QString second, QString third);
    void modifyRow(int row, QString first, QString second, QString third);
    void removeRow(int row);
    void selectKey(bool add, QPair<QString, QPair<QString, QString>> init = (QPair<QString, QPair<QString, QString>>()));

private:
    class SortLock : public QMutex {
    public:
        SortLock(QTableWidget *parent);

        void lock();
        void unlock();
    private:
        QTableWidget *parent;
    };

private:
    Ui::PreferencesDialog *ui;
    BakaEngine *baka;
    QHash<QString, QPair<QString, QString>> input;

    QString screenshotDir;
    int numberOfShortcuts;

    SortLock *sortLock;

    QStandardItemModel *pluginModel = nullptr;
    PluginItemDelegate *pluginItemDelegate = nullptr;
    QMap<QString, QVector<ConfigItem>> pluginConfigs;
};

#endif // PREFERENCESDIALOG_H
