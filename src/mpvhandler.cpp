#include "mpvhandler.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDateTime>
#include <QVBoxLayout>
#include <QFileInfo>

#include "bakaengine.h"
#include "overlayhandler.h"
#include "util.h"

#ifdef ENABLE_MPV_COCOA_WIDGET
#include "widgets/mpvcocoawidget.h"
#else
#include "widgets/mpvglwidget.h"
#endif

static void wakeup(void *ctx)
{
    MpvHandler *mpvhandler = (MpvHandler*)ctx;
    QCoreApplication::postEvent(mpvhandler, new QEvent(QEvent::User));
}

MpvHandler::MpvHandler(QWidget *container, QObject *parent):
    QObject(parent),
    baka(static_cast<BakaEngine*>(parent)),
    defaultAlbumArt(":/img/album_art.png")
{
    // create mpv
    mpv = mpv_create();
    if (!mpv)
        throw "Could not create mpv object";

    // set mpv options
    mpv_set_option_string(mpv, "vo", "libmpv");
    mpv_set_option_string(mpv, "input-cursor", "no");   // no mouse handling
    mpv_set_option_string(mpv, "cursor-autohide", "no");// no cursor-autohide, we handle that
    mpv_set_option_string(mpv, "ytdl", "yes"); // youtube-dl support
    mpv_set_option_string(mpv, "sub-auto", "fuzzy"); // Automatic subfile detection

    // get updates when these properties change
    mpv_observe_property(mpv, 0, "playback-time", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "sid", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "aid", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "vid", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "sub-visibility", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "core-idle", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "paused-for-cache", MPV_FORMAT_FLAG);

    // setup callback event handling
    mpv_set_wakeup_callback(mpv, wakeup, this);

#ifdef ENABLE_MPV_COCOA_WIDGET
    widget = new MpvCocoaWidget(container);
#else
    widget = new MpvGlWidget(container);
#endif
    widget->setMpvHandler(this);
    widget->self()->show();

    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(widget->self());
}

MpvHandler::~MpvHandler()
{
    if (widget) {
        delete widget;
        widget = nullptr;
    }
    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = nullptr;
    }
}

void MpvHandler::Initialize()
{
    if (mpv_initialize(mpv) < 0)
        throw "Could not initialize mpv";
}

QWidget *MpvHandler::getWidget()
{
    return widget->self();
}

mpv_render_context *MpvHandler::createRenderContext(mpv_render_param *params)
{
    mpv_render_context *render = nullptr;
    if (mpv_render_context_create(&render, mpv, params) < 0)
        throw std::runtime_error("Could not create render context");
    readyToRender = true;
    emit renderContextCreated();
    return render;
}

void MpvHandler::destroyRenderContext(mpv_render_context *render)
{
    readyToRender = false;
    mpv_render_context_set_update_callback(render, nullptr, nullptr);
    mpv_render_context_free(render);
}

QString MpvHandler::formatTrackInfo(const Mpv::Track &track)
{
    QStringList infos;
    infos.append(QString("%0:").arg(QString::number(track.id)));
    if (!track.lang.isEmpty() && track.lang != "und")
        infos.append(Util::GetLangName(track.lang));
    if (!track.title.isEmpty())
        infos.append(track.title);

    QStringList descs;
    QString shortDesc = track.decoder_desc.split('(')[0];
    if (!shortDesc.isEmpty())
        descs.append(shortDesc.remove(' '));
    if (track.type == "video") {
        if (track.demux_w && track.demux_h)
            descs.append(QString("%0x%1").arg(QString::number(track.demux_w), QString::number(track.demux_h)));
        if (track.demux_fps)
            descs.append(QString("%0fps").arg(QString::number(track.demux_fps)));
    } else if (track.type == "audio") {
        if (track.demux_channel_count)
            descs.append(QString("%0ch").arg(QString::number(track.demux_channel_count)));
        if (track.demux_samplerate)
            descs.append(QString("%0kHz").arg(QString::number(track.demux_samplerate / 1000.)));
    }
    QString desc = descs.join(", ");
    if (!desc.isEmpty())
        infos.append(desc);

    if (track._default)
        infos.append(QString("(%0)").arg(tr("Default")));
    return infos.join(" ");
}

void MpvHandler::LoadAudioDevices()
{
    QList<Mpv::AudioDevice> devices;
    mpv_node node;
    mpv_get_property(mpv, "audio-device-list", MPV_FORMAT_NODE, &node);
    if (node.format == MPV_FORMAT_NODE_ARRAY) {
        for (int i = 0; i < node.u.list->num; i++) {
            if (node.u.list->values[i].format == MPV_FORMAT_NODE_MAP) {
                Mpv::AudioDevice device;
                for (int n = 0; n < node.u.list->values[i].u.list->num; n++) {
                    if (QString(node.u.list->values[i].u.list->keys[n]) == "name") {
                        if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_STRING)
                            device.name = node.u.list->values[i].u.list->values[n].u.string;
                    } else if (QString(node.u.list->values[i].u.list->keys[n]) == "description") {
                        if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_STRING)
                            device.description = node.u.list->values[i].u.list->values[n].u.string;
                    }
                }
                devices.push_back(device);
            }
        }
    }
    emit audioDeviceListChanged(devices);

    QString device = getAudioDevice();
    emit audioDeviceChanged(device);
}

