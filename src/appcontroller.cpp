#include "appcontroller.h"

#include "audio/audiofiledecoder.h"
#include "audio/audioplaybackengine.h"
#include "audio/audioproject.h"
#include "audio/audiobuffer.h"
#include "audio/recordingengine.h"
#include "audio/segmentmodel.h"
#include "audio/volumeanalyzer.h"
#include "audio/wavutils.h"
#include "persistence/projectserializer.h"
#include "utils/logger.h"
#include "utils/pathutils.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QtGlobal>

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_project(this)
    , m_decoder(new AudioFileDecoder(this))
    , m_playback(new AudioPlaybackEngine(this))
    , m_recorder(new RecordingEngine(this))
    , m_serializer(new ProjectSerializer(this))
{
    m_segmentModel.setProject(&m_project);
    setStatusMessage(tr("Готово"));

    // Pre-initialize audio input device to reduce delay when starting recording
    QAudioFormat format;
    format.setChannelCount(1);
    format.setSampleRate(44100);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setByteOrder(QAudioFormat::LittleEndian);
    m_recorder->prepare(format);

    // Connect playback finished signals to update UI states
    connect(m_playback, &AudioPlaybackEngine::playbackFinished, this, [this]() {
        clearPlaybackStates();
        setStatusMessage(tr("Воспроизведение завершено"));
        LOG_INFO() << "Playback finished";
    });
    connect(m_playback, &AudioPlaybackEngine::playbackError, this, [this](const QString &error) {
        setStatusMessage(error);
        LOG_WARN() << "Playback error:" << error;
        // Note: QMessageBox cannot be used in QML application (QGuiApplication), only logging
    });
    
    // Connect playback position updates
    connect(m_playback, &AudioPlaybackEngine::playbackPositionChanged, this, &AppController::playbackPositionChanged);

    // Connect recording engine signals
    connect(m_recorder, &RecordingEngine::recordingReady, this, [this]() {
        LOG_INFO() << "Recording ready signal received in AppController, sourceRecordingActive:" << m_sourceRecordingActive 
                   << "activeSegmentRecordings:" << m_activeSegmentRecordings.size();
        m_recordingReady = true;
        emit recordingReadyChanged();
        LOG_INFO() << "recordingReady property set to true, signal emitted, dialogVisible:" << m_recordingDialogVisible;
    });
    connect(m_recorder, &RecordingEngine::recordingStopped, this, [this]() {
        // Handle source recording completion
        // Check m_sourceRecordingActive BEFORE resetting it
        bool wasSourceRecording = m_sourceRecordingActive;
        if (wasSourceRecording) {
            // Reset flag now that we're handling the completion
            m_sourceRecordingActive = false;
            emit sourceRecordingChanged();
            
            // Load the recorded file
            const QString filePath = PathUtils::defaultTempRoot() + QDir::separator() + QStringLiteral("source_recording.wav");
            if (QFileInfo::exists(filePath)) {
                loadAudioSource(filePath);
                setStatusMessage(tr("Запись завершена и загружена"));
                LOG_INFO() << "Source recording stopped and loaded";
                // Emit signals for source type and save state changes
                emit sourceTypeChanged();
                emit saveStateChanged();
            } else {
                setStatusMessage(tr("Запись остановлена"));
                LOG_WARN() << "Source recording stopped but file not found:" << filePath;
            }
        }
        // Note: Dialog is already hidden in stopSourceRecording() and toggleSegmentRecording()
        // Note: Segment recording completion is handled in toggleSegmentRecording
    });
}

AppController::~AppController()
{
}

SegmentModel *AppController::segmentModel()
{
    return &m_segmentModel;
}

int AppController::segmentLength() const
{
    return m_project.segmentLengthSeconds();
}

QString AppController::statusMessage() const
{
    return m_statusMessage;
}

bool AppController::projectReady() const
{
    return m_projectReady;
}

QString AppController::currentSourceName() const
{
    return m_currentSourceName;
}

bool AppController::sourceRecording() const
{
    return m_sourceRecordingActive;
}

bool AppController::canAdjustSegmentLength() const
{
    return !m_project.segments().isEmpty();
}

QString AppController::segmentHintText() const
{
    return tr("Сегментов: %1").arg(m_project.segments().count());
}

bool AppController::interactionsEnabled() const
{
    return m_projectReady;
}

bool AppController::canGlue() const
{
    return hasAllSegmentsRecorded();
}

bool AppController::canPlayGlue() const
{
    return hasAllSegmentsRecorded();
}

bool AppController::gluePlaybackActive() const
{
    return m_gluePlaybackActive;
}

bool AppController::canReverseSong() const
{
    // Reverse song button is no longer needed - reverse is created automatically during glue
    return false;
}

bool AppController::canPlayReverse() const
{
    // Reverse is created automatically during glue, so check if it exists
    return !m_reversedSongPath.isEmpty() && QFileInfo::exists(m_reversedSongPath);
}

bool AppController::reversePlaybackActive() const
{
    return m_reversePlaybackActive;
}

bool AppController::recordingDialogVisible() const
{
    return m_recordingDialogVisible;
}

bool AppController::recordingReady() const
{
    return m_recordingReady;
}

bool AppController::isMicrophoneSource() const
{
    if (!m_projectReady)
        return false;
    
    const QString originalPath = m_project.originalFilePath();
    if (originalPath.isEmpty())
        return false;
    
    QFileInfo originalInfo(originalPath);
    return originalInfo.fileName() == QStringLiteral("source_recording.wav");
}

bool AppController::canSaveResults() const
{
    if (!m_projectReady)
        return false;
    
    // If source is from microphone, button is always enabled
    if (isMicrophoneSource())
        return true;
    
    // If source is loaded file, button is enabled only if reverse is ready
    return !m_reversedSongPath.isEmpty() && QFileInfo::exists(m_reversedSongPath);
}

bool AppController::originalPlaybackEnabled() const
{
    return m_originalPlaybackEnabled;
}

void AppController::setOriginalPlaybackEnabled(bool enabled)
{
    if (m_originalPlaybackEnabled == enabled)
        return;
    
    m_originalPlaybackEnabled = enabled;
    emit originalPlaybackEnabledChanged();
}

double AppController::originalNoiseThreshold() const
{
    return m_originalNoiseThreshold;
}

void AppController::setOriginalNoiseThreshold(double threshold)
{
    threshold = qBound(0.0, threshold, 1.0);
    if (qAbs(m_originalNoiseThreshold - threshold) < 0.001)
        return;
    
    m_originalNoiseThreshold = threshold;
    emit volumeSettingsChanged();
}

double AppController::segmentNoiseThreshold() const
{
    return m_segmentNoiseThreshold;
}

void AppController::setSegmentNoiseThreshold(double threshold)
{
    threshold = qBound(0.0, threshold, 1.0);
    // Убираем проверку на минимальное изменение, чтобы сигнал эмитился всегда
    // Это необходимо для обновления waveform при каждом изменении порога
    if (qAbs(m_segmentNoiseThreshold - threshold) < 0.000000000000000001)
        return;
    
    m_segmentNoiseThreshold = threshold;
    emit volumeSettingsChanged();
}

double AppController::playbackPositionMs() const
{
    return m_playback->playbackPositionMs();
}

int AppController::activePlaybackSegmentIndex() const
{
    return m_activePlaybackSegmentIndex;
}

bool AppController::isPlayingOriginalSegment() const
{
    return m_isPlayingOriginalSegment;
}

void AppController::loadAudioSource(const QString &filePath)
{
    if (filePath.isEmpty()) {
        LOG_WARN() << "loadAudioSource called with empty path";
        return;
    }

    LOG_INFO() << "Loading audio source from" << filePath;

    AudioBuffer buffer;
    QString error;
    if (!m_decoder->decodeFile(filePath, buffer, &error)) {
        LOG_WARN() << "Decoding failed for" << filePath << ":" << error;
        setStatusMessage(error);
        return;
    }

    // Delete old reverse files before loading new source
    const QString cutsDir = PathUtils::defaultCutsRoot();
    if (QDir(cutsDir).exists()) {
        QDir dir(cutsDir);
        const QStringList filters = QStringList() << "segment_*_reverse.wav";
        const QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
        for (const QFileInfo &fileInfo : files) {
            QFile::remove(fileInfo.absoluteFilePath());
            LOG_INFO() << "Removed old reverse file:" << fileInfo.absoluteFilePath();
        }
    }

    m_project.originalBuffer() = buffer;
    m_project.setOriginalFilePath(filePath);
    ensureProjectNameFromSource(filePath);
    m_project.splitIntoSegments();
    m_project.resetSegmentStatuses();

    m_projectReady = true;
    m_currentSourceName = QFileInfo(filePath).fileName();

    emit currentSourceNameChanged();
    emit projectReadinessChanged();
    emit canAdjustSegmentLengthChanged();
    emit segmentHintTextChanged();
    emit interactionsStateChanged();
    emit sourceTypeChanged();
    emit saveStateChanged();

    setStatusMessage(tr("Файл загружен: %1").arg(m_currentSourceName));
    LOG_INFO() << "Audio source loaded successfully:" << m_currentSourceName << "segments:" << m_project.segments().size();
}

