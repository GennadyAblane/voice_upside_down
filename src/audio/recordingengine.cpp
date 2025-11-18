#include "recordingengine.h"

#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QBuffer>
#include <QTimer>
#include <memory>

#include "wavutils.h"
#include "../utils/logger.h"

class RecordingEngine::Impl
{
public:
    std::unique_ptr<QAudioInput> audioInput;
    std::unique_ptr<QBuffer> bufferDevice;
    QByteArray bufferData;
    QAudioFormat format;
    QString filePath;
    bool recording = false;
    bool recordingReady = false;
    QAudioFormat preparedFormat;
    bool formatPrepared = false;
    QTimer* stopTimer = nullptr;
    QTimer* readyCheckTimer = nullptr;
    QTimer* dataCheckTimer = nullptr; // Timer to check if data is actually being written
    int bufferSizeWhenReady = 0; // Track buffer size when device becomes ready
};

RecordingEngine::RecordingEngine(QObject *parent)
    : QObject(parent)
    , d(new Impl())
{
    d->stopTimer = new QTimer(this);
    d->stopTimer->setSingleShot(true);
    d->stopTimer->setInterval(250); // 0.25 seconds delay to capture tail
    connect(d->stopTimer, &QTimer::timeout, this, &RecordingEngine::onStopTimerTimeout);
    
    d->readyCheckTimer = new QTimer(this);
    d->readyCheckTimer->setSingleShot(false); // Repeat until ready
    d->readyCheckTimer->setInterval(100); // Check every 100ms
    connect(d->readyCheckTimer, &QTimer::timeout, this, [this]() {
        if (d->recording && !d->recordingReady && d->audioInput) {
            QAudio::State state = d->audioInput->state();
            LOG_INFO() << "Ready check timer: current state is" << state;
            if (state == QAudio::ActiveState || state == QAudio::IdleState) {
                // Device is in ready state - now check if data is actually being written
                d->readyCheckTimer->stop();
                // Start checking for actual data recording
                if (d->dataCheckTimer) {
                    d->dataCheckTimer->start();
                    LOG_INFO() << "Device state is ready, starting data check timer";
                }
            }
            // If not ready yet, timer will continue checking
        } else if (d->recordingReady || !d->recording) {
            // Stop timer if recording is ready or stopped
            d->readyCheckTimer->stop();
        }
    });
    
    d->dataCheckTimer = new QTimer(this);
    d->dataCheckTimer->setSingleShot(false); // Repeat until data appears
    d->dataCheckTimer->setInterval(50); // Check every 50ms for faster response
    connect(d->dataCheckTimer, &QTimer::timeout, this, [this]() {
        if (d->recording && !d->recordingReady && d->audioInput) {
            int currentBufferSize = d->bufferData.size();
            // Check if data is actually being written (buffer size increased)
            if (currentBufferSize > d->bufferSizeWhenReady) {
                // Data is being written - clear buffer and mark as ready
                int bytesToClear = currentBufferSize - d->bufferSizeWhenReady;
                LOG_INFO() << "Data is being written (" << bytesToClear << "bytes), clearing buffer and marking ready";
                d->bufferData.clear();
                if (d->bufferDevice) {
                    d->bufferDevice->close();
                    d->bufferDevice->open(QIODevice::WriteOnly);
                }
                // Now device is really ready - stop timer and emit signal
                d->dataCheckTimer->stop();
                d->recordingReady = true;
                LOG_INFO() << "Recording is ready - data confirmed, buffer cleared, emitting signal";
                emit recordingReady();
            }
            // If no data yet, timer will continue checking
        } else if (d->recordingReady || !d->recording) {
            // Stop timer if recording is ready or stopped
            d->dataCheckTimer->stop();
        }
    });
}

RecordingEngine::~RecordingEngine()
{
    stop();
    delete d;
}

