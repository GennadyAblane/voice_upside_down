#include "audiofiledecoder.h"

#include <QAudioFormat>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QtEndian>
#include <cstdlib>
#include <cstring>

#include "../utils/logger.h"
#include "wavutils.h"
#include "qt6_compat.h"

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD
#include "../../thirdparty/minimp3_ex.h"

namespace {

QString toLocalPath(const QString &path)
{
    if (path.startsWith(QStringLiteral("file://")))
        return QUrl(path).toLocalFile();
    return path;
}

} // namespace

AudioFileDecoder::AudioFileDecoder(QObject *parent)
    : QObject(parent)
{
}

bool AudioFileDecoder::decodeFile(const QString &filePath, AudioBuffer &outBuffer, QString *errorString)
{
    QString localPath = toLocalPath(filePath);
    QFileInfo info(localPath);
    if (!info.exists()) {
        const QString err = QObject::tr("Файл не найден: %1").arg(localPath);
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Audio source not found:" << localPath;
        return false;
    }

    const QString ext = info.suffix().toLower();
    if (ext == QStringLiteral("wav")) {
        QByteArray pcm;
        QAudioFormat format;
        if (!WavUtils::readWavFile(localPath, pcm, format, errorString))
            return false;
        outBuffer.setFormat(format);
        outBuffer.data() = pcm;
        return true;
    }

    if (ext == QStringLiteral("mp3")) {
        mp3dec_t decoder;
        mp3dec_file_info_t mp3info;
        memset(&decoder, 0, sizeof(decoder));
        memset(&mp3info, 0, sizeof(mp3info));
        int res = mp3dec_load(&decoder, localPath.toUtf8().constData(), &mp3info, nullptr, nullptr);
        if (res != 0 || !mp3info.buffer || mp3info.samples == 0 || mp3info.channels <= 0 || mp3info.hz <= 0) {
            if (mp3info.buffer)
                free(mp3info.buffer);
            const QString err = QObject::tr("Не удалось декодировать MP3-файл.");
            if (errorString)
                *errorString = err;
            LOG_WARN() << "Failed to decode MP3:" << localPath << "result" << res;
            return false;
        }

        const size_t totalSamples = static_cast<size_t>(mp3info.samples); // already includes channels
        const size_t totalBytes = totalSamples * sizeof(int16_t);
        QByteArray pcmData(static_cast<int>(totalBytes), Qt::Uninitialized);
        memcpy(pcmData.data(), mp3info.buffer, totalBytes);

        QAudioFormat format;
        format.setChannelCount(mp3info.channels);
        format.setSampleRate(mp3info.hz);
        Qt6Compat::setSampleSize(format, 16);
        Qt6Compat::setSignedInt(format);

        outBuffer.setFormat(format);
        outBuffer.data() = pcmData;

        free(mp3info.buffer);
        LOG_INFO() << "MP3 decoded successfully:" << localPath << "channels" << format.channelCount() << "rate" << format.sampleRate();
        return true;
    }

    const QString err = QObject::tr("Формат файла не поддерживается: %1").arg(ext);
    if (errorString)
        *errorString = err;
    LOG_WARN() << "Unsupported audio format:" << localPath << "extension" << ext;
    return false;
}
#include "audiofiledecoder.h"

#include <QAudioBuffer>
#include <QAudioDecoder>
#include <QEventLoop>
#include <QFileInfo>

#include "../utils/logger.h"

