#include "pluginmanager.h"
#include "bakaengine.h"

#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"
#include "util.h"
#include "mpvhandler.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QDir>

#if defined(Q_OS_WIN)
#include <QDate>
#include "updatemanager.h"
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 2)
class QJsonValueRef2 {
public:
    QJsonValueRef2(const QJsonValueRef &v):
        val(v) {
    }
    QString toString(const QString &defaultValue) const {
        return val.isString() ? val.toString() : defaultValue;
    }
    bool toBool(bool defaultValue = false) const {
        return val.isBool() ? val.toBool() : defaultValue;
    }
    double toDouble(double defaultValue = 0) const {
        return val.isDouble() ? val.toDouble() : defaultValue;
    }
    int toInt(int defaultValue = 0) const {
        return val.isDouble() && val.toDouble() == val.toInt() ? val.toInt() : defaultValue;
    }
private:
    const QJsonValueRef &val;
};
#else
typedef QJsonValueRef QJsonValueRef2;
#endif

void BakaEngine::load2_0_3()
{
    QJsonObject root = settings->getRoot();
    window->setOnTop(QJsonValueRef2(root["onTop"]).toString("never"));
    sysTrayIcon->setVisible(QJsonValueRef2(root["trayIcon"]).toBool(false));
    window->setShowNotification(QJsonValueRef2(root["showNotification"]).toBool(false));
    window->setRemaining(QJsonValueRef2(root["remaining"]).toBool(true));

    int sidebarWidth = QJsonValueRef2(root["sidebarWidth"]).toInt(200);
    window->setSidebarWidth(sidebarWidth);

    double controlsCenterX = QJsonValueRef2(root["controlsCenterX"]).toDouble(0.5);
    double controlsCenterY = QJsonValueRef2(root["controlsCenterY"]).toDouble(0.2);
    window->setControlsCenterPos(QPointF(controlsCenterX, controlsCenterY));

    root["showAll"] = true;
    window->setScreenshotDialog(QJsonValueRef2(root["screenshotDialog"]).toBool(true));
    window->recent.clear();
    for (auto entry : root["recent"].toArray()) {
        QJsonObject entry_json = entry.toObject();
        window->recent.append(Recent(entry_json["path"].toString(), entry_json["title"].toString(), QJsonValueRef2(entry_json["time"]).toDouble(0)));
    }
    window->setMaxRecent(QJsonValueRef2(root["maxRecent"]).toInt(5));
    window->setGestures(QJsonValueRef2(root["gestures"]).toBool(true));
    window->setResume(QJsonValueRef2(root["resume"]).toBool(true));
    window->setLang(QJsonValueRef2(root["lang"]).toString("auto"));
#if defined(Q_OS_WIN)
    QDate last = QDate::fromString(root["lastcheck"].toString()); // convert to date
    if (last.daysTo(QDate::currentDate()) > 7) { // been a week since we last checked?
        update->checkForUpdates();
        root["lastcheck"] = QDate::currentDate().toString();
    }
#endif
    window->updateRecentFiles();

    // apply default shortcut mappings
    input = default_input;

    // load settings defined input bindings
    QJsonObject input_json = root["input"].toObject();
    for (auto &key : input_json.keys()) {
        QJsonArray parts = input_json[key].toArray();
        input[key] = QPair<QString, QString>{
            parts[0].toString(),
            parts[1].toString()
        };
    }

    window->mapShortcuts();

    QJsonObject mpv_json = root["mpv"].toObject();
    mpv->setVolume(QJsonValueRef2(mpv_json["volume"]).toInt(100));
    mpv_json.remove("volume");
    mpv->setSpeed(QJsonValueRef2(mpv_json["speed"]).toDouble(1.0));
    mpv_json.remove("speed");
    mpv->setVo(mpv_json["vo"].toString());
    mpv_json.remove("vo");
    mpv->setScreenshotTemplate(QJsonValueRef2(mpv_json["screenshot-template"]).toString("screenshot%#04n"));
    mpv_json.remove("screenshot-template");
    mpv->setScreenshotDirectory(QJsonValueRef2(mpv_json["screenshot-directory"]).toString("."));
    mpv_json.remove("screenshot-directory");
    mpv->setMsgLevel(QJsonValueRef2(mpv_json["msg-level"]).toString("status"));
    mpv_json.remove("msg-level");
    for (auto &key : mpv_json.keys())
        if (key != QString() && mpv_json[key].toString() != QString())
            mpv->mpvSetOption(key, mpv_json[key].toString());

    for (auto entry : root["disabledPlugins"].toArray())
        pluginManager->getDisableList().insert(entry.toString());

    window->setRepeatType(QJsonValueRef2(root["repeat"]).toString("off"));
}

void BakaEngine::saveSettings()
{
    QString version = "2.0.3";
    QJsonObject root = settings->getRoot();
    root["onTop"] = window->onTop;
    root["trayIcon"] = sysTrayIcon->isVisible();
    root["showNotification"] = window->showNotification;
    root["remaining"] = window->remaining;
    root["sidebarWidth"] = window->getSidebarWidth();
    root["controlsCenterX"] = window->getControlsCenterPos().x();
    root["controlsCenterY"] = window->getControlsCenterPos().y();
    root["showAll"] = true; //!window->ui->hideFilesButton->isChecked();
    root["screenshotDialog"] = window->screenshotDialog;
    root["maxRecent"] = window->maxRecent;
    root["lang"] = window->lang;
    root["gestures"] = window->gestures;
    root["resume"] = window->resume;
    root["version"] = version;

    QJsonArray recent_json;
    for (auto &entry : window->recent) {
        QJsonObject recent_sub_object_json;
        recent_sub_object_json["path"] = QDir::fromNativeSeparators(entry.path);
        if (entry.title != QString())
            recent_sub_object_json["title"] = entry.title;
        if (entry.time > 0)
            recent_sub_object_json["time"] = entry.time;
        recent_json.append(recent_sub_object_json);
    }
    root["recent"] = recent_json;

    QJsonObject input_json;
    for (auto input_iter = input.begin(); input_iter != input.end(); ++input_iter) {
        auto default_iter = default_input.find(input_iter.key());
        if (default_iter != default_input.end()) {
            if (input_iter->first == default_iter->first &&
               input_iter->second == default_iter->second) // skip entries that are the same as a default_input entry
                continue;
        } else { // not found in defaults
            if (*input_iter == QPair<QString, QString>({QString(), QString()})) // skip empty entries
                continue;
        }
        QJsonArray arr;
        arr.append(input_iter->first);
        arr.append(input_iter->second);
        input_json[input_iter.key()] = arr;
    }
    root["input"] = input_json;

    QJsonObject mpv_json = root["mpv"].toObject();
    mpv_json["volume"] = mpv->volume;
    mpv_json["speed"] = mpv->speed;
    mpv_json["vo"] = mpv->vo;
    mpv_json["screenshot-format"] = mpv->screenshotFormat;
    mpv_json["screenshot-template"] = mpv->screenshotTemplate;
    mpv_json["screenshot-directory"] = QDir::fromNativeSeparators(mpv->screenshotDir);
    mpv_json["msg-level"] = mpv->msgLevel;
    root["mpv"] = mpv_json;

    QJsonArray disable_list_json;
    for (auto &entry : pluginManager->getDisableList())
        disable_list_json.append(entry);
    root["disabledPlugins"] = disable_list_json;

    root["repeat"] = window->getRepeatType();

    settings->setRoot(root);
    settings->save();
}