QString MpvHandler::getAudioDevice()
{
    return mpv_get_property_string(mpv, "audio-device");
}

void MpvHandler::AudioDevice(QString name)
{
    QByteArray tmp = name.toUtf8();
    HandleErrorCode(mpv_set_property_string(mpv, "audio-device", tmp.constData()));
    ShowText(tr("Audio Device: %0").arg(name));
    emit audioDeviceChanged(name);
}

QString MpvHandler::getSubtitleEncoding()
{
    return mpv_get_property_string(mpv, "sub-codepage");
}

void MpvHandler::LoadSubtitleEncodings()
{
    const auto &encodings = Util::GetAllCharEncodings();
    emit subtitleEncodingListChanged(encodings);

    QString encoding = getSubtitleEncoding();
    emit subtitleEncodingChanged(encoding);
}

void MpvHandler::SubtitleEncoding(QString encoding)
{
    QByteArray tmp = encoding.toUtf8();
    HandleErrorCode(mpv_set_property_string(mpv, "sub-codepage", tmp.constData()));
    ShowText(tr("Subtitle Encoding: %0").arg(encoding));
    emit subtitleEncodingChanged(encoding);
}

QFont MpvHandler::getSubtitleFont()
{
    QFont font;
    font.setFamily(mpv_get_property_string(mpv, "sub-font"));

    int64_t fontSize = 0;
    mpv_get_property(mpv, "sub-font-size", MPV_FORMAT_INT64, &fontSize);
    font.setPixelSize(fromScaledFontSize(fontSize));

    int bold = 0;
    mpv_get_property(mpv, "sub-bold", MPV_FORMAT_FLAG, &bold);
    font.setBold(bold);

    int italic = 0;
    mpv_get_property(mpv, "sub-italic", MPV_FORMAT_FLAG, &italic);
    font.setItalic(italic);

    int64_t spacing = 0;
    mpv_set_property(mpv, "sub-spacing", MPV_FORMAT_INT64, &spacing);
    font.setLetterSpacing(QFont::AbsoluteSpacing, fromScaledFontSize(spacing));

    return font;
}

void MpvHandler::SubtitleFont(const QFont &font)
{
    QByteArray tmp = font.family().toUtf8();
    mpv_set_property_string(mpv, "sub-font", tmp.constData());

    int64_t fontSize = toScaledFontSize(font.pixelSize());
    mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "sub-font-size", MPV_FORMAT_INT64, &fontSize);

    int bold = font.bold();
    mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "sub-bold", MPV_FORMAT_FLAG, &bold);

    int italic = font.italic();
    mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "sub-italic", MPV_FORMAT_FLAG, &italic);

    if (font.letterSpacingType() == QFont::AbsoluteSpacing) {
        int64_t spacing = toScaledFontSize(font.letterSpacing());
        mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "sub-spacing", MPV_FORMAT_INT64, &spacing);
    }
}

QColor MpvHandler::fromColorString(QString colorStr)
{
    QColor color;
    if (colorStr[0] == '#')
        color.setNamedColor(colorStr);
    else {
        QList<qreal> c;
        for (const auto &v : colorStr.split('/'))
            c.push_back(v.toDouble());
        if (c.length() == 4)
            color.setRgbF(c[0], c[1], c[2], c[3]);
        else if (c.length() == 3)
            color.setRgbF(c[0], c[1], c[2]);
        else if (c.length() == 2)
            color.setRgbF(c[0], c[0], c[0], c[1]);
    }
    return color;
}

QString MpvHandler::toColorString(const QColor &color)
{
    return color.name(QColor::HexArgb);
}

QColor MpvHandler::getSubtitleColor()
{
    QString subColor = mpv_get_property_string(mpv, "sub-color");
    return fromColorString(subColor);
}

void MpvHandler::SubtitleColor(const QColor &color)
{
    QString colorStr = toColorString(color);
    QByteArray tmp = colorStr.toUtf8();
    mpv_set_property_string(mpv, "sub-color", tmp.constData());
}

QColor MpvHandler::getSubtitleBackColor()
{
    QString subBackColor = mpv_get_property_string(mpv, "sub-back-color");
    return fromColorString(subBackColor);
}

void MpvHandler::SubtitleBackColor(const QColor &color)
{
    QString colorStr = toColorString(color);
    QByteArray tmp = colorStr.toUtf8();
    mpv_set_property_string(mpv, "sub-back-color", tmp.constData());
}

void MpvHandler::SubtitleBlur(double factor)
{
    mpv_set_property(mpv, "sub-blur", MPV_FORMAT_DOUBLE, &factor);
}

void MpvHandler::SubtitleShadowOffset(int size)
{
    int64_t offset = toScaledFontSize(size);
    mpv_set_property(mpv, "sub-shadow-offset", MPV_FORMAT_INT64, &offset);
}

QColor MpvHandler::getSubtitleShadowColor()
{
    QString subShadowColor = mpv_get_property_string(mpv, "sub-shadow-color");
    return fromColorString(subShadowColor);
}