void RecordingEngine::prepare(const QAudioFormat &requestedFormat)
{
    if (d->formatPrepared && d->preparedFormat == requestedFormat) {
        return; // Already prepared with the same format
    }

    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultInputDevice();
    QAudioFormat format;
    if (!deviceInfo.isFormatSupported(requestedFormat)) {
        format = deviceInfo.nearestFormat(requestedFormat);
    } else {
        format = requestedFormat;
    }

    if (format.sampleSize() != 16 || format.sampleType() != QAudioFormat::SignedInt) {
        format.setSampleSize(16);
        format.setSampleType(QAudioFormat::SignedInt);
    }
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec(QStringLiteral("audio/pcm"));

    // Pre-check device availability and format - this reduces delay later
    // We don't create QAudioInput here because it can't be reused after stop()
    // But we cache the format and device info for faster initialization
    d->preparedFormat = format;
    d->formatPrepared = true;
    LOG_INFO() << "Audio input device prepared, format:" << format.sampleRate() << "Hz," << format.channelCount() << "ch";
}

bool RecordingEngine::startRecording(const QString &filePath, const QAudioFormat &requestedFormat)
{
    // Stop previous recording if any, but don't reset recordingReady flag yet
    // We'll reset it after we start new recording
    if (d->recording) {
        // Just stop the timer and audio input, but keep recordingReady state
        if (d->readyCheckTimer) {
            d->readyCheckTimer->stop();
        }
        if (d->audioInput) {
            d->audioInput->stop();
        }
        if (d->bufferDevice) {
            d->bufferDevice->close();
        }
        // Save previous recording if needed
        if (!d->filePath.isEmpty() && !d->bufferData.isEmpty()) {
            if (!WavUtils::writeWavFile(d->filePath, d->format, d->bufferData)) {
                LOG_WARN() << "Failed to write previous recorded WAV to" << d->filePath;
            }
        }
        d->audioInput.reset();
        d->bufferDevice.reset();
        d->bufferData.clear();
        d->filePath.clear();
        d->recording = false;
    }

    // Prepare buffer first - this is fast
    d->bufferData.clear();
    d->bufferDevice = std::make_unique<QBuffer>(&d->bufferData);
    if (!d->bufferDevice->open(QIODevice::WriteOnly)) {
        d->bufferDevice.reset();
        return false;
    }
    d->filePath = filePath;
    d->recordingReady = false;
    d->bufferSizeWhenReady = 0;

    // Use prepared format if available and matches, otherwise determine format
    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultInputDevice();
    if (d->formatPrepared && d->preparedFormat == requestedFormat) {
        // Use pre-prepared format - this should be fast
        d->format = d->preparedFormat;
        LOG_INFO() << "Using prepared audio format";
    } else {
        // Determine format (slower path)
        if (!deviceInfo.isFormatSupported(requestedFormat)) {
            d->format = deviceInfo.nearestFormat(requestedFormat);
            LOG_WARN() << "Requested format is not supported. Using nearest input format.";
        } else {
            d->format = requestedFormat;
        }

        if (d->format.sampleSize() != 16 || d->format.sampleType() != QAudioFormat::SignedInt) {
            d->format.setSampleSize(16);
            d->format.setSampleType(QAudioFormat::SignedInt);
        }
        d->format.setByteOrder(QAudioFormat::LittleEndian);
        d->format.setCodec(QStringLiteral("audio/pcm"));
    }

    // Create QAudioInput - this is where delay usually happens
    // But if format was prepared, device info is already known, so it should be faster
    d->audioInput = std::make_unique<QAudioInput>(deviceInfo, d->format);
    d->audioInput->setNotifyInterval(100);
    connect(d->audioInput.get(), &QAudioInput::stateChanged, this, [this](QAudio::State state) {
        LOG_INFO() << "Audio input state changed to:" << state;
        if ((state == QAudio::ActiveState || state == QAudio::IdleState) && !d->recordingReady) {
            // Microphone is now active/idle - but wait for actual data recording
            // Remember current buffer size (should be 0 or small)
            d->bufferSizeWhenReady = d->bufferData.size();
            LOG_INFO() << "Device state is ready, buffer size:" << d->bufferSizeWhenReady << "starting data check";
            // Stop state check timer, start data check timer
            if (d->readyCheckTimer) {
                d->readyCheckTimer->stop();
            }
            if (d->dataCheckTimer) {
                d->dataCheckTimer->start();
            }
        }
        if (state == QAudio::StoppedState && d->audioInput->error() != QAudio::NoError) {
            LOG_WARN() << "Recording stopped due to error:" << d->audioInput->error();
        }
    });

    // Start recording - signal recordingReady() will be emitted when microphone is actually active
    d->audioInput->start(d->bufferDevice.get());
    d->recording = true;
    LOG_INFO() << "Recording initialization started to" << filePath;
    
    // Check state immediately - sometimes QAudioInput is already in ActiveState or IdleState
    QAudio::State currentState = d->audioInput->state();
    LOG_INFO() << "Initial audio input state:" << currentState;
    if ((currentState == QAudio::ActiveState || currentState == QAudio::IdleState) && !d->recordingReady) {
        // Device is in ready state - remember buffer size and start checking for actual data
        d->bufferSizeWhenReady = d->bufferData.size();
        LOG_INFO() << "Device state is ready immediately, buffer size:" << d->bufferSizeWhenReady << "starting data check";
        // Start data check timer to wait for actual data recording
        if (d->dataCheckTimer) {
            d->dataCheckTimer->start();
        }
    } else {
        // Start timer to check state repeatedly until ready
        if (d->readyCheckTimer && !d->recordingReady) {
            d->readyCheckTimer->start();
            LOG_INFO() << "Started ready check timer - will check every 100ms until ready";
        }
    }
    
    return true;
}

