#include "bakaengine.h"

#include <QMessageBox>
#include <QDir>

#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"
#include "mpvhandler.h"
#include "gesturehandler.h"
#include "overlayhandler.h"
#include "updatemanager.h"
#include "widgets/dimdialog.h"
#include "requestmanager.h"
#include "pluginmanager.h"
#include "mtspmessagehandler.h"


#include "util.h"

BakaEngine::BakaEngine(QObject *parent):
    QObject(parent),
    window(static_cast<MainWindow*>(parent)),
    mpv(new MpvHandler(window->ui->mpvContainer, this)),
    settings(new Settings(Util::settingsPath(), this)),
    gesture(new GestureHandler(this)),
    overlay(new OverlayHandler(this)),
    update(new UpdateManager(this)),
    tempDir(new QTemporaryDir),
    pluginManager(new PluginManager(this)),
    requestManager(new RequestManager(this)),
    mtspMessageHandler(new MtspMessageHandler(this)),
    // note: trayIcon does not work in my environment--known qt bug
    // see: https://bugreports.qt-project.org/browse/QTBUG-34364
    sysTrayIcon(new QSystemTrayIcon(window->getTrayIcon(), this)),
    // todo: tray menu/tooltip
    translator(nullptr),
    qtTranslator(nullptr)
{
    if (Util::isDimLightsSupported())
        dimDialog = new DimDialog(window, nullptr);
    else {
        dimDialog = nullptr;
        window->ui->actionDimLights->setEnabled(false);
    }

    connect(mpv, &MpvHandler::messageSignal, [=] (QString msg) {
        print(msg, "mpv");
    });
    connect(update, &UpdateManager::messageSignal, [=] (QString msg) {
        print(msg, "update");
    });
}

BakaEngine::~BakaEngine()
{
    if (translator != nullptr)
        delete translator;
    if (qtTranslator != nullptr)
        delete qtTranslator;
    if (dimDialog != nullptr)
        delete dimDialog;
    delete mtspMessageHandler;
    delete requestManager;
    delete pluginManager;
    delete tempDir;
    delete update;
    delete overlay;
    delete gesture;
    delete settings;
    delete mpv;
}

void BakaEngine::loadSettings()
{
    settings->load();
    load2_0_3();
}

void BakaEngine::loadPlugins()
{
    pluginManager->loadPlugins();
    connect(pluginManager, &PluginManager::pluginsLoaded, [=] (const QMap<QString, Plugin*> &plugins) {
        for (auto plugin : plugins) {
            window->registerPlugin(plugin);
            plugin->setEnabled(!pluginManager->getDisableList().contains(plugin->getName()));
        }
    });
}

void BakaEngine::command(QString cmd)
{
    if (cmd == QString())
        return;
    QStringList args = cmd.split(" ");
    if (!args.empty()) {
        if (args.front() == "baka") // implicitly understood
            args.pop_front();

        if (!args.empty()) {
            auto iter = BakaCommandMap.find(args.front());
            if (iter != BakaCommandMap.end()) {
                args.pop_front();
                (this->*(iter->first))(args); // execute command
            } else
                invalidCommand(args.join(' '));
        } else
            requiresParameters("baka");
    } else
        invalidCommand(args.join(' '));
}

void BakaEngine::print(QString what, QString who)
{
    QString out = QString("[%0]: %1").arg(who, what);
    (qStdout() << out).flush();
}

void BakaEngine::println(QString what, QString who)
{
    print(what + "\n", who);
}

void BakaEngine::invalidCommand(QString command)
{
    println(tr("invalid command '%0'").arg(command));
}

void BakaEngine::invalidParameter(QString parameter)
{
    println(tr("invalid parameter '%0'").arg(parameter));
}

void BakaEngine::requiresParameters(QString what)
{
    println(tr("'%0' requires parameters").arg(what));
}
