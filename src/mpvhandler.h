#ifndef MPVHANDLER_H
#define MPVHANDLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>

#include <mpv/client.h>
#include <mpv/qthelper.hpp>
#include <mpv/render.h>
#include <mpv/render_gl.h>

#include "mpvtypes.h"

class BakaEngine;
class MpvWidget;

class MpvHandler : public QObject {
    friend class BakaEngine;
    Q_OBJECT

public:
    explicit MpvHandler(QWidget *container, QObject *parent = 0);
    ~MpvHandler();

    void initialize();

    const Mpv::FileInfo &getFileInfo()      { return fileInfo; }
    Mpv::PlayState getPlayState()           { return playState; }
    QString getFile()                       { return file; }
    QString getPath()                       { return path; }
    const QMap<QString, QString> &getFileLocalOptions() { return fileLocalOptions; }

    QString getScreenshotFormat()           { return screenshotFormat; }
    QString getScreenshotTemplate()         { return screenshotTemplate; }
    QString getScreenshotDir()              { return screenshotDir; }
    QString getVo()                         { return vo; }
    QString getMsgLevel()                   { return msgLevel; }
    double getAudioDelay()                  { return audioDelay; }
    double getSubtitleDelay()               { return subtitleDelay; }
    double getSpeed()                       { return speed; }
    double getTime()                        { return time; }
    int getVolume()                         { return volume; }
    int getVid()                            { return vid; }
    int getAid()                            { return aid; }
    int getSid()                            { return sid; }
    bool getSubtitleVisibility()            { return subtitleVisibility; }
    bool getMute()                          { return mute; }
    bool hasVideo()                         { return hasVideoTrack; }

    int getOsdWidth()                       { return osdWidth; }
    int getOsdHeight()                      { return osdHeight; }

    int toScaledFontSize(int size)          { return fileInfo.videoParams.dheight ? size * 720 / fileInfo.videoParams.dheight : size; }
    int fromScaledFontSize(int size)        { return fileInfo.videoParams.dheight ? size * fileInfo.videoParams.dheight / 720 : size; }
    QColor fromColorString(QString colorStr);
    QString toColorString(const QColor &color);

    QString getMediaInfo();
    int64_t getCacheSize();
    double getCacheTime();

    QWidget *getWidget();
    mpv_render_context *createRenderContext(mpv_render_param *params);
    void destroyRenderContext(mpv_render_context *render);

    QString formatTrackInfo(const Mpv::Track &track);

    QString getAudioDevice();
    QString getSubtitleEncoding();
    QFont getSubtitleFont();
    QColor getSubtitleColor();
    QColor getSubtitleBackColor();
    QColor getSubtitleShadowColor();

protected:
    virtual bool event(QEvent*);

public slots:
    void loadFile(QString, QString = "", const QMap<QString, QString> & = QMap<QString, QString>());
    QString loadPlaylist(QString);
    bool playFile(QString, QString = "", const QMap<QString, QString> & = QMap<QString, QString>());

    void addOverlay(int id, int x, int y, QString file, int offset, int w, int h);
    void removeOverlay(int id);

    void play();
    void pause();
    void restartPaused();
    void playPause();
    void restart();
    void rewind();
    void stop();
    void setMute(bool);

    void seek(double pos, bool relative = false, bool osd = false);
    double relative(double pos);
    void frameStep();
    void frameBackStep();

    void setChapter(int);
    void nextChapter();
    void previousChapter();

    void setVolume(int, bool osd = false);
    void setAudioDelay(double);
    void setSubtitleDelay(double);
    void setSpeed(double);
    void setAspect(QString);
    void setVid(int);
    void setAid(int);
    void setSid(int);

    void screenshot(bool withSubs = false);

    void setScreenshotFormat(QString);
    void setScreenshotTemplate(QString);
    void setScreenshotDirectory(QString);

    void addSubtitleTrack(QString);
    void addAudioTrack(QString);
    void showSubtitles(bool);
    void setSubtitleScale(double scale, bool relative = false);

    void setDeinterlace(bool);
    void setInterpolate(bool);
    void setVo(QString);

    void setMsgLevel(QString level);

    void showText(QString text, int duration = 4000);

    void loadTracks();
    void loadChapters();
    void loadVideoParams();
    void loadAudioParams();
    void loadMetadata();
    void loadOsdSize();
    void loadAudioDevices();
    void loadSubtitleEncodings();

    void command(const QStringList &strlist);
    void mpvSetOption(QString key, QString val);