void MpvHandler::SubtitleShadowColor(const QColor &color)
{
    QString colorStr = toColorString(color);
    QByteArray tmp = colorStr.toUtf8();
    mpv_set_property_string(mpv, "sub-shadow-color", tmp.constData());
}

QString MpvHandler::getMediaInfo()
{
    QFileInfo fi(path+file);

    double avsync, fps, vbitrate, abitrate;

    mpv_get_property(mpv, "avsync", MPV_FORMAT_DOUBLE, &avsync);
    mpv_get_property(mpv, "estimated-vf-fps", MPV_FORMAT_DOUBLE, &fps);
    mpv_get_property(mpv, "video-bitrate", MPV_FORMAT_DOUBLE, &vbitrate);
    mpv_get_property(mpv, "audio-bitrate", MPV_FORMAT_DOUBLE, &abitrate);
    QString current_vo = mpv_get_property_string(mpv, "current-vo");
    QString current_ao = mpv_get_property_string(mpv, "current-ao");
    QString hwdec_active = mpv_get_property_string(mpv, "hwdec-active");

    int vtracks = 0;
    int atracks = 0;

    for (auto &track : fileInfo.tracks) {
        if (track.type == "video")
            ++vtracks;
        else if (track.type == "audio")
            ++atracks;
    }

    const QString outer = "%0: %1\n", inner = "    %0: %1\n";

    QString out = outer.arg(tr("File"), fi.fileName()) +
            inner.arg(tr("Title"), fileInfo.media_title) +
            inner.arg(tr("File size"), Util::HumanSize(fi.size())) +
            inner.arg(tr("Date created"), fi.created().toString()) +
            inner.arg(tr("Media length"), Util::FormatTime(fileInfo.length, fileInfo.length)) + '\n';
    if (fileInfo.video_params.codec != QString())
        out += outer.arg(tr("Video (x%0)").arg(QString::number(vtracks)), fileInfo.video_params.codec) +
            inner.arg(tr("Video Output"), QString("%0 (hwdec %1)").arg(current_vo, hwdec_active)) +
            inner.arg(tr("Resolution"), QString("%0 x %1 (%2)").arg(QString::number(fileInfo.video_params.width),
                                                                    QString::number(fileInfo.video_params.height),
                                                                    Util::Ratio(fileInfo.video_params.width, fileInfo.video_params.height))) +
            inner.arg(tr("FPS"), QString::number(fps)) +
            inner.arg(tr("A/V Sync"), QString::number(avsync)) +
            inner.arg(tr("Bitrate"), tr("%0 kbps").arg(vbitrate/1000)) + '\n';
    if (fileInfo.audio_params.codec != QString())
        out += outer.arg(tr("Audio (x%0)").arg(QString::number(atracks)), fileInfo.audio_params.codec) +
            inner.arg(tr("Audio Output"), current_ao) +
            inner.arg(tr("Sample Rate"), QString::number(fileInfo.audio_params.samplerate)) +
            inner.arg(tr("Channels"), QString::number(fileInfo.audio_params.channels)) +
            inner.arg(tr("Bitrate"), tr("%0 kbps").arg(abitrate)) + '\n';

    if (fileInfo.chapters.length() > 0) {
        out += outer.arg(tr("Chapters"), QString());
        int n = 1;
        for (auto &chapter : fileInfo.chapters)
            out += inner.arg(QString::number(n++), chapter.title);
        out += '\n';
    }

    if (fileInfo.metadata.size() > 0) {
        out += outer.arg(tr("Metadata"), QString());
        for (auto data = fileInfo.metadata.begin(); data != fileInfo.metadata.end(); ++data)
            out += inner.arg(data.key(), *data);
        out += '\n';
    }

    return out;
}

int64_t MpvHandler::getCacheSize()
{
    int64_t fw_bytes = 0, cache_used = 0;
    mpv_node node;
    mpv_get_property(mpv, "track-list", MPV_FORMAT_NODE, &node);
    if (node.format == MPV_FORMAT_NODE_MAP) {
        for (int n = 0; n < node.u.list->num; n++) {
            if (QString(node.u.list->keys[n]) == "fw-bytes") {
                if (node.u.list->values[n].format == MPV_FORMAT_INT64)
                    fw_bytes = node.u.list->values[n].u.int64;
                break;
            }
        }
    }
    mpv_get_property(mpv, "cache-used", MPV_FORMAT_INT64, &cache_used);
    cache_used *= 1024;
    return fw_bytes + cache_used;
}

