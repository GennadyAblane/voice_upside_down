#pragma once

#include "audio/audioproject.h"
#include "audio/segmentmodel.h"
#include "audio/volumeanalyzer.h"

#include <QObject>
#include <QSet>
#include <QVariant>

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
    Q_PROPERTY(bool isMicrophoneSource READ isMicrophoneSource NOTIFY sourceTypeChanged)
    Q_PROPERTY(bool canSaveResults READ canSaveResults NOTIFY saveStateChanged)
    Q_PROPERTY(bool reversePlaybackActive READ reversePlaybackActive NOTIFY reverseStateChanged)
    Q_PROPERTY(bool recordingDialogVisible READ recordingDialogVisible NOTIFY recordingDialogVisibleChanged)
    Q_PROPERTY(bool recordingReady READ recordingReady NOTIFY recordingReadyChanged)
    Q_PROPERTY(bool originalPlaybackEnabled READ originalPlaybackEnabled WRITE setOriginalPlaybackEnabled NOTIFY originalPlaybackEnabledChanged)
    Q_PROPERTY(double originalNoiseThreshold READ originalNoiseThreshold WRITE setOriginalNoiseThreshold NOTIFY volumeSettingsChanged)
    Q_PROPERTY(double segmentNoiseThreshold READ segmentNoiseThreshold WRITE setSegmentNoiseThreshold NOTIFY volumeSettingsChanged)
    Q_PROPERTY(double playbackPositionMs READ playbackPositionMs NOTIFY playbackPositionChanged)
    Q_PROPERTY(int activePlaybackSegmentIndex READ activePlaybackSegmentIndex NOTIFY playbackPositionChanged)
    Q_PROPERTY(bool isPlayingOriginalSegment READ isPlayingOriginalSegment NOTIFY playbackPositionChanged)

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
    bool recordingDialogVisible() const;
    bool recordingReady() const;
    bool isMicrophoneSource() const;
    bool canSaveResults() const;
    bool originalPlaybackEnabled() const;
    void setOriginalPlaybackEnabled(bool enabled);
    
    double originalNoiseThreshold() const;
    void setOriginalNoiseThreshold(double threshold);
    double segmentNoiseThreshold() const;
    void setSegmentNoiseThreshold(double threshold);
    
    double playbackPositionMs() const;
    int activePlaybackSegmentIndex() const;
    bool isPlayingOriginalSegment() const;

    Q_INVOKABLE void loadAudioSource(const QString &filePath);
    Q_INVOKABLE void startSourceRecording();
    Q_INVOKABLE void stopSourceRecording();
    Q_INVOKABLE void saveProjectTo(const QString &directoryPath);
    Q_INVOKABLE void openProjectFrom(const QString &projectFilePath);
    Q_INVOKABLE void setProjectName(const QString &name);
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
    Q_INVOKABLE void toggleSongReversePlayback();

    Q_INVOKABLE void saveProject();
    Q_INVOKABLE void stopCurrentRecording();
    
    // Analyze volume levels in the audio
    // Returns a list of objects with: startMs, endMs, rmsLevel, peakLevel, isQuiet, isLoud
    Q_INVOKABLE QVariantList analyzeVolume(int windowSizeMs = 100, double quietThreshold = 0.1, double loudThreshold = 0.7);
    
    // Get segment data for waveform visualization
    // Returns a list of objects with: startMs, endMs, index
    Q_INVOKABLE QVariantList getSegmentDataForWaveform();
    
    // Analyze volume levels for a specific segment recording
    // Returns a list of objects with: startMs, endMs, rmsLevel, peakLevel, isQuiet, isLoud
    Q_INVOKABLE QVariantList analyzeSegmentRecording(int segmentIndex, int windowSizeMs = 100);
    
    // Get segment start time in milliseconds
    Q_INVOKABLE double getSegmentStartMs(int segmentIndex);
    Q_INVOKABLE double getSegmentRecordingTrimmedStartMs(int segmentIndex);
    Q_INVOKABLE void recreateSegmentsFromBoundaries(const QVariantList &boundariesMs);
    Q_INVOKABLE QVariantMap getSegmentTrimBoundaries(int segmentIndex);
    Q_INVOKABLE void setSegmentTrimBoundaries(int segmentIndex, double trimStartMs, double trimEndMs);

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
    void recordingDialogVisibleChanged();
    void recordingReadyChanged();
    void sourceTypeChanged();
    void saveStateChanged();
    void originalPlaybackEnabledChanged();
    void volumeSettingsChanged();
    void playbackPositionChanged();

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
    bool m_recordingDialogVisible = false;
    bool m_recordingReady = false;
    bool m_originalPlaybackEnabled = false;
    
    // Noise threshold settings (0.0 to 1.0, RMS level)
    double m_originalNoiseThreshold = 0.1;  // Default: 10% RMS
    double m_segmentNoiseThreshold = 0.1;  // Default: 10% RMS

    QSet<int> m_activeSegmentRecordings;
    QSet<int> m_activeOriginalPlayback;
    QSet<int> m_activeRecordedPlayback;
    QSet<int> m_activeReversePlayback;
    
    // Playback position tracking
    int m_activePlaybackSegmentIndex = -1;
    bool m_isPlayingOriginalSegment = false;
};

