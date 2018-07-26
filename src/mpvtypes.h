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
    const QStringList audio_filetypes = {
        "*.mp3","*.ogg","*.wav","*.wma","*.m4a","*.aac","*.ac3","*.ape",
        "*.flac","*.ra","*.mka","*.dts","*.opus"
    };
    const QStringList video_filetypes = {
        "*.avi","*.divx","*.mpg","*.mpeg","*.m1v","*.m2v","*.mpv","*.dv",
        "*.3gp","*.mov","*.mp4","*.m4v","*.mqv","*.dat","*.vcd","*.ogm",
        "*.ogv","*.asf","*.wmv","*.vob","*.mkv","*.ram","*.flv","*.rm",
        "*.ts","*.rmvb","*.dvr-ms","*.m2t","*.m2ts","*.rec","*.f4v","*.hdmov",
        "*.webm","*.vp8","*.letv","*.hlv","*.mts"
    };
    const QStringList media_filetypes = audio_filetypes + video_filetypes;
    const QStringList subtitle_filetypes = {
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
        int time;
    };
    struct Track {
        int id;
        QString type;
        int src_id;
        QString title;
        QString lang;
        unsigned albumart : 1;
        unsigned _default : 1;
        unsigned external : 1;
        QString external_filename;
        QString codec;
        int demux_w;
        int demux_h;
        double demux_fps;
        int demux_channel_count;
        int demux_samplerate;
        QString decoder_desc;

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
        int samplerate;
        int channels;
    };

    struct FileInfo {
        QString media_title;
        int length = 0;
        QMap<QString, QString> metadata;
        VideoParams video_params;
        AudioParams audio_params;
        QList<Track> tracks; // audio, video, and subs
        QList<Chapter> chapters;
    };

    struct AudioDevice {
        QString name;
        QString description;
    };
}
Q_DECLARE_METATYPE(Mpv::PlayState) // so we can pass it with signals & slots
Q_DECLARE_METATYPE(Mpv::Chapter)
Q_DECLARE_METATYPE(Mpv::Track)
Q_DECLARE_METATYPE(Mpv::VideoParams)
Q_DECLARE_METATYPE(Mpv::AudioParams)
Q_DECLARE_METATYPE(Mpv::FileInfo)


#endif // MPVTYPES_H
