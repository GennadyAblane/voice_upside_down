#include "recordingengine.h"

#include <QAudioSource>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QBuffer>
#include <memory>

#include "wavutils.h"
#include "qt6_compat.h"
#include "../utils/logger.h"

class RecordingEngine::Impl
{
public:
    std::unique_ptr<QAudioSource> audioInput;
    std::unique_ptr<QBuffer> bufferDevice;
    QByteArray bufferData;
    QAudioFormat format;
    QString filePath;
    bool recording = false;
};

RecordingEngine::RecordingEngine(QObject *parent)
    : QObject(parent)
    , d(new Impl())
{
}

RecordingEngine::~RecordingEngine()
{
    stop();
    delete d;
}

bool RecordingEngine::startRecording(const QString &filePath, const QAudioFormat &requestedFormat)
{
    stop();

    QAudioDevice device = QMediaDevices::defaultAudioInput();
    d->format = requestedFormat;
    
    // Ensure format is 16-bit signed integer
    if (Qt6Compat::sampleSize(d->format) != 16 || !Qt6Compat::isSignedInt(d->format)) {
        Qt6Compat::setSampleSize(d->format, 16);
        Qt6Compat::setSignedInt(d->format);
    }

    d->bufferData.clear();
    d->bufferDevice = std::make_unique<QBuffer>(&d->bufferData);
    if (!d->bufferDevice->open(QIODevice::WriteOnly)) {
        d->bufferDevice.reset();
        return false;
    }
    d->filePath = filePath;

    d->audioInput = std::make_unique<QAudioSource>(device, d->format, this);
    connect(d->audioInput.get(), &QAudioSource::stateChanged, this, [this](QAudio::State state) {
        if (state == QAudio::StoppedState) {
            // In Qt 6, error handling is different
            LOG_INFO() << "Recording stopped";
        }
    });

    d->audioInput->start(d->bufferDevice.get());
    d->recording = true;
    LOG_INFO() << "Recording started to" << filePath;
    return true;
}

void RecordingEngine::stop()
{
    if (!d->recording)
        return;

    if (d->audioInput)
        d->audioInput->stop();
    if (d->bufferDevice)
        d->bufferDevice->close();

    if (!d->filePath.isEmpty() && !d->bufferData.isEmpty()) {
        if (!WavUtils::writeWavFile(d->filePath, d->format, d->bufferData)) {
            LOG_WARN() << "Failed to write recorded WAV to" << d->filePath;
        }
    }

    d->audioInput.reset();
    d->bufferDevice.reset();
    d->bufferData.clear();
    d->filePath.clear();
    d->recording = false;
    LOG_INFO() << "Recording stopped";
}

bool RecordingEngine::isRecording() const
{
    return d->recording;
}