bool MpvHandler::event(QEvent *event)
{
    if (event->type() == QEvent::User) {
        while (mpv) {
            mpv_event *event = mpv_wait_event(mpv, 0);
            if (event == nullptr ||
               event->event_id == MPV_EVENT_NONE) {
                break;
            }
            HandleErrorCode(event->error);
            switch (event->event_id) {
            case MPV_EVENT_PROPERTY_CHANGE:
            {
                mpv_event_property *prop = (mpv_event_property*)event->data;
                if (QString(prop->name) == "playback-time") {   // playback-time does the same thing as time-pos but works for streaming media
                    if (prop->format == MPV_FORMAT_DOUBLE) {
                        setTime((int)*(double*)prop->data);
                        lastTime = time;
                    }
                } else if (QString(prop->name) == "volume") {
                    if (prop->format == MPV_FORMAT_DOUBLE)
                        setVolume((int)*(double*)prop->data);
                } else if (QString(prop->name) == "sid") {
                    if (prop->format == MPV_FORMAT_INT64)
                        setSid(*(int64_t*)prop->data);
                    else if (prop->format == MPV_FORMAT_NONE)
                        setSid(0);
                } else if (QString(prop->name) == "aid") {
                    if (prop->format == MPV_FORMAT_INT64)
                        setAid(*(int64_t*)prop->data);
                    else if (prop->format == MPV_FORMAT_NONE)
                        setAid(0);
                } else if (QString(prop->name) == "vid") {
                    if (prop->format == MPV_FORMAT_INT64)
                        setVid(*(int64_t*)prop->data);
                    else if (prop->format == MPV_FORMAT_NONE)
                        setVid(0);
                } else if (QString(prop->name) == "sub-visibility") {
                    if (prop->format == MPV_FORMAT_FLAG)
                        setSubtitleVisibility((bool)*(unsigned*)prop->data);
                } else if (QString(prop->name) == "mute") {
                    if (prop->format == MPV_FORMAT_FLAG)
                        setMute((bool)*(unsigned*)prop->data);
                } else if (QString(prop->name) == "core-idle") {
                    if (prop->format == MPV_FORMAT_FLAG) {
//                        if ((bool)*(unsigned*)prop->data && playState == Mpv::Playing)
//                            ShowText(tr("Buffering..."), 0);
//                        else
//                            ShowText(QString(), 0);
                    }
                } else if (QString(prop->name) == "paused-for-cache") {
                    if (prop->format == MPV_FORMAT_FLAG) {
                        if ((bool)*(unsigned*)prop->data && playState == Mpv::Playing)
                            ShowText(tr("Your network is slow or stuck, please wait a bit"), 0);
                        else
                            ShowText(QString(), 0);
                    }
                }
                break;
            }
            case MPV_EVENT_IDLE:
                fileInfo.length = 0;
                setTime(0);
                setPlayState(Mpv::Idle);
                LoadFileInfo();
                break;
                // these two look like they're reversed but they aren't. the names are misleading.
            case MPV_EVENT_START_FILE:
                setPlayState(Mpv::Loaded);
                break;
            case MPV_EVENT_FILE_LOADED:
                setPlayState(Mpv::Started);
                LoadFileInfo();
                SetProperties();
            case MPV_EVENT_UNPAUSE:
                setPlayState(Mpv::Playing);
                ShowText(tr("Resume"), 1000);
                break;
            case MPV_EVENT_PAUSE:
                setPlayState(Mpv::Paused);
                ShowText(tr("Pause"), 1000);
                break;
            case MPV_EVENT_END_FILE:
                if (playState == Mpv::Loaded)
                    ShowText(tr("File couldn't be opened"));
                setPlayState(Mpv::Stopped);
                break;
            case MPV_EVENT_SHUTDOWN:
                QCoreApplication::quit();
                break;
            case MPV_EVENT_LOG_MESSAGE:
            {
                mpv_event_log_message *message = static_cast<mpv_event_log_message*>(event->data);
                if (message != nullptr)
                    emit messageSignal(message->text);
                break;
            }
            default: // unhandled events
                break;
            }
        }
        return true;
    }
    return QObject::event(event);
}

void MpvHandler::AddOverlay(int id, int x, int y, QString file, int offset, int w, int h)
{
    QByteArray tmp_id = QString::number(id).toUtf8();
    QByteArray tmp_x = QString::number(x).toUtf8();
    QByteArray tmp_y = QString::number(y).toUtf8();
    QByteArray tmp_file = file.toUtf8();
    QByteArray tmp_offset = QString::number(offset).toUtf8();
    QByteArray tmp_w = QString::number(w).toUtf8();
    QByteArray tmp_h = QString::number(h).toUtf8();
    QByteArray tmp_stride = QString::number(4*w).toUtf8();

    const char *args[] = {"overlay_add",
                       tmp_id.constData(),
                       tmp_x.constData(),
                       tmp_y.constData(),
                       tmp_file.constData(),
                       tmp_offset.constData(),
                       "bgra",
                       tmp_w.constData(),
                       tmp_h.constData(),
                       tmp_stride.constData(),
                       NULL};
    Command(args);
}

void MpvHandler::RemoveOverlay(int id)
{
    QByteArray tmp = QString::number(id).toUtf8();
    const char *args[] = {"overlay_remove", tmp.constData(), NULL};
    AsyncCommand(args);
}

bool MpvHandler::FileExists(QString f)
{
    if (Util::IsValidUrl(f)) // web url
        return true;
    return QFile(f).exists();
}

void MpvHandler::LoadFile(QString f)
{
    PlayFile(LoadPlaylist(f));
}

