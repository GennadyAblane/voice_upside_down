#include "appcontroller.h"

#include "audio/audiofiledecoder.h"
#include "audio/audioplaybackengine.h"
#include "audio/audioproject.h"
#include "audio/audiobuffer.h"
#include "audio/recordingengine.h"
#include "audio/segmentmodel.h"
#include "audio/wavutils.h"
#include "persistence/projectserializer.h"
#include "utils/logger.h"
#include "utils/pathutils.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QUrl>

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
    return hasAllSegmentsRecorded();
}

bool AppController::canPlayReverse() const
{
    return hasAllSegmentsRecorded();
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

void AppController::toggleSegmentOriginalPlayback(int segmentIndex)
{
    if (m_activeOriginalPlayback.contains(segmentIndex)) {
        m_playback->stopAll();
        m_activeOriginalPlayback.remove(segmentIndex);
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

    if (m_playback->playFile(segment->recordingPath)) {
        m_activeRecordedPlayback.insert(segmentIndex);
        setStatusMessage(tr("Воспроизведение записи сегмента %1").arg(segmentIndex));
        LOG_INFO() << "Started recorded playback for segment" << segmentIndex;
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

    // Read all recorded segments
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

        gluedPcm.append(pcm);
        LOG_INFO() << "Added segment" << segment.displayIndex << "to glued song, size" << pcm.size();
    }

    // Save glued song
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
    setStatusMessage(tr("Сегменты склеены: %1").arg(songPath));
    LOG_INFO() << "Segments glued successfully to" << songPath;
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

void AppController::reverseSong()
{
    if (m_project.decodedFilePath().isEmpty() || !QFileInfo::exists(m_project.decodedFilePath())) {
        setStatusMessage(tr("Сначала склейте сегменты"));
        LOG_WARN() << "Cannot reverse: glued song not found";
        return;
    }

    setStatusMessage(tr("Реверсирование песни..."));
    LOG_INFO() << "Reverse song requested";

    // Read glued song
    QByteArray pcm;
    QAudioFormat format;
    QString error;
    if (!WavUtils::readWavFile(m_project.decodedFilePath(), pcm, format, &error)) {
        setStatusMessage(tr("Ошибка чтения склеенной песни: %1").arg(error));
        LOG_WARN() << "Failed to read glued song:" << error;
        return;
    }

    // Reverse
    QByteArray reversed = AudioBuffer::reverseSamples(pcm, format);

    // Save reversed song
    const QString resultsDir = PathUtils::defaultResultsRoot();
    PathUtils::ensureDirectory(resultsDir);
    const QString reversePath = PathUtils::composeReverseSongFile(resultsDir, m_project.projectName());

    if (!WavUtils::writeWavFile(reversePath, format, reversed, &error)) {
        setStatusMessage(tr("Ошибка сохранения реверса: %1").arg(error));
        LOG_WARN() << "Failed to write reversed song:" << error;
        return;
    }

    m_reversedSongPath = reversePath;
    setStatusMessage(tr("Песня реверсирована: %1").arg(reversePath));
    LOG_INFO() << "Song reversed successfully to" << reversePath;
    emit saveStateChanged(); // Update save button state
}

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


