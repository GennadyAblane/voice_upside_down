#pragma once

#include <QObject>
#include <QAudioFormat>
#include <QByteArray>

class AudioPlaybackEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double playbackPositionMs READ playbackPositionMs NOTIFY playbackPositionChanged)
public:
    explicit AudioPlaybackEngine(QObject *parent = nullptr);
    ~AudioPlaybackEngine() override;

    bool playBuffer(const QByteArray &pcm, const QAudioFormat &format);
    bool playFile(const QString &filePath);
    void stopAll();
    bool isPlaying() const;
    
    // Get current playback position in milliseconds
    double playbackPositionMs() const;
    
    // Get total duration in milliseconds
    double durationMs() const;

signals:
    void playbackFinished();
    void playbackError(const QString &message);
    void playbackPositionChanged();

private slots:
    void updatePlaybackPosition();

private:
    class Impl;
    Impl *d;
};