void AppController::startSourceRecording()
{
    if (m_sourceRecordingActive) {
        // If already recording, stop it (same as clicking stop button)
        stopSourceRecording();
        return;
    }

    const QString tempDir = PathUtils::defaultTempRoot();
    PathUtils::ensureDirectory(tempDir);
    const QString filePath = tempDir + QDir::separator() + QStringLiteral("source_recording.wav");

    QAudioFormat format;
    format.setChannelCount(1);
    format.setSampleRate(44100);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setByteOrder(QAudioFormat::LittleEndian);

    // Show dialog and reset ready state BEFORE starting recording
    // This ensures dialog is visible when recording starts
    m_recordingDialogVisible = true;
    m_recordingReady = false;
    emit recordingDialogVisibleChanged();
    emit recordingReadyChanged();
    LOG_INFO() << "Dialog shown, recordingReady set to false";

    if (!m_recorder->startRecording(filePath, format)) {
        // Hide dialog if recording failed
        m_recordingDialogVisible = false;
        emit recordingDialogVisibleChanged();
        setStatusMessage(tr("Ошибка начала записи"));
        LOG_WARN() << "Failed to start source recording";
        return;
    }

    m_sourceRecordingActive = true;
    emit sourceRecordingChanged();
    setStatusMessage(tr("Инициализация записи..."));
    LOG_INFO() << "Source recording initialization started to" << filePath << "recordingReady:" << m_recordingReady;
}

void AppController::stopSourceRecording()
{
    if (!m_sourceRecordingActive) {
        LOG_WARN() << "stopSourceRecording requested, but recording not active";
        return;
    }

    // Hide dialog immediately
    m_recordingDialogVisible = false;
    m_recordingReady = false;
    emit recordingDialogVisibleChanged();
    emit recordingReadyChanged();

    // Stop recording (will wait 0.25 seconds to capture tail, then emit recordingStopped)
    // NOTE: Don't set m_sourceRecordingActive = false here!
    // We need it to be true when recordingStopped signal arrives
    m_recorder->stop();
    emit sourceRecordingChanged();
    setStatusMessage(tr("Завершение записи..."));
    LOG_INFO() << "Source recording stop requested";
}

void AppController::setProjectName(const QString &name)
{
    if (name.isEmpty())
        return;
    
    m_project.setProjectName(PathUtils::cleanName(name));
    LOG_INFO() << "Project name set to:" << m_project.projectName();
}

void AppController::saveProjectTo(const QString &filePath)
{
    if (!m_projectReady)
        return;

    // filePath is the full path to WAV file where we'll save
    // Extract directory and file name
    QFileInfo fileInfo(filePath);
    QString directoryPath = fileInfo.absolutePath();
    QString fileName = fileInfo.completeBaseName(); // name without extension
    QString fileExtension = fileInfo.suffix();
    
    LOG_INFO() << "Saving results to" << filePath;
    
    // Check if source is from microphone recording
    bool isMicSource = isMicrophoneSource();
    const QString originalPath = m_project.originalFilePath();
    bool reverseReady = !m_reversedSongPath.isEmpty() && QFileInfo::exists(m_reversedSongPath);
    
    bool savedSomething = false;
    QStringList savedFiles;
    
    // Case 1: Microphone source + reverse NOT ready → save only microphone recording
    if (isMicSource && !reverseReady) {
        QString micSavePath = directoryPath + QDir::separator() + fileName + QStringLiteral(".") + fileExtension;
        if (QFileInfo::exists(originalPath)) {
            if (QFileInfo::exists(micSavePath)) {
                QFile::remove(micSavePath);
            }
            if (QFile::copy(originalPath, micSavePath)) {
                savedFiles << micSavePath;
                savedSomething = true;
                LOG_INFO() << "Saved microphone recording to" << micSavePath;
            } else {
                LOG_WARN() << "Failed to copy microphone recording from" << originalPath << "to" << micSavePath;
            }
        }
    }
    // Case 2: Microphone source + reverse ready → save both files
    else if (isMicSource && reverseReady) {
        // Save microphone recording
        QString micSavePath = directoryPath + QDir::separator() + fileName + QStringLiteral(".") + fileExtension;
        if (QFileInfo::exists(originalPath)) {
            if (QFileInfo::exists(micSavePath)) {
                QFile::remove(micSavePath);
            }
            if (QFile::copy(originalPath, micSavePath)) {
                savedFiles << micSavePath;
                savedSomething = true;
                LOG_INFO() << "Saved microphone recording to" << micSavePath;
            } else {
                LOG_WARN() << "Failed to copy microphone recording from" << originalPath << "to" << micSavePath;
            }
        }
        
        // Save reversed file
        QString reversedFileName = fileName;
        if (!reversedFileName.endsWith(QStringLiteral("rev"), Qt::CaseInsensitive)) {
            reversedFileName = reversedFileName + QStringLiteral("rev");
        }
        QString reversedFilePath = directoryPath + QDir::separator() + reversedFileName + QStringLiteral(".") + fileExtension;
        if (QFileInfo::exists(reversedFilePath)) {
            QFile::remove(reversedFilePath);
        }
        if (QFile::copy(m_reversedSongPath, reversedFilePath)) {
            savedFiles << reversedFilePath;
            savedSomething = true;
            LOG_INFO() << "Saved reversed song to" << reversedFilePath;
        } else {
            LOG_WARN() << "Failed to copy reversed song from" << m_reversedSongPath << "to" << reversedFilePath;
        }
    }
    // Case 3: Loaded file + reverse ready → save only reverse
    else if (!isMicSource && reverseReady) {
        QString reversedFileName = fileName;
        if (!reversedFileName.endsWith(QStringLiteral("rev"), Qt::CaseInsensitive)) {
            reversedFileName = reversedFileName + QStringLiteral("rev");
        }
        QString reversedFilePath = directoryPath + QDir::separator() + reversedFileName + QStringLiteral(".") + fileExtension;
        if (QFileInfo::exists(reversedFilePath)) {
            QFile::remove(reversedFilePath);
        }
        if (QFile::copy(m_reversedSongPath, reversedFilePath)) {
            savedFiles << reversedFilePath;
            savedSomething = true;
            LOG_INFO() << "Saved reversed song to" << reversedFilePath;
        } else {
            LOG_WARN() << "Failed to copy reversed song from" << m_reversedSongPath << "to" << reversedFilePath;
        }
    }
    // Case 4: Loaded file + reverse NOT ready → should not happen (button disabled)
    else {
        setStatusMessage(tr("Ошибка: нет данных для сохранения"));
        LOG_WARN() << "Cannot save: loaded file but reverse not ready";
        return;
    }
    
    if (savedSomething) {
        QString filesList = savedFiles.join(QStringLiteral(", "));
        setStatusMessage(tr("Результаты сохранены: %1").arg(filesList));
        LOG_INFO() << "Results saved successfully:" << filesList;
    } else {
        setStatusMessage(tr("Ошибка сохранения: не удалось сохранить файлы"));
        LOG_WARN() << "Failed to save any files";
    }
}

void AppController::openProjectFrom(const QString &projectFilePath)
{
    LOG_INFO() << "Opening project from" << projectFilePath;
    
    // Convert URL to local file path if needed
    QString localPath = projectFilePath;
    if (localPath.startsWith(QStringLiteral("file://"))) {
        QUrl url(localPath);
        if (url.isLocalFile()) {
            localPath = url.toLocalFile();
        } else {
            // Fallback: remove file:// prefix manually
            localPath = localPath.mid(7); // Remove "file://"
            // Remove leading slashes
            while (localPath.startsWith(QStringLiteral("/"))) {
                localPath = localPath.mid(1);
            }
        }
        LOG_INFO() << "Converted URL to local path:" << localPath;
    }
    
    // If .vups file is provided, find project.json in the project directory
    QString actualProjectPath = localPath;
    QFileInfo fileInfo(localPath);
    if (fileInfo.suffix().toLower() == QStringLiteral("vups")) {
        // .vups file points to project directory
        QString projectName = fileInfo.completeBaseName();
        QString projectDir = fileInfo.absolutePath() + QDir::separator() + projectName;
        QString projectJsonPath = projectDir + QDir::separator() + QStringLiteral("project.json");
        
        if (QFileInfo::exists(projectJsonPath)) {
            actualProjectPath = projectJsonPath;
            LOG_INFO() << "Found project.json at" << actualProjectPath;
        } else {
            // If project.json not found in directory, try to use .vups file directly
            // (in case it contains the project data)
            actualProjectPath = projectFilePath;
            LOG_INFO() << "Using .vups file directly as project file";
        }
    }
    
    QString info;
    if (!m_serializer->load(actualProjectPath, m_project, &info)) {
        setStatusMessage(info);
        LOG_WARN() << "Failed to open project:" << info;
        return;
    }

    // Load original audio file if it exists
    const QString originalPath = m_project.originalFilePath();
    if (!originalPath.isEmpty() && QFileInfo::exists(originalPath)) {
        AudioBuffer buffer;
        QString error;
        if (m_decoder->decodeFile(originalPath, buffer, &error)) {
            m_project.originalBuffer() = buffer;
            LOG_INFO() << "Original audio loaded from project";
        } else {
            LOG_WARN() << "Failed to load original audio:" << error;
        }
    }

    // Load glued song and reversed song if they exist in project directory
    QFileInfo projectFileInfo(actualProjectPath);
    QDir projectDir = projectFileInfo.absoluteDir();
    QString projectName = m_project.projectName();
    
    // If project.json is in a subdirectory (project name directory), use that
    // Otherwise, use the directory containing project.json
    if (projectFileInfo.fileName() == QStringLiteral("project.json")) {
        // project.json is in the project directory
        // projectDir is already correct
    } else {
        // .vups file was used, project directory is a subdirectory
        QString projectNameFromFile = projectFileInfo.completeBaseName();
        QString possibleProjectDir = projectFileInfo.absolutePath() + QDir::separator() + projectNameFromFile;
        if (QFileInfo::exists(possibleProjectDir)) {
            projectDir = QDir(possibleProjectDir);
        }
    }
    
    // Try to find glued song files
    QString fragmentSongPath = projectDir.absoluteFilePath(projectName + QStringLiteral("_fragment_song.wav"));
    if (QFileInfo::exists(fragmentSongPath)) {
        m_project.setDecodedFilePath(fragmentSongPath);
        LOG_INFO() << "Found glued song at" << fragmentSongPath;
    } else {
        LOG_INFO() << "Glued song file not found at" << fragmentSongPath;
    }
    
    QString reversFragmentSongPath = projectDir.absoluteFilePath(projectName + QStringLiteral("_revers_fragment_song.wav"));
    if (QFileInfo::exists(reversFragmentSongPath)) {
        m_reversedSongPath = reversFragmentSongPath;
        LOG_INFO() << "Found reversed glued song at" << reversFragmentSongPath;
    } else {
        LOG_INFO() << "Reversed song file not found at" << reversFragmentSongPath;
    }

    // Segments are already loaded from JSON, no need to re-split
    // Just emit update signal
    emit m_project.segmentsUpdated();

    m_projectReady = true;
    m_currentSourceName = m_project.projectName().isEmpty() ? QFileInfo(projectFilePath).completeBaseName() : m_project.projectName();
    emit currentSourceNameChanged();
    emit projectReadinessChanged();
    emit canAdjustSegmentLengthChanged();
    emit segmentHintTextChanged();
    emit glueStateChanged();
    emit reverseStateChanged();
    emit sourceTypeChanged();
    emit saveStateChanged();

    setStatusMessage(tr("Проект открыт: %1").arg(m_currentSourceName));
    LOG_INFO() << "Project opened successfully:" << m_currentSourceName;
}

