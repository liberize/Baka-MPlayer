#include "pluginmanager.h"
#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "bakaengine.h"
#include "ui/mainwindow.h"
#include "mpvhandler.h"
#include "ui/keydialog.h"
#include "util.h"
#include "delegates/pluginitemdelegate.h"
#include "ui/pluginconfigdialog.h"

#include <QFileDialog>
#include <QMessageBox>

PreferencesDialog::PreferencesDialog(BakaEngine *baka, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog),
    baka(baka),
    screenshotDir("")
{
    ui->setupUi(this);

    ui->infoWidget->sortByColumn(0, Qt::AscendingOrder);
    sortLock = new SortLock(ui->infoWidget);

    PopulateLangs();

    QString ontop = baka->window->getOnTop();
    if (ontop == "never")
        ui->neverRadioButton->setChecked(true);
    else if (ontop == "playing")
        ui->playingRadioButton->setChecked(true);
    else if (ontop == "always")
        ui->alwaysRadioButton->setChecked(true);
    ui->resumeCheckBox->setChecked(baka->window->getResume());
    ui->showTrayIconCheckBox->setChecked(baka->sysTrayIcon->isVisible());
    ui->showNotificationCheckBox->setChecked(baka->window->getShowNotification());
    ui->gestureCheckBox->setChecked(baka->window->getGestures());
    ui->langComboBox->setCurrentText(baka->window->getLang());
    ui->gestureCheckBox->setChecked(baka->window->getGestures());
    int maxRecent= baka->window->getMaxRecent();
    ui->recentCheckBox->setChecked(maxRecent > 0);
    ui->recentSpinBox->setValue(maxRecent);
    ui->resumeCheckBox->setChecked(baka->window->getResume());
    ui->formatComboBox->setCurrentText(baka->mpv->getScreenshotFormat());
    screenshotDir = QDir::toNativeSeparators(baka->mpv->getScreenshotDir());
    ui->templateLineEdit->setText(baka->mpv->getScreenshotTemplate());
    ui->msgLvlComboBox->setCurrentText(baka->mpv->getMsgLevel());

    pluginModel = new QStandardItemModel(this);
    pluginItemDelegate = new PluginItemDelegate(this);
    ui->pluginListView->setItemDelegate(pluginItemDelegate);
    ui->pluginListView->setModel(pluginModel);
    PopulatePlugins();

    // add shortcuts
    input = baka->input;
    PopulateShortcuts();

    connect(ui->changeButton, &QPushButton::clicked, [=] {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Choose screenshot directory"), screenshotDir);
        if (dir != QString())
            screenshotDir = dir;
    });

    connect(ui->addKeyButton, &QPushButton::clicked, [=] {
        SelectKey(true);
    });

    connect(ui->editKeyButton, &QPushButton::clicked, [=] {
        int i = ui->infoWidget->currentRow();
        if (i == -1)
            return;

        SelectKey(false,
            {ui->infoWidget->item(i, 0)->text(),
            {ui->infoWidget->item(i, 1)->text(),
             ui->infoWidget->item(i, 2)->text()}});
    });

    connect(ui->resetKeyButton, &QPushButton::clicked, [=] {
        if (QMessageBox::question(this, tr("Reset All Key Bindings?"), tr("Are you sure you want to reset all shortcut keys to its original bindings?")) == QMessageBox::Yes) {
            input = baka->default_input;
            while (numberOfShortcuts > 0)
                RemoveRow(numberOfShortcuts-1);
            PopulateShortcuts();
        }
    });

    connect(ui->removeKeyButton, &QPushButton::clicked, [=] {
        int row = ui->infoWidget->currentRow();
        if (row == -1)
            return;

        input[ui->infoWidget->item(row, 0)->text()] = {QString(), QString()};
        RemoveRow(row);
    });

    connect(ui->infoWidget, &QTableWidget::currentCellChanged, [=](int r,int,int,int) {
        ui->editKeyButton->setEnabled(r != -1);
        ui->removeKeyButton->setEnabled(r != -1);
    });

    connect(ui->infoWidget, &QTableWidget::doubleClicked, [=](const QModelIndex &index) {
        int i = index.row();
        SelectKey(false,
            {ui->infoWidget->item(i, 0)->text(),
            {ui->infoWidget->item(i, 1)->text(),
             ui->infoWidget->item(i, 2)->text()}});
    });

    connect(ui->recentCheckBox, SIGNAL(toggled(bool)),
            ui->recentSpinBox, SLOT(setEnabled(bool)));

    connect(ui->openPluginFolderButton, &QPushButton::clicked, [=] {
        QModelIndex index = ui->pluginListView->currentIndex();
        Plugin *plugin = index.data(Qt::UserRole).value<Plugin*>();
        Util::ShowInFolder(plugin->getPath(), "");
    });

    connect(ui->pluginConfigButton, &QPushButton::clicked, [=] {
        QModelIndex index = ui->pluginListView->currentIndex();
        Plugin *plugin = index.data(Qt::UserRole).value<Plugin*>();
        if (!plugin->getConfig().empty()) {
            auto config = pluginConfigs.value(plugin->getName(), plugin->getConfig());
            if (PluginConfigDialog::open(plugin->getName(), config, this))
                pluginConfigs[plugin->getName()] = config;
        }
    });

    connect(ui->pluginListView->selectionModel(), &QItemSelectionModel::currentChanged, [=] (const QModelIndex &current, const QModelIndex &) {
        Plugin *plugin = current.data(Qt::UserRole).value<Plugin*>();
        ui->pluginConfigButton->setEnabled(!plugin->getConfig().empty());
        ui->openPluginFolderButton->setEnabled(true);
    });

    connect(ui->okButton, SIGNAL(clicked()),
            this, SLOT(accept()));

    connect(ui->cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
}

PreferencesDialog::~PreferencesDialog()
{
    if (result() == QDialog::Accepted) {
        baka->window->setResume(ui->resumeCheckBox->isChecked());
        if (ui->neverRadioButton->isChecked())
            baka->window->setOnTop("never");
        else if (ui->playingRadioButton->isChecked())
            baka->window->setOnTop("playing");
        else if (ui->alwaysRadioButton->isChecked())
            baka->window->setOnTop("always");
        baka->sysTrayIcon->setVisible(ui->showTrayIconCheckBox->isChecked());
        baka->window->setShowNotification(ui->showNotificationCheckBox->isChecked());
        baka->window->setGestures(ui->gestureCheckBox->isChecked());
        baka->window->setLang(ui->langComboBox->currentText());
        baka->window->setMaxRecent(ui->recentCheckBox->isChecked() ? ui->recentSpinBox->value() : 0);
        baka->window->setGestures(ui->gestureCheckBox->isChecked());
        baka->window->setResume(ui->resumeCheckBox->isChecked());
        baka->mpv->ScreenshotFormat(ui->formatComboBox->currentText());
        baka->mpv->ScreenshotDirectory(screenshotDir);
        baka->mpv->ScreenshotTemplate(ui->templateLineEdit->text());
        baka->mpv->MsgLevel(ui->msgLvlComboBox->currentText());
        baka->input = input;
        baka->window->MapShortcuts();
        UpdatePlugins();
    }
    delete sortLock;
    delete ui;
}

void PreferencesDialog::showPreferences(BakaEngine *baka, QWidget *parent)
{
    PreferencesDialog dialog(baka, parent);
    dialog.exec();
}

void PreferencesDialog::PopulateLangs()
{
    // open the language directory
    QDir root(Util::TranslationsPath());
    // get files in the directory with .qm extension
    QFileInfoList flist;
    flist = root.entryInfoList({"*.qm"}, QDir::Files);
    // add the languages to the combo box
    ui->langComboBox->addItem("auto");
    for (auto &i : flist) {
        QString lang = i.fileName().mid(i.fileName().indexOf("_") + 1); // baka-mplayer_....
        lang.chop(3); // -  .qm
        ui->langComboBox->addItem(lang);
    }
}

void PreferencesDialog::PopulateShortcuts()
{
    sortLock->lock();
    numberOfShortcuts = 0;
    for (auto iter = input.begin(); iter != input.end(); ++iter) {
        QPair<QString, QString> p = iter.value();
        if (p.first == QString() || p.second == QString())
            continue;
        AddRow(iter.key(), p.first, p.second);
    }
    sortLock->unlock();
}

void PreferencesDialog::PopulatePlugins()
{
    auto plugins = baka->pluginManager->getPlugins();
    for (auto plugin : plugins) {
        QStandardItem *item = new QStandardItem;
        item->setData(QVariant::fromValue(plugin), Qt::UserRole);
        item->setCheckable(true);
        item->setEditable(false);
        item->setCheckState(plugin->isEnabled() ? Qt::Checked : Qt::Unchecked);
        pluginModel->appendRow(item);
    }
}

void PreferencesDialog::UpdatePlugins()
{
    for (int i = 0; i < pluginModel->rowCount(); i++) {
        QModelIndex index = pluginModel->index(i, 0);
        Plugin *plugin = pluginModel->data(index, Qt::UserRole).value<Plugin*>();
        bool enable = (pluginModel->data(index, Qt::CheckStateRole) == Qt::Checked);
        if (plugin->isEnabled() != enable)
            plugin->setEnabled(enable);
        if (pluginConfigs.contains(plugin->getName()))
            plugin->updateConfig(pluginConfigs[plugin->getName()]);
    }
}

void PreferencesDialog::AddRow(QString first, QString second, QString third)
{
    bool locked = sortLock->tryLock();
    ui->infoWidget->insertRow(numberOfShortcuts);
    ui->infoWidget->setItem(numberOfShortcuts, 0, new QTableWidgetItem(first));
    ui->infoWidget->setItem(numberOfShortcuts, 1, new QTableWidgetItem(second));
    ui->infoWidget->setItem(numberOfShortcuts, 2, new QTableWidgetItem(third));
    ++numberOfShortcuts;
    if (locked)
        sortLock->unlock();
}

void PreferencesDialog::ModifyRow(int row, QString first, QString second, QString third)
{
    bool locked = sortLock->tryLock();
    ui->infoWidget->item(row, 0)->setText(first);
    ui->infoWidget->item(row, 1)->setText(second);
    ui->infoWidget->item(row, 2)->setText(third);
    if (locked)
        sortLock->unlock();
}

void PreferencesDialog::RemoveRow(int row)
{
    bool locked = sortLock->tryLock();
    ui->infoWidget->removeCellWidget(row, 0);
    ui->infoWidget->removeCellWidget(row, 1);
    ui->infoWidget->removeCellWidget(row, 2);
    ui->infoWidget->removeRow(row);
    --numberOfShortcuts;
    if (locked)
        sortLock->unlock();
}

void PreferencesDialog::SelectKey(bool add, QPair<QString, QPair<QString, QString>> init)
{
    sortLock->lock();
    KeyDialog dialog(this);
    int status = 0;
    while (status != 2) {
        QPair<QString, QPair<QString, QString>> result = dialog.SelectKey(add, init);
        if (result == QPair<QString, QPair<QString, QString>>()) // cancel
            break;
        for (int i = 0; i < numberOfShortcuts; ++i) {
            if (!add && i == ui->infoWidget->currentRow()) // don't compare selected row if we're changing
                continue;
            if (ui->infoWidget->item(i, 0)->text() == result.first) {
                if (QMessageBox::question(this,
                       tr("Existing keybinding"),
                       tr("%0 is already being used. Would you like to change its function?").arg(
                           result.first)) == QMessageBox::Yes) {
                    input[ui->infoWidget->item(i, 0)->text()] = {QString(), QString()};
                    RemoveRow(i);
                    status = 0;
                } else {
                    init = result;
                    status = 1;
                }
                break;
            }
        }
        if (status == 0) {
            if (add) // add
                AddRow(result.first, result.second.first, result.second.second);
            else { // change
                if (result.first != init.first)
                    input[init.first] = {QString(), QString()};
                ModifyRow(ui->infoWidget->currentRow(), result.first, result.second.first, result.second.second);
            }
            input[result.first] = result.second;
            status = 2;
        } else
            status = 0;
    }
    sortLock->unlock();
}

PreferencesDialog::SortLock::SortLock(QTableWidget *parent):parent(parent) {}

void PreferencesDialog::SortLock::lock()
{
    parent->setSortingEnabled(false);
    QMutex::lock();
}

void PreferencesDialog::SortLock::unlock()
{
    QMutex::unlock();
    parent->setSortingEnabled(true);
}
