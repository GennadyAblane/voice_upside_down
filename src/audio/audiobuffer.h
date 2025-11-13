#pragma once

#include <QAudioFormat>
#include <QByteArray>
#include <QVector>

class AudioBuffer
{
public:
    AudioBuffer() = default;

    void clear();
    void setFormat(const QAudioFormat &format);
    const QAudioFormat &format() const;

    QByteArray &data();
    const QByteArray &data() const;

    qint64 sampleCount() const;
    qint64 frameCount() const;
    qint64 durationMs() const;

    QByteArray sliceSamples(qint64 startSample, qint64 sampleCount) const;
    QByteArray sliceFrames(qint64 startFrame, qint64 frameCount) const;

    static QByteArray reverseSamples(const QByteArray &pcm, const QAudioFormat &format);

private:
    QAudioFormat m_format;
    QByteArray m_data;
};

