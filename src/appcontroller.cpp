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

    setStatusMessage(tr("Файл загружен: %1").arg(m_currentSourceName));
    LOG_INFO() << "Audio source loaded successfully:" << m_currentSourceName << "segments:" << m_project.segments().size();
}

void AppController::startSourceRecording()
{
    if (m_sourceRecordingActive) {
        LOG_WARN() << "startSourceRecording requested, but recording already active";
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

    if (!m_recorder->startRecording(filePath, format)) {
        setStatusMessage(tr("Ошибка начала записи"));
        LOG_WARN() << "Failed to start source recording";
        return;
    }

    m_sourceRecordingActive = true;
    emit sourceRecordingChanged();
    setStatusMessage(tr("Запись источника..."));
    LOG_INFO() << "Source recording started to" << filePath;
}

void AppController::stopSourceRecording()
{
    if (!m_sourceRecordingActive) {
        LOG_WARN() << "stopSourceRecording requested, but recording not active";
        return;
    }

    m_recorder->stop();
    m_sourceRecordingActive = false;
    emit sourceRecordingChanged();

    // Load the recorded file
    const QString filePath = PathUtils::defaultTempRoot() + QDir::separator() + QStringLiteral("source_recording.wav");
    if (QFileInfo::exists(filePath)) {
        loadAudioSource(filePath);
        setStatusMessage(tr("Запись завершена и загружена"));
        LOG_INFO() << "Source recording stopped and loaded";
    } else {
        setStatusMessage(tr("Запись остановлена"));
        LOG_WARN() << "Source recording stopped but file not found:" << filePath;
    }
}

void AppController::saveProjectTo(const QString &directoryPath)
{
    if (!m_projectReady)
        return;

    LOG_INFO() << "Saving project to" << directoryPath;
    QString info;
    if (m_serializer->save(directoryPath, m_project, &info)) {
        setStatusMessage(tr("Проект сохранен: %1").arg(directoryPath));
    LOG_INFO() << "Project saved successfully";
    } else {
        setStatusMessage(info);
        LOG_WARN() << "Failed to save project:" << info;
    }
}

void AppController::openProjectFrom(const QString &projectFilePath)
{
    LOG_INFO() << "Opening project from" << projectFilePath;
    QString info;
    if (!m_serializer->load(projectFilePath, m_project, &info)) {
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

    setStatusMessage(tr("Проект открыт: %1").arg(m_currentSourceName));
    LOG_INFO() << "Project opened successfully:" << m_currentSourceName;
}

void AppController::changeSegmentLength(int seconds)
{
    if (seconds == m_project.segmentLengthSeconds())
        return;

    LOG_INFO() << "Changing segment length to" << seconds;
    bool hadRecordings = hasAnySegmentRecorded();
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
        // Stop recording
        m_recorder->stop();
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
        return;
    }

    // Stop all other playback/recording
    m_playback->stopAll();
    m_recorder->stop();
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

    if (!m_recorder->startRecording(segment->recordingPath, format)) {
        setStatusMessage(tr("Ошибка начала записи сегмента %1").arg(segmentIndex));
        LOG_WARN() << "Failed to start recording for segment" << segmentIndex;
        return;
    }

    m_activeSegmentRecordings.insert(segmentIndex);
    setStatusMessage(tr("Запись сегмента %1").arg(segmentIndex));
    LOG_INFO() << "Segment recording started for index" << segmentIndex << "to" << segment->recordingPath;
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
        // Always create reverse from original segment (not from recording)
        QByteArray pcm;
        QAudioFormat format;
        QString error;

        // Extract from original buffer
        const QByteArray segmentData = m_project.originalBuffer().sliceFrames(segment->startFrame, segment->frameCount);
        if (segmentData.isEmpty()) {
            setStatusMessage(tr("Ошибка: сегмент %1 пуст").arg(segmentIndex));
            LOG_WARN() << "Empty segment data for reverse, index" << segmentIndex;
            return;
        }

        format = m_project.originalBuffer().format();
        pcm = segmentData;
        
        LOG_INFO() << "Extracted original segment" << segmentIndex << "size" << pcm.size() 
                   << "format:" << format.channelCount() << "ch" << format.sampleRate() << "Hz"
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
    QByteArray gluedPcm;
    QAudioFormat format;
    bool formatSet = false;

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

