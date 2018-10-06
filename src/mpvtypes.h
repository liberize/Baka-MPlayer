#ifndef MPVTYPES_H
#define MPVTYPES_H

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>

namespace Mpv
{
    // filetypes supported by mpv: https://github.com/mpv-player/mpv/blob/master/player/external_files.c
    const QStringList AUDIO_FILE_TYPES = {
        "*.mp3","*.ogg","*.wav","*.wma","*.m4a","*.aac","*.ac3","*.ape",
        "*.flac","*.ra","*.mka","*.dts","*.opus"
    };
    const QStringList VIDEO_FILE_TYPES = {
        "*.avi","*.divx","*.mpg","*.mpeg","*.m1v","*.m2v","*.mpv","*.dv",
        "*.3gp","*.mov","*.mp4","*.m4v","*.mqv","*.dat","*.vcd","*.ogm",
        "*.ogv","*.asf","*.wmv","*.vob","*.mkv","*.ram","*.flv","*.rm",
        "*.ts","*.rmvb","*.dvr-ms","*.m2t","*.m2ts","*.rec","*.f4v","*.hdmov",
        "*.webm","*.vp8","*.letv","*.hlv","*.mts"
    };
    const QStringList MEDIA_FILE_TYPES = AUDIO_FILE_TYPES + VIDEO_FILE_TYPES;
    const QStringList SUBTITLE_FILE_TYPES = {
        "*.sub","*.srt","*.ass","*.ssa","*.smi","*.rt","*.txt","*.mks",
        "*.vtt","*.sup"
    };

    enum PlayState {
        // this number scheme is set so we know all playStates greater than 0 mean the video is in play
        Idle = -1,
        Started = 1,
        Loaded = 2,
        Playing = 3,
        Paused = 4,
        Stopped = -2
    };

    struct Chapter {
        QString title;
        double time;
    };
    struct Track {
        int id;
        QString type;
        int srcId;
        QString title;
        QString lang;
        unsigned albumArt : 1;
        unsigned _default : 1;
        unsigned external : 1;
        QString externalFileName;
        QString codec;
        int demuxW;
        int demuxH;
        double demuxFps;
        int demuxChannelCount;
        int demuxSampleRate;
        QString decoderDesc;

        bool operator==(const Track &t) { return (id == t.id); }
    };
    struct VideoParams {
        QString codec;
        int width = 0;
        int height = 0;
        int dwidth = 0;
        int dheight = 0;
    };
    struct AudioParams {
        QString codec;
        int sampleRate;
        int channels;
    };

    struct FileInfo {
        QString mediaTitle;
        double length = 0;
        int64_t size = 0;
        QMap<QString, QString> metadata;
        VideoParams videoParams;
        AudioParams audioParams;
        QList<Track> tracks; // audio, video, and subs
        QList<Chapter> chapters;
    };

    struct AudioDevice {
        QString name;
        QString description;
    };

    struct PlaylistItem {
        QString name;
        QString path;
        QMap<QString, QString> options;
        bool local;
        bool playing;
    };
}
Q_DECLARE_METATYPE(Mpv::PlayState) // so we can pass it with signals & slots
Q_DECLARE_METATYPE(Mpv::Chapter)
Q_DECLARE_METATYPE(Mpv::Track)
Q_DECLARE_METATYPE(Mpv::VideoParams)
Q_DECLARE_METATYPE(Mpv::AudioParams)
Q_DECLARE_METATYPE(Mpv::FileInfo)
Q_DECLARE_METATYPE(Mpv::PlaylistItem*)

#endif // MPVTYPES_H
