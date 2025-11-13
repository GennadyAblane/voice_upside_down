#pragma once

#include "audio/audioproject.h"
#include "audio/segmentmodel.h"

#include <QObject>
#include <QSet>

class AudioFileDecoder;
class AudioPlaybackEngine;
class RecordingEngine;
class ProjectSerializer;

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(SegmentModel *segmentModel READ segmentModel CONSTANT)
    Q_PROPERTY(int segmentLength READ segmentLength NOTIFY segmentLengthChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool projectReady READ projectReady NOTIFY projectReadinessChanged)
    Q_PROPERTY(QString currentSourceName READ currentSourceName NOTIFY currentSourceNameChanged)
    Q_PROPERTY(bool sourceRecording READ sourceRecording NOTIFY sourceRecordingChanged)
    Q_PROPERTY(bool canAdjustSegmentLength READ canAdjustSegmentLength NOTIFY canAdjustSegmentLengthChanged)
    Q_PROPERTY(QString segmentHintText READ segmentHintText NOTIFY segmentHintTextChanged)
    Q_PROPERTY(bool interactionsEnabled READ interactionsEnabled NOTIFY interactionsStateChanged)
    Q_PROPERTY(bool canGlue READ canGlue NOTIFY glueStateChanged)
    Q_PROPERTY(bool canPlayGlue READ canPlayGlue NOTIFY glueStateChanged)
    Q_PROPERTY(bool gluePlaybackActive READ gluePlaybackActive NOTIFY glueStateChanged)
    Q_PROPERTY(bool canReverseSong READ canReverseSong NOTIFY reverseStateChanged)
    Q_PROPERTY(bool canPlayReverse READ canPlayReverse NOTIFY reverseStateChanged)
    Q_PROPERTY(bool reversePlaybackActive READ reversePlaybackActive NOTIFY reverseStateChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    SegmentModel *segmentModel();

    int segmentLength() const;
    QString statusMessage() const;
    bool projectReady() const;
    QString currentSourceName() const;
    bool sourceRecording() const;
    bool canAdjustSegmentLength() const;
    QString segmentHintText() const;
    bool interactionsEnabled() const;
    bool canGlue() const;
    bool canPlayGlue() const;
    bool gluePlaybackActive() const;
    bool canReverseSong() const;
    bool canPlayReverse() const;
    bool reversePlaybackActive() const;

    Q_INVOKABLE void loadAudioSource(const QString &filePath);
    Q_INVOKABLE void startSourceRecording();
    Q_INVOKABLE void stopSourceRecording();
    Q_INVOKABLE void saveProjectTo(const QString &directoryPath);
    Q_INVOKABLE void openProjectFrom(const QString &projectFilePath);
    Q_INVOKABLE void changeSegmentLength(int seconds);

    Q_INVOKABLE bool isSegmentRecording(int segmentIndex) const;
    Q_INVOKABLE bool isSegmentOriginalPlaying(int segmentIndex) const;
    Q_INVOKABLE bool isSegmentRecordingPlaying(int segmentIndex) const;
    Q_INVOKABLE bool isSegmentReversePlaying(int segmentIndex) const;

    Q_INVOKABLE void toggleSegmentRecording(int segmentIndex);
    Q_INVOKABLE void toggleSegmentOriginalPlayback(int segmentIndex);
    Q_INVOKABLE void toggleSegmentRecordedPlayback(int segmentIndex);
    Q_INVOKABLE void toggleSegmentReversePlayback(int segmentIndex);

    Q_INVOKABLE void glueSegments();
    Q_INVOKABLE void toggleGluePlayback();
    Q_INVOKABLE void reverseSong();
    Q_INVOKABLE void toggleSongReversePlayback();

    Q_INVOKABLE void saveProject();

signals:
    void segmentLengthChanged();
    void statusMessageChanged();
    void projectReadinessChanged();
    void currentSourceNameChanged();
    void sourceRecordingChanged();
    void canAdjustSegmentLengthChanged();
    void segmentHintTextChanged();
    void interactionsStateChanged();
    void glueStateChanged();
    void reverseStateChanged();

private:
    void setStatusMessage(const QString &message);
    void refreshUiStates();
    void clearPlaybackStates();
    void ensureProjectNameFromSource(const QString &sourcePath);
    bool hasAllSegmentsRecorded() const;
    bool hasAnySegmentRecorded() const;
    SegmentInfo *segmentByDisplayIndex(int displayIndex);
    const SegmentInfo *segmentByDisplayIndex(int displayIndex) const;

    SegmentModel m_segmentModel;
    AudioProject m_project;

    AudioFileDecoder *m_decoder;
    AudioPlaybackEngine *m_playback;
    RecordingEngine *m_recorder;
    ProjectSerializer *m_serializer;

    QString m_statusMessage;
    QString m_currentSourceName;
    QString m_reversedSongPath;
    bool m_sourceRecordingActive = false;
    bool m_projectReady = false;
    bool m_gluePlaybackActive = false;
    bool m_reversePlaybackActive = false;

    QSet<int> m_activeSegmentRecordings;
    QSet<int> m_activeOriginalPlayback;
    QSet<int> m_activeRecordedPlayback;
    QSet<int> m_activeReversePlayback;
};

