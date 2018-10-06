#ifndef BAKAENGINE_H
#define BAKAENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTranslator>
#include <QTemporaryDir>

class MainWindow;
class MpvHandler;
class Settings;
class GestureHandler;
class OverlayHandler;
class UpdateManager;
class DimDialog;
class PluginManager;
class RequestManager;
class MtspMessageHandler;

class BakaEngine : public QObject {
    Q_OBJECT
public:
    explicit BakaEngine(QObject *parent = 0);
    ~BakaEngine();

    MainWindow     *window;
    MpvHandler     *mpv;
    Settings       *settings;
    GestureHandler *gesture;
    OverlayHandler *overlay;
    UpdateManager  *update;
    DimDialog      *dimDialog;
    QTemporaryDir  *tempDir;
    PluginManager  *pluginManager;
    RequestManager *requestManager;
    MtspMessageHandler *mtspMessageHandler;

    QSystemTrayIcon *sysTrayIcon;
    QMenu           *trayIconMenu;

    QTranslator     *translator,
                    *qtTranslator;

    // input hash-table provides O(1) input-command lookups
    QHash<QString, QPair<QString, QString>> input; // [shortcut] = QPair<command, comment>

    // the following are the default input bindings
    // they are loaded into the input before parsing the settings file
    // when saving the settings file we don't save things that appear here
    const QHash<QString, QPair<QString, QString>> default_input = {
        {"Ctrl+Alt++",      {"mpv add sub-scale +0.1", tr("Increase subtitle size")}},
        {"Ctrl+Alt+-",      {"mpv add sub-scale -0.1", tr("Decrease subtitle size")}},
        {"Ctrl+Alt+Up",     {"mpv add sub-pos -1", tr("Move subtitle up")}},
        {"Ctrl+Alt+Down",   {"mpv add sub-pos +1", tr("Move subtitle down")}},
        {"Ctrl+W",          {"mpv cycle sub-visibility", tr("Toggle subtitle visibility")}},
        {"Ctrl+R",          {"mpv set time-pos 0", tr("Restart playback")}},
        {"PgDown",          {"mpv add chapter +1", tr("Go to next chapter")}},
        {"PgUp",            {"mpv add chapter -1", tr("Go to previous chapter")}},
        {"Right",           {"mpv seek +5", tr("Seek forwards by %0 sec").arg("5")}},
        {"Left",            {"mpv seek -5", tr("Seek backwards by %0 sec").arg("5")}},
        {"Shift+Left",      {"mpv frame_back_step", tr("Frame step backwards")}},
        {"Shift+Right",     {"mpv frame_step", tr("Frame step")}},
        {"Ctrl+M",          {"mute", tr("Toggle mute audio")}},
        {"Ctrl+T",          {"screenshot subtitles", tr("Take screenshot with subtitles")}},
        {"Ctrl+Shift+T",    {"screenshot", tr("Take screenshot without subtitles")}},
        {"Down",            {"volume -5", tr("Decrease volume")}},
        {"Up",              {"volume +5", tr("Increase volume")}},
        {"Ctrl+Shift+Up",   {"speed +0.1", tr("Increase playback speed by %0%").arg("10")}},
        {"Ctrl+Shift+Down", {"speed -0.1", tr("Decrease playback speed by %0%").arg("10")}},
        {"Ctrl+]",          {"speed 2.0", tr("Increase playback speed to 2.0x")}},
        {"Ctrl+[",          {"speed 0.5", tr("Decrease playback speed to 0.5x")}},
        {"Ctrl+Shift+R",    {"speed 1.0", tr("Reset speed")}},
        {"Return",          {"fullscreen", tr("Toggle fullscreen")}},
        {"Ctrl+D",          {"dim", tr("Dim lights")}},
        {"Ctrl+E",          {"show_in_folder", tr("Show the file in its folder")}},
        {"Tab",             {"media_info", tr("View media information")}},
        {"Ctrl+J",          {"jump", tr("Show jump to time dialog")}},
        {"Ctrl+N",          {"new", tr("Open a new window")}},
        {"Ctrl+O",          {"open", tr("Show open file dialog")}},
        {"Ctrl+Q",          {"quit", tr("Quit")}},
        {"Ctrl+Right",      {"playlist play +1", tr("Play next file")}},
        {"Ctrl+Left",       {"playlist play -1", tr("Play previous file")}},
        {"Ctrl+S",          {"stop", tr("Stop playback")}},
        {"Ctrl+U",          {"open_location", tr("Show location dialog")}},
        {"Ctrl+V",          {"open_clipboard", tr("Open clipboard location")}},
        {"Alt+P",           {"playlist toggle", tr("Toggle playlist visibility")}},
        {"Alt+O",           {"library toggle", tr("Toggle library visibility")}},
        {"Ctrl+Z",          {"open_recent 0", tr("Open the last played file")}},
        {"Ctrl+`",          {"output", tr("Access command-line")}},
        {"F1",              {"online_help", tr("Launch online help")}},
        {"Space",           {"play_pause", tr("Play/Pause")}},
        {"Alt+1",           {"video_size", tr("Set video size to fit screen")}},
        {"Alt+2",           {"video_size 50", tr("Set video size to %0%").arg("50")}},
        {"Alt+3",           {"video_size 75", tr("Set video size to %0%").arg("75")}},
        {"Alt+4",           {"video_size 100", tr("Set video size to %0%").arg("100")}},
        {"Alt+5",           {"video_size 150", tr("Set video size to %0%").arg("150")}},
        {"Alt+6",           {"video_size 200", tr("Set video size to %0%").arg("200")}},
        {"Esc",             {"boss", tr("Boss key")}},
        {"Ctrl+Down",       {"playlist select +1", tr("Select next file on playlist")}},
        {"Ctrl+Up",         {"playlist select -1", tr("Select previous file on playlist")}},
        {"Ctrl+Return",     {"playlist play", tr("Play selected file on playlist")}},
        {"Ctrl+Del",        {"playlist remove", tr("Remove selected file from playlist")}}
    };

public slots:
    void loadSettings();
    void saveSettings();
    void loadPlugins();