void RecordingEngine::stop()
{
    if (!d->recording)
        return;

    // Start timer to wait 0.25 seconds before finalizing recording
    // This captures the "tail" of the recording
    if (d->stopTimer) {
        d->stopTimer->start();
        LOG_INFO() << "Stop requested, waiting 0.25 seconds to capture tail...";
    } else {
        // If timer is not available, finalize immediately
        onStopTimerTimeout();
    }
}

void RecordingEngine::onStopTimerTimeout()
{
    if (!d->recording)
        return;

    // Stop all timers if running
    if (d->readyCheckTimer) {
        d->readyCheckTimer->stop();
    }
    if (d->dataCheckTimer) {
        d->dataCheckTimer->stop();
    }

    // Stop audio input
    if (d->audioInput)
        d->audioInput->stop();
    if (d->bufferDevice)
        d->bufferDevice->close();

    // Save recording to file
    if (!d->filePath.isEmpty() && !d->bufferData.isEmpty()) {
        if (!WavUtils::writeWavFile(d->filePath, d->format, d->bufferData)) {
            LOG_WARN() << "Failed to write recorded WAV to" << d->filePath;
        } else {
            LOG_INFO() << "Recording saved to" << d->filePath << "size:" << d->bufferData.size() << "bytes";
        }
    }

    // Always reset audioInput - QAudioInput cannot be reused after stop()
    // But we keep the prepared format info for faster re-initialization
    d->audioInput.reset();
    d->bufferDevice.reset();
    d->bufferData.clear();
    d->filePath.clear();
    d->recording = false;
    d->recordingReady = false;
    
    // Re-prepare format for next recording to reduce delay
    // This doesn't activate microphone, just prepares format info
    if (d->formatPrepared && d->format.isValid()) {
        // Re-prepare with the format we just used
        d->preparedFormat = d->format;
        LOG_INFO() << "Format re-prepared for next recording";
    }
    
    emit recordingStopped();
    LOG_INFO() << "Recording stopped and saved";
}

bool RecordingEngine::isRecording() const
{
    return d->recording;
}

