#pragma once

#include "audiobuffer.h"

#include <QObject>
#include <QString>
#include <QVector>

struct SegmentInfo
{
    int displayIndex = 0;
    qint64 startFrame = 0;
    qint64 frameCount = 0;
    bool hasRecording = false;
    bool hasReverse = false;
    QString recordingPath;
    QString reversePath;

    qint64 durationMs = 0;

    QString title() const;
};

class AudioProject : public QObject
{
    Q_OBJECT
public:
    explicit AudioProject(QObject *parent = nullptr);

    QString projectName() const;
    void setProjectName(const QString &name);

    QString originalFilePath() const;
    void setOriginalFilePath(const QString &path);

    QString decodedFilePath() const;
    void setDecodedFilePath(const QString &path);

    AudioBuffer &originalBuffer();
    const AudioBuffer &originalBuffer() const;

    QVector<SegmentInfo> &segments();
    const QVector<SegmentInfo> &segments() const;

    int segmentLengthSeconds() const;
    void setSegmentLengthSeconds(int seconds);

    void resetSegmentStatuses();
    void splitIntoSegments();

signals:
    void segmentsUpdated();

private:
    QString m_projectName;
    QString m_originalFilePath;
    QString m_decodedFilePath;
    AudioBuffer m_originalBuffer;
    QVector<SegmentInfo> m_segments;
    int m_segmentLengthSeconds = 5;
};

