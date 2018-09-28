
#ifndef UTIL_H
#define UTIL_H

#include <QWidget>
#include <QString>
#include <QTextStream>
#include <QMainWindow>
#include "recent.h"

class Settings;

namespace Util {

// platform specific
QString VersionFileUrl();
QString DownloadFileUrl();

bool DimLightsSupported();
void SetAlwaysOnTop(QMainWindow *main, bool);
void SetAspectRatio(QMainWindow *main, int w, int h);
QString SettingsLocation();

bool IsValidFile(QString path);
bool IsValidLocation(QString loc); // combined file and url

void ShowInFolder(QString path, QString file);

QString MonospaceFont();

// common
bool IsValidUrl(QString url);

QString FormatTime(int time, int totalTime);
QString FormatRelativeTime(int time);
QString FormatNumber(int val, int length);
QString FormatNumberWithAmpersand(int val, int length);
QString HumanSize(qint64);
QString ShortenPathToParent(const Recent &recent);
QStringList ToNativeSeparators(QStringList list);
QStringList FromNativeSeparators(QStringList list);
int GCD(int v, int u);
QString Ratio(int w, int h);

}

inline QTextStream& qStdout()
{
    static QTextStream r{stdout};
    return r;
}

#endif // UTIL_H