void AppController::changeSegmentLength(int seconds)
{
    if (seconds == m_project.segmentLengthSeconds())
        return;

    LOG_INFO() << "Changing segment length to" << seconds;
    bool hadRecordings = hasAnySegmentRecorded();
    
    // Delete all existing reverse files before changing segment length
    // because segments will be recreated with new frame counts
    const QString cutsDir = PathUtils::defaultCutsRoot();
    if (QDir(cutsDir).exists()) {
        const QStringList filters = QStringList() << "segment_*_reverse.wav";
        const QFileInfoList files = QDir(cutsDir).entryInfoList(filters, QDir::Files);
        for (const QFileInfo &fileInfo : files) {
            if (QFile::remove(fileInfo.absoluteFilePath())) {
                LOG_INFO() << "Removed old reverse file due to segment length change:" << fileInfo.absoluteFilePath();
            }
        }
    }
    
    m_project.setSegmentLengthSeconds(seconds);
    emit segmentLengthChanged();
    emit segmentHintTextChanged();
    if (hadRecordings) {
        setStatusMessage(tr("Длина сегмента изменена на %1 c. Записи сегментов сброшены.").arg(seconds));
    } else {
        setStatusMessage(tr("Новая длина сегмента: %1 c").arg(seconds));
    }
}

bool AppController::isSegmentRecording(int segmentIndex) const
{
    return m_activeSegmentRecordings.contains(segmentIndex);
}

bool AppController::isSegmentOriginalPlaying(int segmentIndex) const
{
    return m_activeOriginalPlayback.contains(segmentIndex);
}

bool AppController::isSegmentRecordingPlaying(int segmentIndex) const
{
    return m_activeRecordedPlayback.contains(segmentIndex);
}

bool AppController::isSegmentReversePlaying(int segmentIndex) const
{
    return m_activeReversePlayback.contains(segmentIndex);
}

void AppController::toggleSegmentRecording(int segmentIndex)
{
    if (m_activeSegmentRecordings.contains(segmentIndex)) {
        // Hide dialog immediately
        m_recordingDialogVisible = false;
        m_recordingReady = false;
        emit recordingDialogVisibleChanged();
        emit recordingReadyChanged();

        // Stop recording (will wait 0.25 seconds to capture tail, then emit recordingStopped)
        // Don't remove from active set yet - stopCurrentRecording needs it
        m_recorder->stop();
        setStatusMessage(tr("Завершение записи сегмента %1...").arg(segmentIndex));
        
        // Connect to recordingStopped to finalize segment recording
        // We need to track which segment is being finalized
        // Use a lambda that captures segmentIndex and disconnects itself
        QMetaObject::Connection *connection = new QMetaObject::Connection();
        *connection = connect(m_recorder, &RecordingEngine::recordingStopped, this, [this, segmentIndex, connection]() {
            // Disconnect immediately to ensure this is only called once
            disconnect(*connection);
            delete connection;
            
            // Now remove from active set
            m_activeSegmentRecordings.remove(segmentIndex);
            
            auto *segment = segmentByDisplayIndex(segmentIndex);
            if (segment && QFileInfo::exists(segment->recordingPath)) {
                segment->hasRecording = true;
                emit m_project.segmentsUpdated();
                setStatusMessage(tr("Запись сегмента %1 завершена").arg(segmentIndex));
                LOG_INFO() << "Segment recording stopped for index" << segmentIndex << "saved to" << segment->recordingPath;
            } else {
                setStatusMessage(tr("Ошибка сохранения записи сегмента %1").arg(segmentIndex));
                LOG_WARN() << "Segment recording file not found:" << (segment ? segment->recordingPath : QString());
            }
            
            emit glueStateChanged();
        });
        
        return;
    }

    // Stop all other playback/recording
    m_playback->stopAll();
    // Stop recorder if it's recording
    // Note: stop() will wait 0.25s for tail capture, but we need to start new recording
    // So we just call stop() and let it handle cleanup in background
    if (m_recorder->isRecording()) {
        m_recorder->stop();
        LOG_INFO() << "Stopped previous recording before starting segment recording";
    }
    m_activeOriginalPlayback.clear();
    m_activeRecordedPlayback.clear();
    m_activeReversePlayback.clear();

    // Get segment and prepare recording path
    auto *segment = segmentByDisplayIndex(segmentIndex);
    if (!segment) {
        LOG_WARN() << "Segment not found for index" << segmentIndex;
        return;
    }

    const QString cutsDir = PathUtils::defaultCutsRoot();
    PathUtils::ensureDirectory(cutsDir);
    segment->recordingPath = PathUtils::composeSegmentFile(cutsDir, segmentIndex);

    // Delete old recording if exists (for re-recording)
    if (QFileInfo::exists(segment->recordingPath)) {
        QFile::remove(segment->recordingPath);
        LOG_INFO() << "Removed old recording file for segment" << segmentIndex;
    }
    
    // Delete old reverse file if exists
    if (!segment->reversePath.isEmpty() && QFileInfo::exists(segment->reversePath)) {
        QFile::remove(segment->reversePath);
        LOG_INFO() << "Removed old reverse file for segment" << segmentIndex;
    }
    
    // Reset segment recording state
    segment->hasRecording = false;
    segment->hasReverse = false;
    segment->reversePath.clear();
    emit m_project.segmentsUpdated();

    // Use the same format as original buffer
    QAudioFormat format = m_project.originalBuffer().format();
    if (!format.isValid()) {
        format.setChannelCount(1);
        format.setSampleRate(44100);
        format.setSampleSize(16);
        format.setSampleType(QAudioFormat::SignedInt);
        format.setCodec(QStringLiteral("audio/pcm"));
        format.setByteOrder(QAudioFormat::LittleEndian);
    }

    // Show dialog BEFORE starting recording (same as source recording)
    m_recordingDialogVisible = true;
    m_recordingReady = false;
    emit recordingDialogVisibleChanged();
    emit recordingReadyChanged();
    LOG_INFO() << "Dialog shown for segment recording, recordingReady set to false";

    if (!m_recorder->startRecording(segment->recordingPath, format)) {
        // Hide dialog if recording failed
        m_recordingDialogVisible = false;
        emit recordingDialogVisibleChanged();
        setStatusMessage(tr("Ошибка начала записи сегмента %1").arg(segmentIndex));
        LOG_WARN() << "Failed to start recording for segment" << segmentIndex;
        return;
    }

    m_activeSegmentRecordings.insert(segmentIndex);
    setStatusMessage(tr("Инициализация записи сегмента %1...").arg(segmentIndex));
    LOG_INFO() << "Segment recording initialization started for index" << segmentIndex << "to" << segment->recordingPath << "recordingReady:" << m_recordingReady;
}

