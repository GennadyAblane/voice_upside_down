#pragma once

#include <QObject>
#include <QAudioFormat>
#include <QByteArray>

class AudioPlaybackEngine : public QObject
{
    Q_OBJECT
public:
    explicit AudioPlaybackEngine(QObject *parent = nullptr);
    ~AudioPlaybackEngine() override;

    bool playBuffer(const QByteArray &pcm, const QAudioFormat &format);
    bool playFile(const QString &filePath);
    void stopAll();
    bool isPlaying() const;

signals:
    void playbackFinished();
    void playbackError(const QString &message);

private:
    class Impl;
    Impl *d;
};