QString MpvHandler::LoadPlaylist(QString f)
{
    if (f == QString()) // ignore empty file name
        return QString();

    if (f == "-") {
        setPath("");
        setPlaylist({f});
    } else if (Util::IsValidUrl(f)) { // web url
        setPath("");
        setPlaylist({f});
    } else { // local file
        QFileInfo fi(f);
        if (!fi.exists()) { // file doesn't exist
            ShowText(tr("File does not exist")); // tell the user
            return QString(); // don't do anything more
        } else if (fi.isDir()) { // if directory
            setPath(QDir::toNativeSeparators(fi.absoluteFilePath()+"/")); // set new path
            return PopulatePlaylist();
        } else if (fi.isFile()) { // if file
            setPath(QDir::toNativeSeparators(fi.absolutePath()+"/")); // set new path
            PopulatePlaylist();
            return fi.fileName();
        }
    }
    return f;
}

bool MpvHandler::PlayFile(QString f)
{
    if (f == QString()) // ignore if file doesn't exist
        return false;

    if (path == QString()) { // web url
        OpenFile(f);
        setFile(f);
    } else {
        QFile qf(path+f);
        if (qf.exists()) {
            OpenFile(path+f);
            setFile(f);
            Play();
        } else {
            ShowText(tr("File no longer exists")); // tell the user
            return false;
        }
    }
    return true;
}

void MpvHandler::Play()
{
    if (playState > 0 && mpv) {
        int f = 0;
        mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "pause", MPV_FORMAT_FLAG, &f);
    }
}

void MpvHandler::Pause()
{
    if (playState > 0 && mpv) {
        int f = 1;
        mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "pause", MPV_FORMAT_FLAG, &f);
    }
}

void MpvHandler::RestartPaused()
{
    Restart();
    Pause();
}

void MpvHandler::PlayPause(QString fileIfStopped)
{
    if (playState < 0) // not playing, play plays the selected playlist file
        PlayFile(fileIfStopped);
    else {
        const char *args[] = {"cycle", "pause", NULL};
        AsyncCommand(args);
    }
}

void MpvHandler::Restart()
{
    Seek(0);
    Play();
}

void MpvHandler::Rewind()
{
    // if user presses rewind button twice within 3 seconds, stop video
    if (time < 3) {
        RestartPaused();
    } else {
        if (playState == Mpv::Playing)
            Restart();
        else
            RestartPaused();
    }
}

void MpvHandler::Stop()
{
    if (playState > 0) {
        const char *args[] = {"stop", NULL};
        AsyncCommand(args);
    }
}

void MpvHandler::Mute(bool m)
{
    if (playState > 0) {
        int f = m;
        mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "mute", MPV_FORMAT_FLAG, &f);
    } else
        setMute(m);
}

void MpvHandler::Seek(int pos, bool relative, bool osd)
{
    if (playState > 0) {
        if (relative) {
            const QByteArray tmp = (((pos >= 0) ? "+" : QString())+QString::number(pos)).toUtf8();
            if (osd) {
                const char *args[] = {"osd-msg", "seek", tmp.constData(), NULL};
                AsyncCommand(args);
            } else {
                const char *args[] = {"seek", tmp.constData(), NULL};
                AsyncCommand(args);
            }
        } else {
            const QByteArray tmp = QString::number(pos).toUtf8();
            if (osd) {
                const char *args[] = {"osd-msg", "seek", tmp.constData(), "absolute", NULL};
                AsyncCommand(args);
            } else {
                const char *args[] = {"seek", tmp.constData(), "absolute", NULL};
                AsyncCommand(args);
            }
        }
    }
}

int MpvHandler::Relative(int pos)
{
    int ret = pos - lastTime;
    lastTime = pos;
    return ret;
}

void MpvHandler::FrameStep()
{
    const char *args[] = {"frame_step", NULL};
    AsyncCommand(args);
}

void MpvHandler::FrameBackStep()
{
    const char *args[] = {"frame_back_step", NULL};
    AsyncCommand(args);
}

void MpvHandler::Chapter(int c)
{
    int64_t chapter = c;
    mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "chapter", MPV_FORMAT_INT64, &chapter);
}

void MpvHandler::NextChapter()
{
    const char *args[] = {"add", "chapter", "1", NULL};
    AsyncCommand(args);
}

void MpvHandler::PreviousChapter()
{
    const char *args[] = {"add", "chapter", "-1", NULL};
    AsyncCommand(args);
}

void MpvHandler::Volume(int level, bool osd)
{
    if (level > 100) level = 100;
    else if (level < 0) level = 0;

    double v = level;

    if (playState > 0) {
        mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "volume", MPV_FORMAT_DOUBLE, &v);
        if (osd)
            ShowText(tr("Volume: %0%").arg(QString::number(level)));
    } else {
        mpv_set_option(mpv, "volume", MPV_FORMAT_DOUBLE, &v);
        setVolume(level);
    }
}

void MpvHandler::AudioDelay(double d)
{
    if (playState > 0)
        mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "audio-delay", MPV_FORMAT_DOUBLE, &d);
    setAudioDelay(d);
}

void MpvHandler::SubtitleDelay(double d)
{
    if (playState > 0)
        mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "sub-delay", MPV_FORMAT_DOUBLE, &d);
    setSubtitleDelay(d);
}

void MpvHandler::Speed(double d)
{
    if (playState > 0)
        mpv_set_property_async(mpv, MPV_REPLY_PROPERTY, "speed", MPV_FORMAT_DOUBLE, &d);
    setSpeed(d);
}