// Helper function to trim noise from segment start and end
// If trimStartMs and trimEndMs are >= 0, they are used instead of automatic detection
static QByteArray trimSegmentNoise(const QByteArray &pcm, const QAudioFormat &format, double noiseThreshold, double trimStartMs = -1.0, double trimEndMs = -1.0)
{
    if (pcm.isEmpty() || !format.isValid()) {
        return pcm;
    }
    
    // Create AudioBuffer from PCM data
    AudioBuffer buffer;
    buffer.setFormat(format);
    buffer.data() = pcm;
    
    if (buffer.frameCount() == 0) {
        return pcm;
    }
    
    // Analyze volume with 100ms windows
    const QVector<VolumeLevel> levels = VolumeAnalyzer::analyzeVolume(
        buffer, 100, noiseThreshold, 0.7);
    
    if (levels.isEmpty()) {
        return pcm;
    }
    
    const qint64 sampleRate = format.sampleRate();
    if (sampleRate <= 0) {
        return pcm;
    }
    
    qint64 startFrame = 0;
    qint64 endFrame = buffer.frameCount();
    
    // Use manual trim boundaries if provided
    if (trimStartMs >= 0 && trimEndMs >= 0 && trimStartMs < trimEndMs) {
        startFrame = static_cast<qint64>((trimStartMs * sampleRate) / 1000.0);
        endFrame = static_cast<qint64>((trimEndMs * sampleRate) / 1000.0);
        startFrame = qBound(0LL, startFrame, buffer.frameCount());
        endFrame = qBound(0LL, endFrame, buffer.frameCount());
        LOG_INFO() << "Using manual trim boundaries: startMs:" << trimStartMs << "endMs:" << trimEndMs 
                   << "startFrame:" << startFrame << "endFrame:" << endFrame;
    } else {
        // Automatic detection
        LOG_INFO() << "Trimming segment: total levels:" << levels.size() 
                   << "noiseThreshold:" << noiseThreshold
                   << "totalFrames:" << buffer.frameCount();
        
        // Log first few and last few levels for debugging
        for (int i = 0; i < qMin(5, levels.size()); ++i) {
            const VolumeLevel &level = levels[i];
            LOG_INFO() << "Level" << i << "startFrame:" << level.startFrame 
                       << "frameCount:" << level.frameCount 
                       << "rms:" << level.rmsLevel 
                       << "isQuiet:" << level.isQuiet 
                       << "isLoud:" << level.isLoud;
        }
        if (levels.size() > 5) {
            for (int i = qMax(5, levels.size() - 5); i < levels.size(); ++i) {
                const VolumeLevel &level = levels[i];
                LOG_INFO() << "Level" << i << "startFrame:" << level.startFrame 
                           << "frameCount:" << level.frameCount 
                           << "rms:" << level.rmsLevel 
                           << "isQuiet:" << level.isQuiet 
                           << "isLoud:" << level.isLoud;
            }
        }
        
        // Find start of real sound
        // Logic: find first loud section (any length), trim everything before it
        // (all quiet sections, regardless of their length - 1ms or 10000ms, doesn't matter)
        for (int i = 0; i < levels.size(); ++i) {
            const VolumeLevel &level = levels[i];
            if (!level.isQuiet) {
                // Found first loud section (any length) - this is the real start
                // Trim everything before it (all quiet sections, regardless of length)
                startFrame = level.startFrame;
                LOG_INFO() << "Found start at frame" << startFrame << "from loud section" << i 
                           << "frames:" << level.frameCount << "rms:" << level.rmsLevel;
                break;
            }
            // Quiet section - continue searching, will be trimmed regardless of length
        }
        
        // Find end of real sound
        // Logic: find last loud section (any length), trim everything after it
        // (all quiet sections, regardless of their length - 1ms or 10000ms, doesn't matter)
        for (int i = levels.size() - 1; i >= 0; --i) {
            const VolumeLevel &level = levels[i];
            if (!level.isQuiet) {
                // Found last loud section (any length) - this is the real end
                // Trim everything after it (all quiet sections, regardless of length)
                endFrame = level.startFrame + level.frameCount;
                LOG_INFO() << "Found end at frame" << endFrame << "from loud section" << i 
                           << "frames:" << level.frameCount << "rms:" << level.rmsLevel;
                break;
            }
            // Quiet section - continue searching backwards, will be trimmed regardless of length
        }
    }
    
    // Ensure start and end don't overlap
    if (startFrame >= endFrame) {
        // Overlap detected - adjust to prevent it
        if (startFrame > 0 && endFrame < buffer.frameCount()) {
            // Both were found, but they overlap - use middle point
            qint64 middleFrame = (startFrame + endFrame) / 2;
            startFrame = qMin(startFrame, middleFrame);
            endFrame = qMax(endFrame, middleFrame + 1);
            LOG_WARN() << "Start and end overlapped, adjusted to startFrame:" << startFrame 
                       << "endFrame:" << endFrame;
        } else {
            // No valid sound found, return empty
            LOG_WARN() << "No valid sound found in segment after trimming, startFrame:" 
                       << startFrame << "endFrame:" << endFrame;
            return QByteArray();
        }
    }
    
    const qint64 trimmedFrameCount = endFrame - startFrame;
    QByteArray trimmed = buffer.sliceFrames(startFrame, trimmedFrameCount);
    
    LOG_INFO() << "Trimmed segment: original frames:" << buffer.frameCount() 
               << "trimmed frames:" << trimmedFrameCount 
               << "removed from start:" << startFrame << "frames"
               << "removed from end:" << (buffer.frameCount() - endFrame) << "frames";
    
    return trimmed;
}

void AppController::toggleSegmentOriginalPlayback(int segmentIndex)
{
    if (m_activeOriginalPlayback.contains(segmentIndex)) {
        m_playback->stopAll();
        m_activeOriginalPlayback.remove(segmentIndex);
        m_activePlaybackSegmentIndex = -1;
        m_isPlayingOriginalSegment = false;
        emit playbackPositionChanged();
        setStatusMessage(tr("Оригинал сегмента %1 остановлен").arg(segmentIndex));
        LOG_INFO() << "Stopped original playback for segment" << segmentIndex;
        emit m_project.segmentsUpdated();
        return;
    }

    // Stop all other playback/recording
    m_playback->stopAll();
    m_recorder->stop();
    m_activeRecordedPlayback.clear();
    m_activeReversePlayback.clear();
    m_activeOriginalPlayback.clear();
    // Update UI immediately so buttons return to original state
    emit m_project.segmentsUpdated();

    const auto *segment = segmentByDisplayIndex(segmentIndex);
    if (!segment) {
        LOG_WARN() << "Segment not found for index" << segmentIndex;
        return;
    }

    // Extract segment from original buffer
    const QByteArray segmentData = m_project.originalBuffer().sliceFrames(segment->startFrame, segment->frameCount);
    if (segmentData.isEmpty()) {
        setStatusMessage(tr("Ошибка: сегмент %1 пуст").arg(segmentIndex));
        LOG_WARN() << "Empty segment data for index" << segmentIndex;
        return;
    }

    if (m_playback->playBuffer(segmentData, m_project.originalBuffer().format())) {
        m_activeOriginalPlayback.insert(segmentIndex);
        m_activePlaybackSegmentIndex = segmentIndex;
        m_isPlayingOriginalSegment = true;
        emit playbackPositionChanged();
        setStatusMessage(tr("Воспроизведение оригинала сегмента %1").arg(segmentIndex));
        LOG_INFO() << "Started original playback for segment" << segmentIndex;
        emit m_project.segmentsUpdated();
    } else {
        setStatusMessage(tr("Ошибка воспроизведения оригинала сегмента %1").arg(segmentIndex));
        LOG_WARN() << "Failed to play original segment" << segmentIndex;
    }
}

void AppController::toggleSegmentRecordedPlayback(int segmentIndex)
{
    if (m_activeRecordedPlayback.contains(segmentIndex)) {
        m_playback->stopAll();
        m_activeRecordedPlayback.remove(segmentIndex);
        m_activePlaybackSegmentIndex = -1;
        m_isPlayingOriginalSegment = false;
        emit playbackPositionChanged();
        setStatusMessage(tr("Запись сегмента %1 остановлена").arg(segmentIndex));
        LOG_INFO() << "Stopped recorded playback for segment" << segmentIndex;
        emit m_project.segmentsUpdated();
        return;
    }

    // Stop all other playback/recording
    m_playback->stopAll();
    m_recorder->stop();
    m_activeOriginalPlayback.clear();
    m_activeReversePlayback.clear();
    m_activeRecordedPlayback.clear();
    // Update UI immediately so buttons return to original state
    emit m_project.segmentsUpdated();

    const auto *segment = segmentByDisplayIndex(segmentIndex);
    if (!segment || !segment->hasRecording || segment->recordingPath.isEmpty()) {
        setStatusMessage(tr("Сегмент %1 не записан").arg(segmentIndex));
        LOG_WARN() << "Segment" << segmentIndex << "not recorded";
        return;
    }

    if (!QFileInfo::exists(segment->recordingPath)) {
        setStatusMessage(tr("Файл записи сегмента %1 не найден").arg(segmentIndex));
        LOG_WARN() << "Recording file not found:" << segment->recordingPath;
        return;
    }

    // Load and trim the recording (same as in glueSegments)
    QByteArray pcm;
    QAudioFormat format;
    QString error;
    if (!WavUtils::readWavFile(segment->recordingPath, pcm, format, &error)) {
        setStatusMessage(tr("Ошибка чтения записи сегмента %1: %2").arg(segmentIndex).arg(error));
        LOG_WARN() << "Failed to read segment recording:" << segment->recordingPath << error;
        return;
    }

    // Trim noise from start and end (use manual boundaries if set)
    double trimStartMs = segment->trimStartMs;
    double trimEndMs = segment->trimEndMs;
    QByteArray trimmedPcm = trimSegmentNoise(pcm, format, m_segmentNoiseThreshold, trimStartMs, trimEndMs);
    if (trimmedPcm.isEmpty()) {
        setStatusMessage(tr("Ошибка: сегмент %1 не содержит звука после обрезки").arg(segmentIndex));
        LOG_WARN() << "Segment" << segmentIndex << "is empty after trimming";
        return;
    }

    // Play the trimmed audio
    if (m_playback->playBuffer(trimmedPcm, format)) {
        m_activeRecordedPlayback.insert(segmentIndex);
        m_activePlaybackSegmentIndex = segmentIndex;
        m_isPlayingOriginalSegment = false;
        emit playbackPositionChanged();
        setStatusMessage(tr("Воспроизведение записи сегмента %1 (обрезано)").arg(segmentIndex));
        LOG_INFO() << "Started recorded playback for segment" << segmentIndex 
                   << "original size:" << pcm.size() << "trimmed size:" << trimmedPcm.size();
        emit m_project.segmentsUpdated();
    } else {
        setStatusMessage(tr("Ошибка воспроизведения записи сегмента %1").arg(segmentIndex));
        LOG_WARN() << "Failed to play recorded segment" << segmentIndex;
    }
}

