#include "recordingengine.h"

#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QBuffer>
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

    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultInputDevice();
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

    d->bufferData.clear();
    d->bufferDevice = std::make_unique<QBuffer>(&d->bufferData);
    if (!d->bufferDevice->open(QIODevice::WriteOnly)) {
        d->bufferDevice.reset();
        return false;
    }
    d->filePath = filePath;

    d->audioInput = std::make_unique<QAudioInput>(deviceInfo, d->format);
    d->audioInput->setNotifyInterval(100);
    connect(d->audioInput.get(), &QAudioInput::stateChanged, this, [this](QAudio::State state) {
        if (state == QAudio::StoppedState && d->audioInput->error() != QAudio::NoError) {
            LOG_WARN() << "Recording stopped due to error:" << d->audioInput->error();
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