    void command(QString cmd);

protected slots:
    // Utility functions
    void print(QString what, QString who = "baka");
    void println(QString what, QString who = "baka");
    void invalidCommand(QString);
    void invalidParameter(QString);
    void requiresParameters(QString);

    // Settings Loading
    void load2_0_3();

private:
    // This is a baka-command hashtable initialized below
    //  by using a hash-table -> function pointer we acheive O(1) function lookups
    // Format: void BakaCommand(QStringList args)
    // See bakacommands.cpp for function definitions

    // todo: write advanced information about commands
    typedef void(BakaEngine::*BakaCommandFPtr)(QStringList&);
    const QHash<QString, QPair<BakaCommandFPtr, QStringList>> BakaCommandMap = {
        {"mpv",            {&BakaEngine::bakaMpv, {QString(), tr("executes mpv command"), QString()}}},
        {"sh",             {&BakaEngine::bakaSh, {QString(), tr("executes system shell command"), QString()}}},
        {"new",            {&BakaEngine::bakaNew, {QString(), tr("creates a new instance of upv"), QString()}}},
        {"open_location",  {&BakaEngine::bakaOpenLocation, {QString(), tr("shows the open location dialog"), QString()}}},
        {"open_clipboard", {&BakaEngine::bakaOpenClipboard, {QString(), tr("opens the clipboard"), QString()}}},
        {"close",          {&BakaEngine::bakaClose, {QString(), tr("close file"), QString()}}},
        {"show_in_folder", {&BakaEngine::bakaShowInFolder, {QString(), tr("shows the current file in folder"), QString()}}},
        {"add_subtitles",  {&BakaEngine::bakaAddSubtitles, {QString(), tr("add subtitles dialog"), QString()}}},
        {"add_audio",      {&BakaEngine::bakaAddAudio, {QString(), tr("add audio track dialog"), QString()}}},
        {"screenshot",     {&BakaEngine::bakaScreenshot, {tr("[subs]"), tr("take a screenshot (with subtitles if specified)"), QString()}}},
        {"media_info",     {&BakaEngine::bakaMediaInfo, {QString(), tr("toggles media info display"), QString()}}},
        {"stop",           {&BakaEngine::bakaStop, {QString(), tr("stops the current playback"), QString()}}},
        {"playlist",       {&BakaEngine::bakaPlaylist, {"[...]", tr("playlist options"), QString()}}},
        {"library",        {&BakaEngine::bakaLibrary, {"[...]", tr("library options"), QString()}}},
        {"jump",           {&BakaEngine::bakaJump, {QString(), tr("opens jump dialog"), QString()}}},
        {"dim",            {&BakaEngine::bakaDim, {QString(), tr("toggles dim desktop"), QString()}}},
        {"preferences",    {&BakaEngine::bakaPreferences, {QString(), tr("opens preferences dialog"), QString()}}},
        {"online_help",    {&BakaEngine::bakaOnlineHelp, {QString(), tr("launches online help"), QString()}}},
        {"update",         {&BakaEngine::bakaUpdate, {QString(), tr("opens the update dialog or updates youtube-dl"), QString()}}},
        {"open",           {&BakaEngine::bakaOpen, {tr("[file]"), tr("opens the open file dialog or the file specified"), QString()}}},
        {"play_pause",     {&BakaEngine::bakaPlayPause, {QString(), tr("toggle play/pause state"), QString()}}},
        {"video_size",     {&BakaEngine::bakaVideoSize, {tr("[percent]"), tr("set video size"), QString()}}},
        {"deinterlace",    {&BakaEngine::bakaDeinterlace, {QString(), tr("toggle deinterlace"), QString()}}},
        {"interpolate",    {&BakaEngine::bakaInterpolate, {QString(), tr("toggle motion interpolation"), QString()}}},
        {"mute",           {&BakaEngine::bakaMute, {QString(), tr("mutes the audio"), QString()},}},
        {"volume",         {&BakaEngine::bakaVolume, {tr("[level]"), tr("adjusts the volume"), QString()}}},
        {"audio_delay",    {&BakaEngine::bakaAudioDelay, {tr("[level]"), tr("adjusts audio delay"), QString()}}},
        {"subtitle_delay", {&BakaEngine::bakaSubtitleDelay, {tr("[level]"), tr("adjusts subtitle delay"), QString()}}},
        {"subtitle_font",  {&BakaEngine::bakaSubtitleFont, {tr("[level]"), tr("change subtitle font"), QString()}}},
        {"subtitle_style", {&BakaEngine::bakaSubtitleStyle, {tr("[level]"), tr("change subtitle style"), QString()}}},
        {"speed",          {&BakaEngine::bakaSpeed, {tr("[ratio]"), tr("adjusts the speed"), QString()}}},
        {"fullscreen",     {&BakaEngine::bakaFullScreen, {QString(), tr("toggles fullscreen state"), QString()}}},
        {"boss",           {&BakaEngine::bakaBoss, {QString(), tr("pause and hide the window"), QString()}}},
        {"help",           {&BakaEngine::bakaHelp, {tr("[command]"), tr("internal help menu"), QString()}}},
        {"about",          {&BakaEngine::bakaAbout, {tr("[qt]"), tr("open about dialog"), QString()}}},
        {"msg_level",      {&BakaEngine::bakaMsgLevel, {tr("[level]"), tr("set mpv msg-level"), QString()}}},
        {"quit",           {&BakaEngine::bakaQuit, {QString(), tr("quit upv"), QString()}}}
    };
    // Baka Command Functions
    void bakaMpv(QStringList&);
    void bakaSh(QStringList&);
    void bakaNew(QStringList&);
    void bakaOpenLocation(QStringList&);
    void bakaOpenClipboard(QStringList&);
    void bakaClose(QStringList&);
    void bakaShowInFolder(QStringList&);
    void bakaAddSubtitles(QStringList&);
    void bakaAddAudio(QStringList&);
    void bakaScreenshot(QStringList&);
    void bakaMediaInfo(QStringList&);
    void bakaStop(QStringList&);
    void bakaPlaylist(QStringList&);
    void bakaLibrary(QStringList&);
    void bakaJump(QStringList&);
    void bakaDim(QStringList&);
    void bakaPreferences(QStringList&);
    void bakaOnlineHelp(QStringList&);
    void bakaUpdate(QStringList&);
    void bakaOpen(QStringList&);
    void bakaPlayPause(QStringList&);
    void bakaVideoSize(QStringList&);
    void bakaAspect(QStringList&);
    void bakaDeinterlace(QStringList&);
    void bakaInterpolate(QStringList&);
    void bakaMute(QStringList&);
    void bakaVolume(QStringList&);
    void bakaAudioDelay(QStringList&);
    void bakaSubtitleDelay(QStringList&);
    void bakaSubtitleFont(QStringList&);
    void bakaSubtitleStyle(QStringList&);
    void bakaSpeed(QStringList&);
    void bakaFullScreen(QStringList&);
    void bakaBoss(QStringList&);
    void bakaHelp(QStringList&);
    void bakaAbout(QStringList&);
    void bakaMsgLevel(QStringList&);
    void bakaQuit(QStringList&);
public slots:
    void open();
    void openLocation();
    void screenshot(bool subs);
    void mediaInfo(bool show);
    void playPause();
    void jump();
    void fitWindow(int percent = 0, bool msg = true);
    void dim(bool dim);
    void about(QString what = QString());
    void quit();
};

#endif // BAKAENGINE_H