void AppController::toggleSegmentReversePlayback(int segmentIndex)
{
    if (m_activeReversePlayback.contains(segmentIndex)) {
        m_playback->stopAll();
        m_activeReversePlayback.remove(segmentIndex);
        setStatusMessage(tr("Реверс сегмента %1 остановлен").arg(segmentIndex));
        LOG_INFO() << "Stopped reverse playback for segment" << segmentIndex;
        emit m_project.segmentsUpdated();
        return;
    }

    // Stop all other playback/recording
    m_playback->stopAll();
    m_recorder->stop();
    m_activeOriginalPlayback.clear();
    m_activeRecordedPlayback.clear();
    m_activeReversePlayback.clear();
    // Update UI immediately so buttons return to original state
    emit m_project.segmentsUpdated();

    auto *segment = segmentByDisplayIndex(segmentIndex);
    if (!segment) {
        LOG_WARN() << "Segment not found for index" << segmentIndex;
        return;
    }

    // Check if reverse file exists, create if needed
    if (segment->reversePath.isEmpty()) {
        const QString cutsDir = PathUtils::defaultCutsRoot();
        PathUtils::ensureDirectory(cutsDir);
        segment->reversePath = PathUtils::composeSegmentReverseFile(cutsDir, segmentIndex);
    }

    // Delete existing reverse file if it's corrupted or needs recreation
    if (QFileInfo::exists(segment->reversePath)) {
        QFileInfo fileInfo(segment->reversePath);
        if (fileInfo.size() < 44) { // WAV header is 44 bytes minimum
            LOG_INFO() << "Removing corrupted reverse file for segment" << segmentIndex << "size" << fileInfo.size();
            QFile::remove(segment->reversePath);
        } else {
            // Try to verify file is valid by attempting to read it
            QByteArray testPcm;
            QAudioFormat testFormat;
            QString testError;
            if (!WavUtils::readWavFile(segment->reversePath, testPcm, testFormat, &testError)) {
                LOG_INFO() << "Removing invalid reverse file for segment" << segmentIndex << "error:" << testError;
                QFile::remove(segment->reversePath);
            } else {
                LOG_INFO() << "Reverse file exists and is valid for segment" << segmentIndex;
            }
        }
    }

    if (!QFileInfo::exists(segment->reversePath)) {
        LOG_INFO() << "Creating reverse file for segment" << segmentIndex;
        QByteArray pcm;
        QAudioFormat format;
        QString error;

        // Always create reverse from original buffer (button is "Реверс оригинал")
        LOG_INFO() << "Creating reverse from original segment" << segmentIndex;
        const QByteArray segmentData = m_project.originalBuffer().sliceFrames(segment->startFrame, segment->frameCount);
        if (segmentData.isEmpty()) {
            setStatusMessage(tr("Ошибка: сегмент %1 пуст").arg(segmentIndex));
            LOG_WARN() << "Empty segment data for reverse, index" << segmentIndex;
            return;
        }

        format = m_project.originalBuffer().format();
        pcm = segmentData;
        
        LOG_INFO() << "Extracted segment" << segmentIndex << "from original" 
                   << "size" << pcm.size() << "format:" << format.channelCount() << "ch" << format.sampleRate() << "Hz"
                   << format.sampleSize() << "bit" << (format.sampleType() == QAudioFormat::SignedInt ? "signed" : "float");
        
        if (!format.isValid()) {
            setStatusMessage(tr("Ошибка: неверный формат для сегмента %1").arg(segmentIndex));
            LOG_WARN() << "Invalid format for segment" << segmentIndex;
            return;
        }

        // Ensure format is 16-bit PCM for WAV writing
        // Note: format may already be 16-bit PCM from MP3 decoder, but we ensure it's correct
        if (format.sampleSize() != 16 || format.sampleType() != QAudioFormat::SignedInt) {
            LOG_INFO() << "Format needs conversion for segment" << segmentIndex 
                       << "current:" << format.sampleSize() << "bit" 
                       << (format.sampleType() == QAudioFormat::SignedInt ? "signed" : "float");
            // Data is already in correct format from decoder, just update format metadata
            format.setSampleSize(16);
            format.setSampleType(QAudioFormat::SignedInt);
            format.setCodec(QStringLiteral("audio/pcm"));
            format.setByteOrder(QAudioFormat::LittleEndian);
        }
        
        // Validate format and data
        if (!format.isValid() || pcm.isEmpty()) {
            setStatusMessage(tr("Ошибка: неверный формат или пустые данные для сегмента %1").arg(segmentIndex));
            LOG_WARN() << "Invalid format or empty data for reverse, segment" << segmentIndex 
                       << "format valid:" << format.isValid() << "pcm size:" << pcm.size();
            return;
        }

        // Create reversed audio
        QByteArray reversed = AudioBuffer::reverseSamples(pcm, format);
        if (reversed.isEmpty()) {
            setStatusMessage(tr("Ошибка: не удалось создать реверс для сегмента %1").arg(segmentIndex));
            LOG_WARN() << "Failed to reverse samples for segment" << segmentIndex;
            return;
        }

        LOG_INFO() << "Writing reverse file for segment" << segmentIndex << "size" << reversed.size() 
                   << "format" << format.channelCount() << "ch" << format.sampleRate() << "Hz"
                   << format.sampleSize() << "bit" << "to" << segment->reversePath;
        if (!WavUtils::writeWavFile(segment->reversePath, format, reversed, &error)) {
            setStatusMessage(tr("Ошибка создания реверса сегмента %1: %2").arg(segmentIndex).arg(error));
            LOG_WARN() << "Failed to write reverse file:" << segment->reversePath << error;
            return;
        }
        
        // Verify file was created and has data
        QFileInfo fileInfo(segment->reversePath);
        if (!fileInfo.exists() || fileInfo.size() < 44) { // WAV header is 44 bytes minimum
            setStatusMessage(tr("Ошибка: файл реверса создан некорректно для сегмента %1").arg(segmentIndex));
            LOG_WARN() << "Reverse file verification failed for segment" << segmentIndex 
                       << "exists:" << fileInfo.exists() << "size:" << fileInfo.size();
            return;
        }
        
        segment->hasReverse = true;
        emit m_project.segmentsUpdated();
        LOG_INFO() << "Created reverse file for segment" << segmentIndex << "at" << segment->reversePath << "size" << fileInfo.size();
    }

    if (m_playback->playFile(segment->reversePath)) {
        m_activeReversePlayback.insert(segmentIndex);
        setStatusMessage(tr("Воспроизведение реверса сегмента %1").arg(segmentIndex));
        LOG_INFO() << "Started reverse playback for segment" << segmentIndex;
        emit m_project.segmentsUpdated();
    } else {
        setStatusMessage(tr("Ошибка воспроизведения реверса сегмента %1").arg(segmentIndex));
        LOG_WARN() << "Failed to play reverse segment" << segmentIndex;
    }
}