void MpvHandler::Aspect(QString aspect)
{
    const QByteArray tmp = aspect.toUtf8();
    const char *args[] = {"set", "video-aspect", tmp.constData(), NULL};
    AsyncCommand(args);
}


void MpvHandler::Vid(int vid)
{
    const QByteArray tmp = QString::number(vid).toUtf8();
    const char *args[] = {"set", "vid", tmp.constData(), NULL};
    AsyncCommand(args);
}

void MpvHandler::Aid(int aid)
{
    const QByteArray tmp = QString::number(aid).toUtf8();
    const char *args[] = {"set", "aid", tmp.constData(), NULL};
    AsyncCommand(args);
}

void MpvHandler::Sid(int sid)
{
    const QByteArray tmp = QString::number(sid).toUtf8();
    const char *args[] = {"set", "sid", tmp.constData(), NULL};
    AsyncCommand(args);
}

void MpvHandler::Screenshot(bool withSubs)
{
    const char *args[] = {"screenshot", (withSubs ? "subtitles" : "video"), NULL};
    AsyncCommand(args);
}

void MpvHandler::ScreenshotFormat(QString s)
{
    SetOption("screenshot-format", s);
    setScreenshotFormat(s);
}

void MpvHandler::ScreenshotTemplate(QString s)
{
    SetOption("screenshot-template", s);
    setScreenshotTemplate(s);
}

void MpvHandler::ScreenshotDirectory(QString s)
{
    SetOption("screenshot-directory", s);
    setScreenshotDir(s);
}

void MpvHandler::AddSubtitleTrack(QString f)
{
    if (f == QString())
        return;
    const QByteArray tmp = f.toUtf8();
    const char *args[] = {"sub-add", tmp.constData(), NULL};
    Command(args);
    // this could be more efficient if we saved tracks in a bst
    auto old = fileInfo.tracks; // save the current track-list
    LoadTracks(); // load the new track list
    auto current = fileInfo.tracks;
    for (auto track : old) // remove the old tracks in current
        current.removeOne(track);
    Mpv::Track &track = current.first();
    ShowText(QString("%0: %1 (%2)").arg(QString::number(track.id), track.title, track.external ? "external" : track.lang));
}

void MpvHandler::AddAudioTrack(QString f)
{
    if (f == QString())
        return;
    const QByteArray tmp = f.toUtf8();
    const char *args[] = {"audio-add", tmp.constData(), NULL};
    Command(args);
    auto old = fileInfo.tracks;
    LoadTracks();
    auto current = fileInfo.tracks;
    for (auto track : old)
        current.removeOne(track);
    Mpv::Track &track = current.first();
    ShowText(QString("%0: %1 (%2)").arg(QString::number(track.id), track.title, track.external ? "external" : track.lang));
}

void MpvHandler::ShowSubtitles(bool b)
{
    const char *args[] = {"set", "sub-visibility", b ? "yes" : "no", NULL};
    AsyncCommand(args);
}

void MpvHandler::SubtitleScale(double scale, bool relative)
{
    const QByteArray tmp = QString::number(scale).toUtf8();
    const char *args[] = {relative?"add":"set", "sub-scale", tmp.constData(), NULL};
    AsyncCommand(args);
}

void MpvHandler::Deinterlace(bool deinterlace)
{
    HandleErrorCode(mpv_set_property_string(mpv, "deinterlace", deinterlace ? "yes" : "auto"));
    ShowText(tr("Deinterlacing: %0").arg(deinterlace ? tr("enabled") : tr("disabled")));
}

void MpvHandler::Interpolate(bool interpolate)
{
    if (vo == QString())
        vo = mpv_get_property_string(mpv, "current-vo");
    QStringList vos = vo.split(',');
    for (auto &o : vos) {
        int i = o.indexOf(":interpolation");
        if (interpolate && i == -1)
            o.append(":interpolation");
        else if (i != -1)
            o.remove(i, QString(":interpolation").length());
    }
    setVo(vos.join(','));
    SetOption("vo", vo);
    ShowText(tr("Motion Interpolation: %0").arg(interpolate ? tr("enabled") : tr("disabled")));
}

void MpvHandler::Vo(QString o)
{
    setVo(o);
    SetOption("vo", vo);
}

void MpvHandler::MsgLevel(QString level)
{
    QByteArray tmp = level.toUtf8();
    mpv_request_log_messages(mpv, tmp.constData());
    setMsgLevel(level);
}

void MpvHandler::ShowText(QString text, int duration)
{
    baka->overlay->showStatusText(text, duration);
    /*
    const QByteArray tmp1 = text.toUtf8(),
                     tmp2 = QString::number(duration).toUtf8(),
                     tmp3 = QString::number(level).toUtf8();
    const char *args[] = {"show_text", tmp1.constData(), tmp2.constData(), tmp3.constData(), NULL};
    AsyncCommand(args);
    */
}

