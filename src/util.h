
#ifndef UTIL_H
#define UTIL_H

#include <QWidget>
#include <QString>
#include <QTextStream>
#include <QMainWindow>
#include <QDir>
#include "recent.h"

class Settings;

namespace Util {

void srand();
int randInt(int low, int high);

// platform specific
QString versionFileUrl();
QString downloadFileUrl();

bool isDimLightsSupported();
void initWindow(QMainWindow *main);
void setAlwaysOnTop(QMainWindow *main, bool);
void setAspectRatio(QMainWindow *main, int w, int h);
void enableScreenSaver(bool);

QString path(QString dir, QString file);
QString ensureDirExists(QString dir);
QString configDir();
QString dataDir();
QString appDataDir();
QString settingsPath();
QString translationsPath();
QString scriptsPath();
QList<QString> pluginsPaths();

bool isValidFile(QString path);
bool isValidLocation(QString loc); // combined file and url
bool isValidUrl(QString url);
QString toLocalFile(QString s);
void showInFolder(QString path, QString file);

QString defaultFont();
QString monospaceFont();

// mac only
void setWantsLayer(QWidget *widget, bool wants);
void setLayerOpaque(QWidget *widget, bool opaque);
void setLayerOpacity(QWidget *widget, double opacity);
void setLayerBackgroundColor(QWidget *widget, double r, double g, double b, double a);
void setLayerCornerRadius(QWidget *widget, double r);
void setCanDrawSubviewsIntoLayer(QWidget *widget);


QString formatTime(double time, double totalTime);
QString formatRelativeTime(double time);
QString formatNumber(int val, int length);
QString formatNumberWithAmpersand(int val, int length);
QString humanSize(qint64);
QString formatPath(const Recent &recent);
QStringList toNativeSeparators(QStringList list);
QStringList fromNativeSeparators(QStringList list);
int gcd(int v, int u);
QString ratio(int w, int h);

QString getLangName(QString code);
QString getCharEncodingTitle(QString name);
const QList<QPair<QString, QString>> &getAllCharEncodings();

QString toUnicode(const QByteArray &bytes, const char *codec = nullptr);

}

inline QTextStream &qStdout()
{
    static QTextStream r{stdout};
    return r;
}

#endif // UTIL_H