void AppController::glueSegments()
{
    if (!hasAllSegmentsRecorded()) {
        setStatusMessage(tr("Не все сегменты записаны"));
        LOG_WARN() << "Cannot glue: not all segments recorded";
        return;
    }

    setStatusMessage(tr("Склейка сегментов..."));
    LOG_INFO() << "Glue segments requested";

    const auto &segments = m_project.segments();
    if (segments.isEmpty()) {
        setStatusMessage(tr("Нет сегментов для склейки"));
        LOG_WARN() << "No segments to glue";
        return;
    }

    // Read all recorded segments and trim noise
    // Segments are stored in reverse order (from end to start of song):
    // segment 1 = end of song, segment 2, segment 3, segment 4 = start of song
    // We glue them in display order (1 → 2 → 3 → 4) so that after reversing
    // the glued song, we get correct order (4 → 3 → 2 → 1 = start to end)
    QByteArray gluedPcm;
    QAudioFormat format;
    bool formatSet = false;

    // Iterate in display order (as shown in the list)
    for (const auto &segment : segments) {
        if (segment.recordingPath.isEmpty() || !QFileInfo::exists(segment.recordingPath)) {
            setStatusMessage(tr("Файл записи сегмента %1 не найден").arg(segment.displayIndex));
            LOG_WARN() << "Recording file not found for segment" << segment.displayIndex;
            return;
        }

        QByteArray pcm;
        QAudioFormat segFormat;
        QString error;
        if (!WavUtils::readWavFile(segment.recordingPath, pcm, segFormat, &error)) {
            setStatusMessage(tr("Ошибка чтения сегмента %1: %2").arg(segment.displayIndex).arg(error));
            LOG_WARN() << "Failed to read segment" << segment.displayIndex << error;
            return;
        }

        if (!formatSet) {
            format = segFormat;
            formatSet = true;
        } else {
            // Ensure format matches
            if (format.sampleRate() != segFormat.sampleRate() ||
                format.channelCount() != segFormat.channelCount() ||
                format.sampleSize() != segFormat.sampleSize()) {
                setStatusMessage(tr("Несовместимые форматы сегментов"));
                LOG_WARN() << "Incompatible segment formats";
                return;
            }
        }

        // Trim noise from start and end (use manual boundaries if set)
        double trimStartMs = segment.trimStartMs;
        double trimEndMs = segment.trimEndMs;
        QByteArray trimmedPcm = trimSegmentNoise(pcm, segFormat, m_segmentNoiseThreshold, trimStartMs, trimEndMs);
        if (trimmedPcm.isEmpty()) {
            setStatusMessage(tr("Ошибка: сегмент %1 не содержит звука после обрезки").arg(segment.displayIndex));
            LOG_WARN() << "Segment" << segment.displayIndex << "is empty after trimming";
            return;
        }

        gluedPcm.append(trimmedPcm);
        LOG_INFO() << "Added trimmed segment" << segment.displayIndex 
                   << "to glued song, original size:" << pcm.size() 
                   << "trimmed size:" << trimmedPcm.size();
    }

    // Save glued song (normal order)
    const QString resultsDir = PathUtils::defaultResultsRoot();
    PathUtils::ensureDirectory(resultsDir);
    const QString songPath = PathUtils::composeSongFile(resultsDir, m_project.projectName());

    QString error;
    if (!WavUtils::writeWavFile(songPath, format, gluedPcm, &error)) {
        setStatusMessage(tr("Ошибка сохранения склеенной песни: %1").arg(error));
        LOG_WARN() << "Failed to write glued song:" << error;
        return;
    }

    m_project.setDecodedFilePath(songPath);
    LOG_INFO() << "Normal glued song saved to" << songPath;
    
    // Create reversed song: reverse each segment individually, then glue in reverse order
    // Segments in array: [end of song (displayIndex 1), ..., start of song (displayIndex N)]
    // For reverse: iterate backwards through array, reverse each segment, then glue
    QByteArray reversedPcm;
    bool reversedFormatSet = false;
    
    // Iterate in reverse order (from start of song to end of song)
    for (int i = segments.size() - 1; i >= 0; --i) {
        const auto &segment = segments[i];
        if (segment.recordingPath.isEmpty() || !QFileInfo::exists(segment.recordingPath)) {
            setStatusMessage(tr("Файл записи сегмента %1 не найден").arg(segment.displayIndex));
            LOG_WARN() << "Recording file not found for segment" << segment.displayIndex;
            return;
        }

        QByteArray pcm;
        QAudioFormat segFormat;
        if (!WavUtils::readWavFile(segment.recordingPath, pcm, segFormat, &error)) {
            setStatusMessage(tr("Ошибка чтения сегмента %1: %2").arg(segment.displayIndex).arg(error));
            LOG_WARN() << "Failed to read segment" << segment.displayIndex << error;
            return;
        }

        if (!reversedFormatSet) {
            format = segFormat;
            reversedFormatSet = true;
        } else {
            // Ensure format matches
            if (format.sampleRate() != segFormat.sampleRate() ||
                format.channelCount() != segFormat.channelCount() ||
                format.sampleSize() != segFormat.sampleSize()) {
                setStatusMessage(tr("Несовместимые форматы сегментов"));
                LOG_WARN() << "Incompatible segment formats";
                return;
            }
        }

        // Trim noise from start and end (use manual boundaries if set)
        double trimStartMs = segment.trimStartMs;
        double trimEndMs = segment.trimEndMs;
        QByteArray trimmedPcm = trimSegmentNoise(pcm, segFormat, m_segmentNoiseThreshold, trimStartMs, trimEndMs);
        if (trimmedPcm.isEmpty()) {
            setStatusMessage(tr("Ошибка: сегмент %1 не содержит звука после обрезки").arg(segment.displayIndex));
            LOG_WARN() << "Segment" << segment.displayIndex << "is empty after trimming";
            return;
        }

        // Reverse the trimmed segment
        QByteArray reversedSegment = AudioBuffer::reverseSamples(trimmedPcm, segFormat);
        if (reversedSegment.isEmpty()) {
            setStatusMessage(tr("Ошибка: не удалось перевернуть сегмент %1").arg(segment.displayIndex));
            LOG_WARN() << "Failed to reverse segment" << segment.displayIndex;
            return;
        }

        reversedPcm.append(reversedSegment);
        LOG_INFO() << "Added reversed segment" << segment.displayIndex 
                   << "to reversed song, original size:" << pcm.size() 
                   << "trimmed size:" << trimmedPcm.size()
                   << "reversed size:" << reversedSegment.size();
    }

    // Save reversed song
    const QString reversePath = PathUtils::composeReverseSongFile(resultsDir, m_project.projectName());
    if (!WavUtils::writeWavFile(reversePath, format, reversedPcm, &error)) {
        setStatusMessage(tr("Ошибка сохранения реверса: %1").arg(error));
        LOG_WARN() << "Failed to write reversed song:" << error;
        return;
    }

    m_reversedSongPath = reversePath;
    setStatusMessage(tr("Сегменты склеены: %1, реверс: %2").arg(songPath).arg(reversePath));
    LOG_INFO() << "Segments glued successfully. Normal:" << songPath << "Reversed:" << reversePath;
    emit saveStateChanged(); // Update save button state
    emit reverseStateChanged(); // Update reverse playback button state
}

void AppController::toggleGluePlayback()
{
    if (m_gluePlaybackActive) {
        m_playback->stopAll();
        m_gluePlaybackActive = false;
        emit glueStateChanged();
        setStatusMessage(tr("Воспроизведение склеенной песни остановлено"));
        LOG_INFO() << "Glue playback stopped";
        return;
    }

    if (m_project.decodedFilePath().isEmpty() || !QFileInfo::exists(m_project.decodedFilePath())) {
        setStatusMessage(tr("Сначала склейте сегменты"));
        LOG_WARN() << "Glued song file not found";
        return;
    }

    // Stop all other playback/recording
    m_playback->stopAll();
    m_recorder->stop();
    m_activeOriginalPlayback.clear();
    m_activeRecordedPlayback.clear();
    m_activeReversePlayback.clear();
    m_reversePlaybackActive = false;

    if (m_playback->playFile(m_project.decodedFilePath())) {
        m_gluePlaybackActive = true;
        emit glueStateChanged();
        emit reverseStateChanged();
        setStatusMessage(tr("Воспроизведение склеенной песни"));
        LOG_INFO() << "Glue playback started";
    } else {
        setStatusMessage(tr("Ошибка воспроизведения склеенной песни"));
        LOG_WARN() << "Failed to play glued song";
    }
}

// reverseSong() method removed - reverse is now created automatically during glueSegments()

void AppController::toggleSongReversePlayback()
{
    if (m_reversePlaybackActive) {
        m_playback->stopAll();
        m_reversePlaybackActive = false;
        emit reverseStateChanged();
        setStatusMessage(tr("Воспроизведение реверса остановлено"));
        LOG_INFO() << "Reverse playback stopped";
        return;
    }

    if (m_reversedSongPath.isEmpty() || !QFileInfo::exists(m_reversedSongPath)) {
        setStatusMessage(tr("Сначала создайте реверс песни"));
        LOG_WARN() << "Reversed song file not found";
        return;
    }

    // Stop all other playback/recording
    m_playback->stopAll();
    m_recorder->stop();
    m_activeOriginalPlayback.clear();
    m_activeRecordedPlayback.clear();
    m_activeReversePlayback.clear();
    m_gluePlaybackActive = false;

    if (m_playback->playFile(m_reversedSongPath)) {
        m_reversePlaybackActive = true;
        emit reverseStateChanged();
        emit glueStateChanged();
        setStatusMessage(tr("Воспроизведение реверса"));
        LOG_INFO() << "Reverse playback started";
    } else {
        setStatusMessage(tr("Ошибка воспроизведения реверса"));
        LOG_WARN() << "Failed to play reversed song";
    }
}

void AppController::saveProject()
{
    if (!m_projectReady) {
        setStatusMessage(tr("Нет проекта для сохранения"));
        LOG_WARN() << "No project to save";
        return;
    }

    const QString projectsDir = PathUtils::defaultProjectsRoot();
    PathUtils::ensureDirectory(projectsDir);
    const QString projectDir = projectsDir + QDir::separator() + m_project.projectName();
    PathUtils::ensureDirectory(projectDir);

    saveProjectTo(projectDir);
}

void AppController::stopCurrentRecording()
{
    // Stop source recording if active
    if (m_sourceRecordingActive) {
        stopSourceRecording();
        return;
    }
    
    // Stop segment recording if active
    if (!m_activeSegmentRecordings.isEmpty()) {
        // Get the first (and should be only) active segment recording
        int segmentIndex = *m_activeSegmentRecordings.begin();
        toggleSegmentRecording(segmentIndex);
        return;
    }
}

void AppController::setStatusMessage(const QString &message)
{
    m_statusMessage = message;
    emit statusMessageChanged();
}

void AppController::refreshUiStates()
{
    emit interactionsStateChanged();
    emit glueStateChanged();
    emit reverseStateChanged();
}