void MpvHandler::LoadFileInfo()
{
    if (playState < 0) {
        fileInfo.media_title.clear();
        fileInfo.length = 0;
    } else {
        // get media-title
        fileInfo.media_title = mpv_get_property_string(mpv, "media-title");
        // get length
        double len;
        mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &len);
        fileInfo.length = (int)len;
    }

    LoadTracks();
    LoadChapters();
    LoadVideoParams();
    LoadAudioParams();
    LoadMetadata();

    emit fileInfoChanged(fileInfo);
}

void MpvHandler::LoadTracks()
{
    fileInfo.tracks.clear();
    hasVideo = false;
    if (playState < 0)
        widget->setContentImage(QImage());
    else {
        mpv_node node;
        mpv_get_property(mpv, "track-list", MPV_FORMAT_NODE, &node);
        if (node.format == MPV_FORMAT_NODE_ARRAY) {
            for (int i = 0; i < node.u.list->num; i++) {
                if (node.u.list->values[i].format == MPV_FORMAT_NODE_MAP) {
                    Mpv::Track track;
                    for (int n = 0; n < node.u.list->values[i].u.list->num; n++) {
                        if (QString(node.u.list->values[i].u.list->keys[n]) == "id") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_INT64)
                                track.id = node.u.list->values[i].u.list->values[n].u.int64;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "type") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_STRING)
                                track.type = node.u.list->values[i].u.list->values[n].u.string;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "src-id") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_INT64)
                                track.src_id = node.u.list->values[i].u.list->values[n].u.int64;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "title") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_STRING)
                                track.title = node.u.list->values[i].u.list->values[n].u.string;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "lang") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_STRING)
                                track.lang = node.u.list->values[i].u.list->values[n].u.string;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "albumart") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_FLAG)
                                track.albumart = node.u.list->values[i].u.list->values[n].u.flag;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "default") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_FLAG)
                                track._default = node.u.list->values[i].u.list->values[n].u.flag;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "external") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_FLAG)
                                track.external = node.u.list->values[i].u.list->values[n].u.flag;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "external-filename") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_STRING)
                                track.external_filename = node.u.list->values[i].u.list->values[n].u.string;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "codec") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_STRING)
                                track.codec = node.u.list->values[i].u.list->values[n].u.string;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "demux-w") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_INT64)
                                track.demux_w = node.u.list->values[i].u.list->values[n].u.int64;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "demux-h") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_INT64)
                                track.demux_h = node.u.list->values[i].u.list->values[n].u.int64;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "demux-fps") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_DOUBLE)
                                track.demux_fps = node.u.list->values[i].u.list->values[n].u.double_;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "demux-channel-count") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_INT64)
                                track.demux_channel_count = node.u.list->values[i].u.list->values[n].u.int64;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "demux-samplerate") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_INT64)
                                track.demux_samplerate = node.u.list->values[i].u.list->values[n].u.int64;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "decoder-desc") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_STRING)
                                track.decoder_desc = node.u.list->values[i].u.list->values[n].u.string;
                        }
                    }
                    if (track.type == "video")
                        hasVideo = true;
                    fileInfo.tracks.push_back(track);
                }
            }
        }
        widget->setContentImage(hasVideo ? QImage() : defaultAlbumArt);
    }
    emit trackListChanged(fileInfo.tracks);
}

void MpvHandler::LoadChapters()
{
    fileInfo.chapters.clear();
    if (playState > 0) {
        mpv_node node;
        mpv_get_property(mpv, "chapter-list", MPV_FORMAT_NODE, &node);
        if (node.format == MPV_FORMAT_NODE_ARRAY) {
            for (int i = 0; i < node.u.list->num; i++) {
                if (node.u.list->values[i].format == MPV_FORMAT_NODE_MAP) {
                    Mpv::Chapter ch;
                    for (int n = 0; n < node.u.list->values[i].u.list->num; n++) {
                        if (QString(node.u.list->values[i].u.list->keys[n]) == "title") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_STRING)
                                ch.title = node.u.list->values[i].u.list->values[n].u.string;
                        } else if (QString(node.u.list->values[i].u.list->keys[n]) == "time") {
                            if (node.u.list->values[i].u.list->values[n].format == MPV_FORMAT_DOUBLE)
                                ch.time = (int)node.u.list->values[i].u.list->values[n].u.double_;
                        }
                    }
                    fileInfo.chapters.push_back(ch);
                }
            }
        }
    }
    emit chaptersChanged(fileInfo.chapters);
}

void MpvHandler::LoadVideoParams()
{
    if (playState < 0) {
        fileInfo.video_params.codec.clear();
        fileInfo.video_params.width = fileInfo.video_params.dwidth = 0;
        fileInfo.video_params.height = fileInfo.video_params.dheight = 0;
    } else if (hasVideo) {
        fileInfo.video_params.codec = mpv_get_property_string(mpv, "video-codec");

        int64_t width = 0, height = 0, dwidth = 0, dheight = 0;
        mpv_get_property(mpv, "width",        MPV_FORMAT_INT64, &width);
        mpv_get_property(mpv, "height",       MPV_FORMAT_INT64, &height);
        mpv_get_property(mpv, "dwidth",       MPV_FORMAT_INT64, &dwidth);
        mpv_get_property(mpv, "dheight",      MPV_FORMAT_INT64, &dheight);

        fileInfo.video_params.width = width;
        fileInfo.video_params.height = height;
        fileInfo.video_params.dwidth = dwidth;
        fileInfo.video_params.dheight = dheight;
    } else {
        fileInfo.video_params.codec = "png (PNG (Portable Network Graphics) image)";
        fileInfo.video_params.width = fileInfo.video_params.dwidth = defaultAlbumArt.width();
        fileInfo.video_params.height = fileInfo.video_params.dheight = defaultAlbumArt.height();
    }

    emit videoParamsChanged(fileInfo.video_params);
}

