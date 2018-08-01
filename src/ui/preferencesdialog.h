#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

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
class PluginModel;
class PluginItemDelegate;

class PreferencesDialog : public QDialog {
    Q_OBJECT
public:
    explicit PreferencesDialog(BakaEngine *baka, QWidget *parent = 0);
    ~PreferencesDialog();

    static void showPreferences(BakaEngine *baka, QWidget *parent = 0);

protected:
    void PopulateLangs();
    void PopulateShortcuts();
    void PopulatePlugins();
    void UpdatePlugins();
    void AddRow(QString first, QString second, QString third);
    void ModifyRow(int row, QString first, QString second, QString third);
    void RemoveRow(int row);
    void SelectKey(bool add, QPair<QString, QPair<QString, QString>> init = (QPair<QString, QPair<QString, QString>>()));

private:
    Ui::PreferencesDialog *ui;
    BakaEngine *baka;
    QHash<QString, QPair<QString, QString>> input;

    QString screenshotDir;
    int numberOfShortcuts;

    class SortLock : public QMutex {
    public:
        SortLock(QTableWidget *parent);

        void lock();
        void unlock();
    private:
        QTableWidget *parent;
    } *sortLock;

    PluginModel *pluginModel = nullptr;
    PluginItemDelegate *pluginItemDelegate = nullptr;
    QSet<QString> configChangedPlugins;
};

#endif // PREFERENCESDIALOG_H
