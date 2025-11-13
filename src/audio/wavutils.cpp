#include "wavutils.h"

#include <QDataStream>
#include <QFile>
#include <QtEndian>

#include "../utils/logger.h"

namespace {

constexpr int kWavHeaderSize = 44;

}

namespace WavUtils {

bool readWavFile(const QString &filePath, QByteArray &pcmData, QAudioFormat &format, QString *errorString)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        const QString err = QObject::tr("Не удалось открыть WAV-файл: %1").arg(file.errorString());
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Failed to open WAV file:" << filePath << err;
        return false;
    }

    // Read RIFF header
    QByteArray riffHeader = file.read(12);
    if (riffHeader.size() < 12) {
        const QString err = QObject::tr("WAV-файл поврежден или имеет неполный заголовок.");
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Invalid WAV header in" << filePath;
        return false;
    }

    const char *hdr = riffHeader.constData();
    if (memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) {
        const QString err = QObject::tr("Файл не является корректным WAV (отсутствует подпись RIFF/WAVE).");
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Not a valid RIFF/WAVE file:" << filePath;
        return false;
    }

    // Read fmt chunk
    QByteArray fmtChunkId = file.read(4);
    if (fmtChunkId.size() < 4 || memcmp(fmtChunkId.constData(), "fmt ", 4) != 0) {
        const QString err = QObject::tr("WAV-файл не содержит блока fmt.");
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Missing fmt chunk in WAV:" << filePath;
        return false;
    }

    QByteArray fmtSizeBytes = file.read(4);
    if (fmtSizeBytes.size() < 4) {
        const QString err = QObject::tr("WAV-файл поврежден (неполный блок fmt).");
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Incomplete fmt chunk in WAV:" << filePath;
        return false;
    }

    quint32 fmtSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(fmtSizeBytes.constData()));
    QByteArray fmtData = file.read(fmtSize);
    if (fmtData.size() < 16) {
        const QString err = QObject::tr("WAV-файл поврежден (неполные данные fmt).");
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Incomplete fmt data in WAV:" << filePath;
        return false;
    }

    const uchar *fmt = reinterpret_cast<const uchar *>(fmtData.constData());
    quint16 audioFormat = qFromLittleEndian<quint16>(fmt);
    quint16 channelCount = qFromLittleEndian<quint16>(fmt + 2);
    quint32 sampleRate = qFromLittleEndian<quint32>(fmt + 4);
    quint16 bitsPerSample = qFromLittleEndian<quint16>(fmt + 14);

    if ((audioFormat != 1 && audioFormat != 3) || (bitsPerSample != 16 && bitsPerSample != 32)) {
        const QString err = QObject::tr("Формат WAV не поддерживается (код %1, %2 бит).").arg(audioFormat).arg(bitsPerSample);
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Unsupported WAV format:" << filePath << "format" << audioFormat << "bits" << bitsPerSample;
        return false;
    }

    // Skip any extra fmt data or other chunks before data
    if (fmtSize > 16) {
        file.seek(file.pos() + (fmtSize - 16));
    }

    // Find data chunk
    QByteArray chunkId;
    quint32 chunkSize = 0;
    while (!file.atEnd()) {
        chunkId = file.read(4);
        if (chunkId.size() < 4)
            break;
        QByteArray sizeBytes = file.read(4);
        if (sizeBytes.size() < 4)
            break;
        chunkSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(sizeBytes.constData()));
        if (chunkId == "data")
            break;
        // Skip to next chunk (chunk size + padding if odd)
        file.seek(file.pos() + chunkSize + (chunkSize & 1));
    }

    if (chunkId != "data") {
        const QString err = QObject::tr("В WAV-файле отсутствует блок PCM-данных.");
        if (errorString)
            *errorString = err;
        LOG_WARN() << "WAV data chunk not found:" << filePath;
        return false;
    }

    QByteArray data = file.read(chunkSize);
    if (data.isEmpty()) {
        const QString err = QObject::tr("Не удалось прочитать PCM-данные из WAV.");
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Empty PCM data in WAV:" << filePath;
        return false;
    }

    if (audioFormat == 3 && bitsPerSample == 32) {
        const int sampleCount = data.size() / static_cast<int>(sizeof(float));
        const float *floatSamples = reinterpret_cast<const float *>(data.constData());
        QByteArray pcm16(sampleCount * static_cast<int>(sizeof(qint16)), Qt::Uninitialized);
        qint16 *dst = reinterpret_cast<qint16 *>(pcm16.data());
        for (int i = 0; i < sampleCount; ++i) {
            float sample = floatSamples[i];
            if (sample > 1.0f)
                sample = 1.0f;
            else if (sample < -1.0f)
                sample = -1.0f;
            dst[i] = static_cast<qint16>(sample * 32767.0f);
        }
        pcmData = pcm16;
        bitsPerSample = 16;
        audioFormat = 1;
    } else {
        pcmData = data;
    }

    format.setChannelCount(channelCount);
    format.setSampleRate(sampleRate);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setByteOrder(QAudioFormat::LittleEndian);

    LOG_INFO() << "WAV loaded:" << filePath << "channels" << channelCount << "rate" << sampleRate;
    return true;
}

bool writeWavFile(const QString &filePath, const QAudioFormat &format, const QByteArray &pcmData, QString *errorString)
{
    if (!format.isValid() || format.sampleSize() != 16 || format.sampleType() != QAudioFormat::SignedInt) {
        const QString err = QObject::tr("Для записи WAV требуется 16-битный PCM.");
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Attempted to write WAV with unsupported format:" << filePath;
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        const QString err = QObject::tr("Не удалось открыть файл для записи: %1").arg(file.errorString());
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Failed to write WAV file:" << filePath << err;
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    const quint32 dataSize = static_cast<quint32>(pcmData.size());
    const quint32 riffSize = dataSize + 36;

    stream.writeRawData("RIFF", 4);
    stream << riffSize;
    stream.writeRawData("WAVE", 4);
    stream.writeRawData("fmt ", 4);
    stream << quint32(16); // PCM chunk size
    stream << quint16(1); // PCM format
    stream << quint16(format.channelCount());
    stream << quint32(format.sampleRate());
    const quint32 byteRate = format.sampleRate() * format.channelCount() * (format.sampleSize() / 8);
    const quint16 blockAlign = format.channelCount() * (format.sampleSize() / 8);
    stream << byteRate;
    stream << blockAlign;
    stream << quint16(format.sampleSize());
    stream.writeRawData("data", 4);
    stream << dataSize;

    if (stream.writeRawData(pcmData.constData(), pcmData.size()) != pcmData.size()) {
        const QString err = QObject::tr("Ошибка при записи PCM-данных в WAV.");
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Failed to write PCM data for WAV:" << filePath;
        return false;
    }

    file.close();
    LOG_INFO() << "WAV written:" << filePath << "bytes" << dataSize;
    return true;
}

} // namespace WavUtils