void MpvHandler::LoadAudioParams()
{
    if (playState < 0) {
        fileInfo.audio_params.codec.clear();
        fileInfo.audio_params.samplerate = 0;
        fileInfo.audio_params.channels = 0;
    } else {
        fileInfo.audio_params.codec = mpv_get_property_string(mpv, "audio-codec");
        mpv_node node;
        mpv_get_property(mpv, "audio-params", MPV_FORMAT_NODE, &node);
        if (node.format == MPV_FORMAT_NODE_MAP) {
            for (int i = 0; i < node.u.list->num; i++) {
                if (QString(node.u.list->keys[i]) == "samplerate") {
                    if (node.u.list->values[i].format == MPV_FORMAT_INT64)
                        fileInfo.audio_params.samplerate = node.u.list->values[i].u.int64;
                } else if (QString(node.u.list->keys[i]) == "channel-count") {
                    if (node.u.list->values[i].format == MPV_FORMAT_INT64)
                        fileInfo.audio_params.channels = node.u.list->values[i].u.int64;
                }
            }
        }
    }

    emit audioParamsChanged(fileInfo.audio_params);
}

void MpvHandler::LoadMetadata()
{
    fileInfo.metadata.clear();
    if (playState > 0) {
        mpv_node node;
        mpv_get_property(mpv, "metadata", MPV_FORMAT_NODE, &node);
        if (node.format == MPV_FORMAT_NODE_MAP)
            for (int n = 0; n < node.u.list->num; n++)
                if (node.u.list->values[n].format == MPV_FORMAT_STRING)
                    fileInfo.metadata[node.u.list->keys[n]] = node.u.list->values[n].u.string;
    }
}

void MpvHandler::LoadOsdSize()
{
    int64_t width = 0, height = 0;
    mpv_get_property(mpv, "osd-width", MPV_FORMAT_INT64, &width);
    mpv_get_property(mpv, "osd-height", MPV_FORMAT_INT64, &height);
    osdWidth = width;
    osdHeight = height;
}

void MpvHandler::Command(const QStringList &strlist)
{
    // convert input string into char array
    int len = strlist.length();
    char **data = new char*[len+1];
    for (int i = 0; i < len; ++i) {
        data[i] = new char[strlist[i].length()+1];
        memcpy(data[i], QByteArray(strlist[i].toUtf8()).begin(), strlist[i].length()+1);
    }
    data[len] = NULL;
    AsyncCommand(const_cast<const char**>(data));
    for (int i = 0; i < len; ++i)
        delete [] data[i];
    delete [] data;

//    const QByteArray tmp = str.toUtf8();
//    mpv_command_string(mpv, tmp.constData());
}

void MpvHandler::SetOption(QString key, QString val)
{
    QByteArray tmp1 = key.toUtf8(),
               tmp2 = val.toUtf8();
    HandleErrorCode(mpv_set_option_string(mpv, tmp1.constData(), tmp2.constData()));
}

void MpvHandler::OpenFile(QString f)
{
    emit fileChanging(time, fileInfo.length);

    if (readyToRender) {
        const QByteArray tmp = f.toUtf8();
        const char *args[] = {"loadfile", tmp.constData(), NULL};
        Command(args);
    } else {
        connect(this, &MpvHandler::renderContextCreated, this, [=] {
            const QByteArray tmp = f.toUtf8();
            const char *args[] = {"loadfile", tmp.constData(), NULL};
            Command(args);
        }, Qt::QueuedConnection);
    }
}

QString MpvHandler::PopulatePlaylist()
{
    if (path != "") {
        QStringList playlist;
        QDir root(path);
        QStringList filter = Mpv::media_filetypes;
        if (path != QString() && file != QString())
            filter.append(QString("*.%1").arg(file.split(".").last()));
        QFileInfoList flist;
        flist = root.entryInfoList(filter, QDir::Files);
        for (auto &i : flist)
            playlist.push_back(i.fileName()); // add files to the list
        setPlaylist(playlist);
        if (playlist.empty())
            return QString();
        return playlist.first();
    }
    return QString();
}

void MpvHandler::SetProperties()
{
    Volume(volume);
    Speed(speed);
    Mute(mute);
}

void MpvHandler::AsyncCommand(const char *args[])
{
    mpv_command_async(mpv, MPV_REPLY_COMMAND, args);
}

void MpvHandler::Command(const char *args[])
{
    HandleErrorCode(mpv_command(mpv, args));
}

void MpvHandler::HandleErrorCode(int error_code)
{
    if (error_code >= 0)
        return;
    QString error = mpv_error_string(error_code);
    if (error != QString())
        emit messageSignal(error+"\n");
}
