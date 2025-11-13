#include "audioplaybackengine.h"

#include <QAudioOutput>
#include <QAudio>
#include <QBuffer>
#include <QFile>
#include <QScopedPointer>
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
};

AudioPlaybackEngine::AudioPlaybackEngine(QObject *parent)
    : QObject(parent)
    , d(new Impl())
{
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
}

bool AudioPlaybackEngine::isPlaying() const
{
    return d->playing;
}

