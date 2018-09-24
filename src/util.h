
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

void RandSeed();
int RandInt(int low, int high);

// platform specific
QString VersionFileUrl();
QString DownloadFileUrl();

bool DimLightsSupported();
void InitWindow(QMainWindow *main);
void SetAlwaysOnTop(QMainWindow *main, bool);
void SetAspectRatio(QMainWindow *main, int w, int h);
void EnableScreenSaver(bool);

QString Path(QString dir, QString file);
QString EnsureDirExists(QString dir);
QString ConfigDir();
QString DataDir();
QString AppDataDir();
QString SettingsPath();
QString TranslationsPath();
QString ScriptsPath();
QList<QString> PluginsPaths();

bool IsValidFile(QString path);
bool IsValidLocation(QString loc); // combined file and url
bool IsValidUrl(QString url);
QString ToLocalFile(QString s);
void ShowInFolder(QString path, QString file);

QString MonospaceFont();

// mac only
void SetWantsLayer(QWidget *widget, bool wants);
void SetLayerOpaque(QWidget *widget, bool opaque);
void SetLayerOpacity(QWidget *widget, double opacity);
void SetLayerBackgroundColor(QWidget *widget, double r, double g, double b, double a);
void SetLayerCornerRadius(QWidget *widget, double r);
void SetCanDrawSubviewsIntoLayer(QWidget *widget);


QString FormatTime(double time, double totalTime);
QString FormatRelativeTime(double time);
QString FormatNumber(int val, int length);
QString FormatNumberWithAmpersand(int val, int length);
QString HumanSize(qint64);
QString ShortenPathToParent(const Recent &recent);
QStringList ToNativeSeparators(QStringList list);
QStringList FromNativeSeparators(QStringList list);
int GCD(int v, int u);
QString Ratio(int w, int h);

QString GetLangName(QString code);
QString GetCharEncodingTitle(QString name);
const QList<QPair<QString, QString> > &GetAllCharEncodings();

QString DetectCharEncoding(const QByteArray &bytes);

}

inline QTextStream& qStdout()
{
    static QTextStream r{stdout};
    return r;
}

#endif // UTIL_H
