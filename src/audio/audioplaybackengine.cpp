#include "audioplaybackengine.h"

#include <QAudioOutput>
#include <QAudio>
#include <QBuffer>
#include <QFile>
#include <QScopedPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>

#include "wavutils.h"
#include "../utils/logger.h"

class AudioPlaybackEngine::Impl
{
public:
    std::unique_ptr<QAudioOutput> output;
    std::unique_ptr<QBuffer> bufferDevice;
    std::unique_ptr<QFile> fileDevice;
    QByteArray bufferData;
    bool playing = false;
    
    QAudioFormat format;
    QTimer *positionTimer = nullptr;
    QElapsedTimer elapsedTimer;
    qint64 totalBytes = 0;
    double durationMs = 0.0;
};

AudioPlaybackEngine::AudioPlaybackEngine(QObject *parent)
    : QObject(parent)
    , d(new Impl())
{
    d->positionTimer = new QTimer(this);
    d->positionTimer->setInterval(50); // Update every 50ms for smooth animation
    connect(d->positionTimer, &QTimer::timeout, this, &AudioPlaybackEngine::updatePlaybackPosition);
}

AudioPlaybackEngine::~AudioPlaybackEngine()
{
    stopAll();
    delete d;
}

bool AudioPlaybackEngine::playBuffer(const QByteArray &pcm, const QAudioFormat &format)
{
    if (!format.isValid() || pcm.isEmpty()) {
        LOG_WARN() << "playBuffer received invalid data";
        return false;
    }

    stopAll();

    d->bufferData = pcm;
    d->format = format;
    d->totalBytes = pcm.size();
    
    // Calculate duration in milliseconds
    int bytesPerFrame = format.channelCount() * (format.sampleSize() / 8);
    int sampleRate = format.sampleRate();
    if (bytesPerFrame > 0 && sampleRate > 0) {
        qint64 totalFrames = d->totalBytes / bytesPerFrame;
        d->durationMs = (totalFrames * 1000.0) / sampleRate;
    } else {
        d->durationMs = 0.0;
    }
    
    d->bufferDevice = std::make_unique<QBuffer>(&d->bufferData);
    d->bufferDevice->open(QIODevice::ReadOnly);

    d->output = std::make_unique<QAudioOutput>(format);
    connect(d->output.get(), &QAudioOutput::stateChanged, this, [this](QAudio::State state) {
        if (state == QAudio::IdleState) {
            stopAll();
            emit playbackFinished();
        } else if (state == QAudio::StoppedState && d->output && d->output->error() != QAudio::NoError) {
            const QString message = tr("Ошибка воспроизведения: %1").arg(d->output->error());
            emit playbackError(message);
        }
    });

    d->output->start(d->bufferDevice.get());
    d->playing = true;
    d->elapsedTimer.start();
    d->positionTimer->start();
    return true;
}

bool AudioPlaybackEngine::playFile(const QString &filePath)
{
    QByteArray pcm;
    QAudioFormat format;
    QString error;
    if (!WavUtils::readWavFile(filePath, pcm, format, &error)) {
        LOG_WARN() << "Unable to play file" << filePath << error;
        emit playbackError(error);
        return false;
    }
    return playBuffer(pcm, format);
}

void AudioPlaybackEngine::stopAll()
{
    d->positionTimer->stop();
    if (d->output) {
        d->output->stop();
        d->output.reset();
    }
    if (d->bufferDevice) {
        d->bufferDevice->close();
        d->bufferDevice.reset();
    }
    if (d->fileDevice) {
        d->fileDevice->close();
        d->fileDevice.reset();
    }
    d->playing = false;
    d->totalBytes = 0;
    d->durationMs = 0.0;
    emit playbackPositionChanged();
}

bool AudioPlaybackEngine::isPlaying() const
{
    return d->playing;
}

double AudioPlaybackEngine::playbackPositionMs() const
{
    if (!d->playing || !d->output) {
        return 0.0;
    }
    
    // Use processedUSecs() to get accurate position
    qint64 processedUSecs = d->output->processedUSecs();
    return processedUSecs / 1000.0; // Convert microseconds to milliseconds
}

double AudioPlaybackEngine::durationMs() const
{
    return d->durationMs;
}

void AudioPlaybackEngine::updatePlaybackPosition()
{
    if (d->playing) {
        emit playbackPositionChanged();
    }
}

