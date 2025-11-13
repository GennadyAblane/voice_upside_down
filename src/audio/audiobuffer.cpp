#include "audiobuffer.h"

#include <QtGlobal>
#include <cstring>

namespace {
qint64 bytesPerSample(const QAudioFormat &format)
{
    if (!format.isValid())
        return 0;
    const int sampleSize = format.sampleSize();
    if (sampleSize <= 0)
        return 0;
    return sampleSize / 8;
}

qint64 bytesPerFrame(const QAudioFormat &format)
{
    const qint64 sampleBytes = bytesPerSample(format);
    if (sampleBytes <= 0)
        return 0;
    return sampleBytes * format.channelCount();
}
}

void AudioBuffer::clear()
{
    m_data.clear();
    m_format = QAudioFormat();
}

void AudioBuffer::setFormat(const QAudioFormat &format)
{
    m_format = format;
}

const QAudioFormat &AudioBuffer::format() const
{
    return m_format;
}

QByteArray &AudioBuffer::data()
{
    return m_data;
}

const QByteArray &AudioBuffer::data() const
{
    return m_data;
}

qint64 AudioBuffer::sampleCount() const
{
    const qint64 sampleBytes = bytesPerSample(m_format);
    if (sampleBytes <= 0)
        return 0;
    return m_data.size() / sampleBytes;
}

qint64 AudioBuffer::frameCount() const
{
    const qint64 frameBytes = bytesPerFrame(m_format);
    if (frameBytes <= 0)
        return 0;
    return m_data.size() / frameBytes;
}

qint64 AudioBuffer::durationMs() const
{
    if (!m_format.isValid() || m_format.sampleRate() == 0)
        return 0;
    const double seconds = static_cast<double>(frameCount()) / static_cast<double>(m_format.sampleRate());
    return static_cast<qint64>(seconds * 1000.0);
}

QByteArray AudioBuffer::sliceSamples(qint64 startSample, qint64 sampleCount) const
{
    const qint64 sampleBytes = bytesPerSample(m_format);
    if (sampleBytes <= 0)
        return {};
    const qint64 totalSamples = this->sampleCount();
    if (startSample < 0 || sampleCount <= 0 || startSample >= totalSamples)
        return {};

    const qint64 availableSamples = qMin(sampleCount, totalSamples - startSample);
    const qint64 startByte = startSample * sampleBytes;
    return QByteArray(m_data.constData() + startByte, static_cast<int>(availableSamples * sampleBytes));
}

QByteArray AudioBuffer::sliceFrames(qint64 startFrame, qint64 frameCount) const
{
    const qint64 frameBytes = bytesPerFrame(m_format);
    if (frameBytes <= 0)
        return {};

    const qint64 totalFrames = this->frameCount();
    if (startFrame < 0 || frameCount <= 0 || startFrame >= totalFrames)
        return {};

    const qint64 availableFrames = qMin(frameCount, totalFrames - startFrame);
    const qint64 startByte = startFrame * frameBytes;
    return QByteArray(m_data.constData() + startByte, static_cast<int>(availableFrames * frameBytes));
}

QByteArray AudioBuffer::reverseSamples(const QByteArray &pcm, const QAudioFormat &format)
{
    const qint64 frameBytes = bytesPerFrame(format);
    if (frameBytes <= 0)
        return {};

    QByteArray reversed(pcm.size(), Qt::Uninitialized);
    const char *src = pcm.constData();
    char *dst = reversed.data();
    const qint64 frameCount = pcm.size() / frameBytes;
    for (qint64 i = 0; i < frameCount; ++i) {
        const char *frameStart = src + (frameCount - 1 - i) * frameBytes;
        memcpy(dst + i * frameBytes, frameStart, frameBytes);
    }
    return reversed;
}