void AppController::clearPlaybackStates()
{
    m_activeOriginalPlayback.clear();
    m_activeRecordedPlayback.clear();
    m_activeReversePlayback.clear();
    m_gluePlaybackActive = false;
    m_reversePlaybackActive = false;
    m_activePlaybackSegmentIndex = -1;
    m_isPlayingOriginalSegment = false;
    emit playbackPositionChanged();
    refreshUiStates();
    // Emit segments update to refresh button states in QML
    emit m_project.segmentsUpdated();
}

void AppController::ensureProjectNameFromSource(const QString &sourcePath)
{
    const QFileInfo info(sourcePath);
    m_project.setProjectName(PathUtils::cleanName(info.completeBaseName()));
}

bool AppController::hasAllSegmentsRecorded() const
{
    const auto &segments = m_project.segments();
    if (segments.isEmpty())
        return false;
    for (const auto &segment : segments) {
        if (!segment.hasRecording)
            return false;
    }
    return true;
}

bool AppController::hasAnySegmentRecorded() const
{
    const auto &segments = m_project.segments();
    for (const auto &segment : segments) {
        if (segment.hasRecording)
            return true;
    }
    return false;
}

SegmentInfo *AppController::segmentByDisplayIndex(int displayIndex)
{
    auto &segments = m_project.segments();
    for (auto &segment : segments) {
        if (segment.displayIndex == displayIndex)
            return &segment;
    }
    return nullptr;
}

const SegmentInfo *AppController::segmentByDisplayIndex(int displayIndex) const
{
    const auto &segments = m_project.segments();
    for (const auto &segment : segments) {
        if (segment.displayIndex == displayIndex)
            return &segment;
    }
    return nullptr;
}

QVariantList AppController::analyzeVolume(int windowSizeMs, double quietThreshold, double loudThreshold)
{
    QVariantList result;
    
    if (!m_projectReady) {
        setStatusMessage(tr("Проект не загружен"));
        LOG_WARN() << "analyzeVolume called but project not ready";
        return result;
    }
    
    const AudioBuffer &buffer = m_project.originalBuffer();
    if (!buffer.format().isValid() || buffer.frameCount() == 0) {
        setStatusMessage(tr("Аудио данные недоступны"));
        LOG_WARN() << "analyzeVolume called but buffer is empty or invalid";
        return result;
    }
    
    setStatusMessage(tr("Анализ громкости..."));
    LOG_INFO() << "Starting volume analysis, windowSizeMs:" << windowSizeMs 
               << "quietThreshold:" << quietThreshold << "loudThreshold:" << loudThreshold;
    
    const QVector<VolumeLevel> levels = VolumeAnalyzer::analyzeVolume(
        buffer, windowSizeMs, quietThreshold, loudThreshold);
    
    const QAudioFormat &format = buffer.format();
    const qint64 sampleRate = format.sampleRate();
    
    // Convert to QVariantList for QML
    for (const VolumeLevel &level : levels) {
        QVariantMap levelMap;
        const qint64 startMs = (level.startFrame * 1000) / sampleRate;
        const qint64 endMs = ((level.startFrame + level.frameCount) * 1000) / sampleRate;
        
        levelMap["startMs"] = startMs;
        levelMap["endMs"] = endMs;
        levelMap["startFrame"] = level.startFrame;
        levelMap["frameCount"] = level.frameCount;
        levelMap["rmsLevel"] = level.rmsLevel;
        levelMap["peakLevel"] = level.peakLevel;
        levelMap["isQuiet"] = level.isQuiet;
        levelMap["isLoud"] = level.isLoud;
        
        result.append(levelMap);
    }
    
    // Count quiet and loud sections
    int quietCount = 0;
    int loudCount = 0;
    for (const VolumeLevel &level : levels) {
        if (level.isQuiet) quietCount++;
        if (level.isLoud) loudCount++;
    }
    
    setStatusMessage(tr("Анализ завершен: тихих участков: %1, громких: %2").arg(quietCount).arg(loudCount));
    LOG_INFO() << "Volume analysis completed: quiet sections:" << quietCount << "loud sections:" << loudCount;
    
    return result;
}

QVariantList AppController::getSegmentDataForWaveform()
{
    QVariantList result;
    
    if (!m_projectReady) {
        return result;
    }
    
    const auto &segments = m_project.segments();
    const QAudioFormat &format = m_project.originalBuffer().format();
    const qint64 sampleRate = format.sampleRate();
    
    if (sampleRate <= 0) {
        return result;
    }
    
    for (const auto &segment : segments) {
        QVariantMap segmentMap;
        const qint64 startMs = (segment.startFrame * 1000) / sampleRate;
        const qint64 endMs = ((segment.startFrame + segment.frameCount) * 1000) / sampleRate;
        
        segmentMap["startMs"] = startMs;
        segmentMap["endMs"] = endMs;
        segmentMap["index"] = segment.displayIndex;
        
        result.append(segmentMap);
    }
    
    return result;
}

QVariantList AppController::analyzeSegmentRecording(int segmentIndex, int windowSizeMs)
{
    QVariantList result;
    
    if (!m_projectReady) {
        return result;
    }
    
    const SegmentInfo *segment = segmentByDisplayIndex(segmentIndex);
    if (!segment || !segment->hasRecording || segment->recordingPath.isEmpty()) {
        return result;
    }
    
    if (!QFileInfo::exists(segment->recordingPath)) {
        LOG_WARN() << "Segment recording file not found:" << segment->recordingPath;
        return result;
    }
    
    // Load the recording file
    QByteArray pcmData;
    QAudioFormat format;
    QString error;
    if (!WavUtils::readWavFile(segment->recordingPath, pcmData, format, &error)) {
        LOG_WARN() << "Failed to read segment recording file:" << segment->recordingPath << "error:" << error;
        return result;
    }
    
    // Create AudioBuffer from loaded data
    AudioBuffer buffer;
    buffer.setFormat(format);
    buffer.data() = pcmData;
    
    if (buffer.frameCount() == 0) {
        LOG_WARN() << "Segment recording buffer is empty";
        return result;
    }
    
    // Analyze volume using segment noise threshold
    const QVector<VolumeLevel> levels = VolumeAnalyzer::analyzeVolume(
        buffer, windowSizeMs, m_segmentNoiseThreshold, 0.7);
    
    const qint64 sampleRate = format.sampleRate();
    
    // Convert to QVariantList for QML
    for (const VolumeLevel &level : levels) {
        QVariantMap levelMap;
        const qint64 startMs = (level.startFrame * 1000) / sampleRate;
        const qint64 endMs = ((level.startFrame + level.frameCount) * 1000) / sampleRate;
        
        levelMap["startMs"] = startMs;
        levelMap["endMs"] = endMs;
        levelMap["startFrame"] = level.startFrame;
        levelMap["frameCount"] = level.frameCount;
        levelMap["rmsLevel"] = level.rmsLevel;
        levelMap["peakLevel"] = level.peakLevel;
        levelMap["isQuiet"] = level.isQuiet;
        levelMap["isLoud"] = level.isLoud;
        
        result.append(levelMap);
    }
    
    return result;
}

double AppController::getSegmentStartMs(int segmentIndex)
{
    if (!m_projectReady) {
        return 0.0;
    }
    
    const SegmentInfo *segment = segmentByDisplayIndex(segmentIndex);
    if (!segment) {
        return 0.0;
    }
    
    const QAudioFormat &format = m_project.originalBuffer().format();
    const qint64 sampleRate = format.sampleRate();
    
    if (sampleRate <= 0) {
        return 0.0;
    }
    
    return (segment->startFrame * 1000.0) / sampleRate;
}

double AppController::getSegmentRecordingTrimmedStartMs(int segmentIndex)
{
    if (!m_projectReady) {
        return 0.0;
    }
    
    const SegmentInfo *segment = segmentByDisplayIndex(segmentIndex);
    if (!segment || !segment->hasRecording || segment->recordingPath.isEmpty()) {
        return 0.0;
    }
    
    // If manual trim boundaries are set, use them
    if (segment->trimStartMs >= 0) {
        return segment->trimStartMs;
    }
    
    if (!QFileInfo::exists(segment->recordingPath)) {
        return 0.0;
    }
    
    // Load the recording file
    QByteArray pcm;
    QAudioFormat format;
    QString error;
    if (!WavUtils::readWavFile(segment->recordingPath, pcm, format, &error)) {
        LOG_WARN() << "Failed to read segment recording for trimmed start calculation:" << segment->recordingPath << error;
        return 0.0;
    }
    
    // Create AudioBuffer from loaded data
    AudioBuffer buffer;
    buffer.setFormat(format);
    buffer.data() = pcm;
    
    if (buffer.frameCount() == 0) {
        return 0.0;
    }
    
    const qint64 sampleRate = format.sampleRate();
    if (sampleRate <= 0) {
        return 0.0;
    }
    
    // Analyze volume to find where trimming starts
    const QVector<VolumeLevel> levels = VolumeAnalyzer::analyzeVolume(
        buffer, 100, m_segmentNoiseThreshold, 0.7);
    
    if (levels.isEmpty()) {
        return 0.0;
    }
    
    // Find start of real sound (first loud section)
    qint64 startFrame = 0;
    for (int i = 0; i < levels.size(); ++i) {
        const VolumeLevel &level = levels[i];
        if (!level.isQuiet) {
            // Found first loud section - this is where trimming starts
            startFrame = level.startFrame;
            break;
        }
    }
    
    // Convert frames to milliseconds
    return (startFrame * 1000.0) / sampleRate;
}

