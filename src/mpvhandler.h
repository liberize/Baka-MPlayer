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

#define MPV_REPLY_COMMAND 1
#define MPV_REPLY_PROPERTY 2

class BakaEngine;
class MpvWidget;

class MpvHandler : public QObject {
    friend class BakaEngine;
    Q_OBJECT
public:
    explicit MpvHandler(QWidget *container, QObject *parent = 0);
    ~MpvHandler();

    void Initialize();
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

    int toScaledFontSize(int size)          { return fileInfo.video_params.dheight ? size * 720 / fileInfo.video_params.dheight : size; }
    int fromScaledFontSize(int size)        { return fileInfo.video_params.dheight ? size * fileInfo.video_params.dheight / 720 : size; }
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

    bool FileExists(QString);

public slots:
    void LoadFile(QString, QString = "", const QMap<QString, QString> & = QMap<QString, QString>());
    QString LoadPlaylist(QString);
    bool PlayFile(QString, QString = "", const QMap<QString, QString> & = QMap<QString, QString>());

    void AddOverlay(int id, int x, int y, QString file, int offset, int w, int h);
    void RemoveOverlay(int id);

    void Play();
    void Pause();
    void RestartPaused();
    void PlayPause();
    void Restart();
    void Rewind();
    void Stop();
    void Mute(bool);

    void Seek(double pos, bool relative = false, bool osd = false);
    double Relative(double pos);
    void FrameStep();
    void FrameBackStep();

    void Chapter(int);
    void NextChapter();
    void PreviousChapter();

    void Volume(int, bool osd = false);
    void AudioDelay(double);
    void SubtitleDelay(double);
    void Speed(double);
    void Aspect(QString);
    void Vid(int);
    void Aid(int);
    void Sid(int);

    void Screenshot(bool withSubs = false);

    void ScreenshotFormat(QString);
    void ScreenshotTemplate(QString);
    void ScreenshotDirectory(QString);

    void AddSubtitleTrack(QString);
    void AddAudioTrack(QString);
    void ShowSubtitles(bool);
    void SubtitleScale(double scale, bool relative = false);

    void Deinterlace(bool);
    void Interpolate(bool);
    void Vo(QString);

    void MsgLevel(QString level);

    void ShowText(QString text, int duration = 4000);

    void LoadTracks();
    void LoadChapters();
    void LoadVideoParams();
    void LoadAudioParams();
    void LoadMetadata();
    void LoadOsdSize();

    void Command(const QStringList &strlist);
    void SetOption(QString key, QString val);

    void LoadAudioDevices();
    void AudioDevice(QString name);
    void LoadSubtitleEncodings();
    void SubtitleEncoding(QString encoding);
    void SubtitleFont(const QFont &font);
    void SubtitleColor(const QColor &color);
    void SubtitleBackColor(const QColor &color);
    void SubtitleBlur(double factor);
    void SubtitleShadowOffset(int size);
    void SubtitleShadowColor(const QColor &color);

    void SetFileLocalOptions();

protected slots:
    void OpenFile(QString);
    void LoadFileInfo();
    void SetProperties();

    void AsyncCommand(const char *args[]);
    void Command(const char *args[]);
    void HandleErrorCode(int);

private slots:
    void setFileInfo()                      { emit fileInfoChanged(fileInfo); }
    void setPlayState(Mpv::PlayState s)     { emit playStateChanged(playState = s); }
    void setFile(QString s, QString t, const QMap<QString, QString> &opts) { emit fileChanged(file = s, t, fileLocalOptions = opts); }
    void setPath(QString s)                 { if (path != s) emit pathChanged(path = s); }
    void setScreenshotFormat(QString s)     { emit screenshotFormatChanged(screenshotFormat = s); }
    void setScreenshotTemplate(QString s)   { emit screenshotTemplateChanged(screenshotTemplate = s); }
    void setScreenshotDir(QString s)        { emit screenshotDirChanged(screenshotDir = s); }
    void setVo(QString s)                   { emit voChanged(vo = s); }
    void setMsgLevel(QString s)             { emit msgLevelChanged(msgLevel = s); }
    void setAudioDelay(double d)            { emit audioDelayChanged(audioDelay = d); }
    void setSubtitleDelay(double d)         { emit subtitleDelayChanged(subtitleDelay = d); }
    void setSpeed(double d)                 { emit speedChanged(speed = d); }
    void setTime(double i)                  { emit timeChanged(time = i); }
    void setVolume(int i)                   { emit volumeChanged(volume = i); }
    void setIndex(int i)                    { emit indexChanged(index = i); }
    void setVid(int i)                      { emit vidChanged(vid = i); }
    void setAid(int i)                      { emit aidChanged(aid = i); }
    void setSid(int i)                      { emit sidChanged(sid = i); }
    void setSubtitleVisibility(bool b)      { emit subtitleVisibilityChanged(subtitleVisibility = b); }
    void setMute(bool b)                    { if (mute != b) emit muteChanged(mute = b); }

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
