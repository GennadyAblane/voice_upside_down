#include "audioproject.h"

#include <QtMath>

QString SegmentInfo::title() const
{
    return QString::fromUtf8("Отрезок %1").arg(displayIndex);
}

AudioProject::AudioProject(QObject *parent)
    : QObject(parent)
{
}

QString AudioProject::projectName() const
{
    return m_projectName;
}

void AudioProject::setProjectName(const QString &name)
{
    m_projectName = name;
}

QString AudioProject::originalFilePath() const
{
    return m_originalFilePath;
}

void AudioProject::setOriginalFilePath(const QString &path)
{
    m_originalFilePath = path;
}

QString AudioProject::decodedFilePath() const
{
    return m_decodedFilePath;
}

void AudioProject::setDecodedFilePath(const QString &path)
{
    m_decodedFilePath = path;
}

AudioBuffer &AudioProject::originalBuffer()
{
    return m_originalBuffer;
}

const AudioBuffer &AudioProject::originalBuffer() const
{
    return m_originalBuffer;
}

QVector<SegmentInfo> &AudioProject::segments()
{
    return m_segments;
}

const QVector<SegmentInfo> &AudioProject::segments() const
{
    return m_segments;
}

int AudioProject::segmentLengthSeconds() const
{
    return m_segmentLengthSeconds;
}

void AudioProject::setSegmentLengthSeconds(int seconds)
{
    if (seconds < 1)
        seconds = 1;
    if (seconds > 10)
        seconds = 10;
    if (m_segmentLengthSeconds != seconds) {
        m_segmentLengthSeconds = seconds;
        resetSegmentStatuses();
        splitIntoSegments();
    }
}

void AudioProject::resetSegmentStatuses()
{
    for (auto &segment : m_segments) {
        segment.hasRecording = false;
        segment.hasReverse = false;
        segment.recordingPath.clear();
        segment.reversePath.clear();
    }
}

void AudioProject::splitIntoSegments()
{
    m_segments.clear();
    const auto &buffer = m_originalBuffer;
    if (!buffer.format().isValid() || buffer.frameCount() == 0)
        return;

    const qint64 sampleRate = buffer.format().sampleRate();
    if (sampleRate <= 0)
        return;

    const qint64 framesPerSecond = sampleRate;
    const qint64 framesPerSegment = framesPerSecond * m_segmentLengthSeconds;
    const qint64 minFrames = framesPerSecond; // 1 second
    const qint64 totalFrames = buffer.frameCount();

    qint64 remainingFrames = totalFrames;
    int displayIndex = 1;

    while (remainingFrames > 0) {
        const qint64 framesForSegment = qMin(framesPerSegment, remainingFrames);
        const qint64 startFrame = remainingFrames - framesForSegment;

        if (startFrame == 0 && framesForSegment < minFrames && totalFrames > minFrames) {
            // discard tail shorter than 1 second
            break;
        }
        if (totalFrames <= minFrames && framesForSegment < minFrames) {
            // whole track shorter than 1 second; skip creating segments
            m_segments.clear();
            emit segmentsUpdated();
            return;
        }

        SegmentInfo info;
        info.displayIndex = displayIndex++;
        info.startFrame = startFrame;
        info.frameCount = framesForSegment;
        info.durationMs = static_cast<qint64>((static_cast<double>(framesForSegment) / sampleRate) * 1000.0);
        m_segments.push_back(info);

        if (startFrame == 0)
            break;
        remainingFrames = startFrame;
    }

    emit segmentsUpdated();
}