// Helper function to calculate trim boundaries for a segment recording
static QPair<double, double> calculateTrimBoundaries(const QByteArray &pcm, const QAudioFormat &format, double noiseThreshold)
{
    QPair<double, double> result(-1.0, -1.0);
    
    if (pcm.isEmpty() || !format.isValid()) {
        return result;
    }
    
    AudioBuffer buffer;
    buffer.setFormat(format);
    buffer.data() = pcm;
    
    if (buffer.frameCount() == 0) {
        return result;
    }
    
    const qint64 sampleRate = format.sampleRate();
    if (sampleRate <= 0) {
        return result;
    }
    
    const QVector<VolumeLevel> levels = VolumeAnalyzer::analyzeVolume(
        buffer, 100, noiseThreshold, 0.7);
    
    if (levels.isEmpty()) {
        return result;
    }
    
    // Find start of real sound (first loud section)
    qint64 startFrame = 0;
    for (int i = 0; i < levels.size(); ++i) {
        const VolumeLevel &level = levels[i];
        if (!level.isQuiet) {
            startFrame = level.startFrame;
            break;
        }
    }
    
    // Find end of real sound (last loud section)
    qint64 endFrame = buffer.frameCount();
    for (int i = levels.size() - 1; i >= 0; --i) {
        const VolumeLevel &level = levels[i];
        if (!level.isQuiet) {
            endFrame = level.startFrame + level.frameCount;
            break;
        }
    }
    
    // Convert to milliseconds
    double startMs = (startFrame * 1000.0) / sampleRate;
    double endMs = (endFrame * 1000.0) / sampleRate;
    
    result.first = startMs;
    result.second = endMs;
    
    return result;
}

QVariantMap AppController::getSegmentTrimBoundaries(int segmentIndex)
{
    QVariantMap result;
    result["trimStartMs"] = -1.0;
    result["trimEndMs"] = -1.0;
    
    if (!m_projectReady) {
        return result;
    }
    
    SegmentInfo *segment = segmentByDisplayIndex(segmentIndex);
    if (!segment || !segment->hasRecording || segment->recordingPath.isEmpty()) {
        return result;
    }
    
    if (!QFileInfo::exists(segment->recordingPath)) {
        return result;
    }
    
    // If manual boundaries are set, return them
    if (segment->trimStartMs >= 0 && segment->trimEndMs >= 0) {
        result["trimStartMs"] = segment->trimStartMs;
        result["trimEndMs"] = segment->trimEndMs;
        return result;
    }
    
    // Otherwise, calculate automatic boundaries
    QByteArray pcm;
    QAudioFormat format;
    QString error;
    if (!WavUtils::readWavFile(segment->recordingPath, pcm, format, &error)) {
        LOG_WARN() << "Failed to read segment recording for trim boundaries:" << segment->recordingPath << error;
        return result;
    }
    
    QPair<double, double> boundaries = calculateTrimBoundaries(pcm, format, m_segmentNoiseThreshold);
    result["trimStartMs"] = boundaries.first;
    result["trimEndMs"] = boundaries.second;
    
    return result;
}

void AppController::setSegmentTrimBoundaries(int segmentIndex, double trimStartMs, double trimEndMs)
{
    if (!m_projectReady) {
        return;
    }
    
    SegmentInfo *segment = segmentByDisplayIndex(segmentIndex);
    if (!segment || !segment->hasRecording) {
        return;
    }
    
    // Validate boundaries
    if (trimStartMs < 0 || trimEndMs < 0 || trimStartMs >= trimEndMs) {
        LOG_WARN() << "Invalid trim boundaries for segment" << segmentIndex 
                   << "start:" << trimStartMs << "end:" << trimEndMs;
        return;
    }
    
    segment->trimStartMs = trimStartMs;
    segment->trimEndMs = trimEndMs;
    
    LOG_INFO() << "Set trim boundaries for segment" << segmentIndex 
               << "start:" << trimStartMs << "ms end:" << trimEndMs << "ms";
    
    emit m_project.segmentsUpdated();
}

void AppController::recreateSegmentsFromBoundaries(const QVariantList &boundariesMs, bool manualBoundaries)
{
    if (!m_projectReady) {
        setStatusMessage(tr("Проект не загружен"));
        LOG_WARN() << "Cannot recreate segments: project not ready";
        return;
    }
    
    const AudioBuffer &buffer = m_project.originalBuffer();
    if (!buffer.format().isValid() || buffer.frameCount() == 0) {
        setStatusMessage(tr("Аудио данные недоступны"));
        LOG_WARN() << "Cannot recreate segments: buffer is empty or invalid";
        return;
    }
    
    const qint64 sampleRate = buffer.format().sampleRate();
    if (sampleRate <= 0) {
        setStatusMessage(tr("Неверный формат аудио"));
        LOG_WARN() << "Cannot recreate segments: invalid sample rate";
        return;
    }
    
    const qint64 totalFrames = buffer.frameCount();
    const double totalDurationMs = (totalFrames * 1000.0) / sampleRate;
    
    // Convert boundaries from milliseconds to frames and sort
    QVector<qint64> boundariesFrames;
    boundariesFrames.reserve(boundariesMs.size());
    
    for (const QVariant &boundary : boundariesMs) {
        double ms = boundary.toDouble();
        // Clamp to valid range
        ms = qBound(0.0, ms, totalDurationMs);
        qint64 frame = static_cast<qint64>((ms * sampleRate) / 1000.0);
        frame = qBound(0LL, frame, totalFrames);
        boundariesFrames.append(frame);
    }
    
    // Sort boundaries
    std::sort(boundariesFrames.begin(), boundariesFrames.end());
    
    // Remove duplicates and ensure we have boundaries at start and end
    QVector<qint64> uniqueBoundaries;
    uniqueBoundaries.append(0); // Always start at 0
    for (qint64 frame : boundariesFrames) {
        if (frame > 0 && frame < totalFrames && 
            (uniqueBoundaries.isEmpty() || frame != uniqueBoundaries.last())) {
            uniqueBoundaries.append(frame);
        }
    }
    if (uniqueBoundaries.last() != totalFrames) {
        uniqueBoundaries.append(totalFrames); // Always end at totalFrames
    }
    
    // Delete all existing reverse files before recreating segments
    // because segments will be recreated with new frame boundaries
    const QString cutsDir = PathUtils::defaultCutsRoot();
    if (QDir(cutsDir).exists()) {
        const QStringList filters = QStringList() << "segment_*_reverse.wav";
        const QFileInfoList files = QDir(cutsDir).entryInfoList(filters, QDir::Files);
        for (const QFileInfo &fileInfo : files) {
            if (QFile::remove(fileInfo.absoluteFilePath())) {
                LOG_INFO() << "Removed old reverse file due to boundary changes:" << fileInfo.absoluteFilePath();
            }
        }
    }
    
    // Create segments from boundaries
    // IMPORTANT: Create segments from end to start (same order as splitIntoSegments)
    // This ensures consistent display order in the list
    auto &segments = m_project.segments();
    segments.clear();
    
    const qint64 minFrames = sampleRate; // 1 second minimum
    const qint64 minFramesFirstLast = static_cast<qint64>(sampleRate * 0.75); // 0.75 seconds for first/last segments
    
    // Build segments from end to start to match splitIntoSegments order
    // This ensures the array order is [end of song, ..., start of song]
    // which matches the display order in the list
    int displayIndex = 1;
    
    const int totalBoundaries = uniqueBoundaries.size();
    
    for (int i = uniqueBoundaries.size() - 2; i >= 0; --i) {
        qint64 startFrame = uniqueBoundaries[i];
        qint64 endFrame = uniqueBoundaries[i + 1];
        qint64 frameCount = endFrame - startFrame;
        
        // Check if this is the first segment (i == totalBoundaries - 2) or last segment (i == 0)
        bool isFirstSegment = (i == totalBoundaries - 2);
        bool isLastSegment = (i == 0);
        
        // Special rule: skip first or last segment if shorter than 0.75 seconds
        if ((isFirstSegment || isLastSegment) && frameCount < minFramesFirstLast) {
            LOG_INFO() << "Skipping" << (isFirstSegment ? "first" : "last") << "segment: too short (" 
                       << (frameCount * 1000.0 / sampleRate) << "ms < 750ms)";
            continue;
        }
        
        // Skip segments shorter than 1 second (except if it's the only segment)
        // BUT: if boundaries were set manually, don't skip short segments (except first/last)
        if (!manualBoundaries && frameCount < minFrames && uniqueBoundaries.size() > 2) {
            continue;
        }
        
        SegmentInfo info;
        info.displayIndex = displayIndex++;
        info.startFrame = startFrame;
        info.frameCount = frameCount;
        info.durationMs = static_cast<qint64>((static_cast<double>(frameCount) / sampleRate) * 1000.0);
        segments.append(info);
    }
    
    // Reset segment statuses (recordings are lost when boundaries change)
    m_project.resetSegmentStatuses();
    
    LOG_INFO() << "Recreated" << segments.size() << "segments from" << uniqueBoundaries.size() << "boundaries";
    setStatusMessage(tr("Отрезки пересозданы: %1").arg(segments.size()));
    
    emit m_project.segmentsUpdated();
    emit segmentLengthChanged();
}


