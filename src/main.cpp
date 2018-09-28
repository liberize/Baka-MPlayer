#include "ui/mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QString>

#include <locale.h>

#include "util.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN)
    FreeConsole();
#endif
    QApplication a(argc, argv);
    setlocale(LC_NUMERIC, "C"); // for mpv

    Util::srand();

    MainWindow w;
    w.show();

    // parse command line
    QStringList args = QApplication::arguments();
    QStringList::iterator arg = args.begin();
    if (++arg != args.end())
        w.load(*arg);
    else
        w.load();

    return a.exec();
}