    void setAudioDevice(QString name);
    void setSubtitleEncoding(QString encoding);
    void setSubtitleFont(const QFont &font);
    void setSubtitleColor(const QColor &color);
    void setSubtitleBackColor(const QColor &color);
    void setSubtitleBlur(double factor);
    void setSubtitleShadowOffset(int size);
    void setSubtitleShadowColor(const QColor &color);

protected slots:
    void openFile(QString);
    void loadFileInfo();
    void setProperties();

    void mpvCommandAsync(const char *args[]);
    void mpvCommand(const char *args[]);
    void handleErrorCode(int);

    bool fileExists(QString);
    void setFileLocalOptions();

private slots:
    void updateFileInfo()                      { emit fileInfoChanged(fileInfo); }
    void updatePlayState(Mpv::PlayState s)     { emit playStateChanged(playState = s); }
    void updateFile(QString s, QString t, const QMap<QString, QString> &opts) { emit fileChanged(file = s, t, fileLocalOptions = opts); }
    void updatePath(QString s)                 { if (path != s) emit pathChanged(path = s); }
    void updateScreenshotFormat(QString s)     { emit screenshotFormatChanged(screenshotFormat = s); }
    void updateScreenshotTemplate(QString s)   { emit screenshotTemplateChanged(screenshotTemplate = s); }
    void updateScreenshotDir(QString s)        { emit screenshotDirChanged(screenshotDir = s); }
    void updateVo(QString s)                   { emit voChanged(vo = s); }
    void updateMsgLevel(QString s)             { emit msgLevelChanged(msgLevel = s); }
    void updateAudioDelay(double d)            { emit audioDelayChanged(audioDelay = d); }
    void updateSubtitleDelay(double d)         { emit subtitleDelayChanged(subtitleDelay = d); }
    void updateSpeed(double d)                 { emit speedChanged(speed = d); }
    void updateTime(double i)                  { emit timeChanged(time = i); }
    void updateVolume(int i)                   { emit volumeChanged(volume = i); }
    void updateIndex(int i)                    { emit indexChanged(index = i); }
    void updateVid(int i)                      { emit vidChanged(vid = i); }
    void updateAid(int i)                      { emit aidChanged(aid = i); }
    void updateSid(int i)                      { emit sidChanged(sid = i); }
    void updateSubtitleVisibility(bool b)      { emit subtitleVisibilityChanged(subtitleVisibility = b); }
    void updateMute(bool b)                    { if (mute != b) emit muteChanged(mute = b); }

signals:
    void fileInfoChanged(const Mpv::FileInfo&);
    void trackListChanged(const QList<Mpv::Track>&);
    void chaptersChanged(const QList<Mpv::Chapter>&);
    void videoParamsChanged(const Mpv::VideoParams&);
    void audioParamsChanged(const Mpv::AudioParams&);
    void playStateChanged(Mpv::PlayState);
    void fileChanging(double, double);
    void fileChanged(QString, QString, const QMap<QString, QString> &);
    void pathChanged(QString);
    void screenshotFormatChanged(QString);
    void screenshotTemplateChanged(QString);
    void screenshotDirChanged(QString);
    void voChanged(QString);
    void msgLevelChanged(QString);
    void audioDelayChanged(double);
    void subtitleDelayChanged(double);
    void speedChanged(double);
    void timeChanged(double);
    void volumeChanged(int);
    void indexChanged(int);
    void vidChanged(int);
    void aidChanged(int);
    void sidChanged(int);
    void subtitleVisibilityChanged(bool);
    void muteChanged(bool);
    void audioDeviceListChanged(const QList<Mpv::AudioDevice> &);
    void audioDeviceChanged(QString);
    void subtitleEncodingListChanged(const QList<QPair<QString, QString>> &);
    void subtitleEncodingChanged(QString);
    void renderContextCreated();

    void messageSignal(QString m);

private:
    BakaEngine *baka;
    mpv_handle *mpv = nullptr;
    MpvWidget *widget = nullptr;

    // variables
    Mpv::PlayState playState = Mpv::Idle;
    Mpv::FileInfo fileInfo;
    QString file;
    QString path;
    QString screenshotFormat;
    QString screenshotTemplate;
    QString screenshotDir;
    QString suffix;
    QString vo;
    QString msgLevel;
    double  audioDelay = 0;
    double  subtitleDelay = 0;
    double  speed = 1;
    double  time = 0;
    double  lastTime = 0;
    int     volume = 100;
    int     index = 0;
    int     vid;
    int     aid;
    int     sid;
    bool    init = false;
    bool    subtitleVisibility = true;
    bool    mute = false;
    int     osdWidth;
    int     osdHeight;
    bool    readyToRender = false;
    bool    hasVideoTrack = true;
    QImage  defaultAlbumArt;
    QMap<QString, QString> fileLocalOptions;
};

#endif // MPVHANDLER_H
