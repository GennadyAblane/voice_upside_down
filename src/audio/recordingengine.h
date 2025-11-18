#pragma once

#include <QObject>
#include <QAudioFormat>

class RecordingEngine : public QObject
{
    Q_OBJECT
public:
    explicit RecordingEngine(QObject *parent = nullptr);
    ~RecordingEngine() override;

    bool startRecording(const QString &filePath, const QAudioFormat &format);
    void stop();
    bool isRecording() const;
    
    // Pre-initialize audio device to reduce delay when starting recording
    void prepare(const QAudioFormat &format);

signals:
    // Emitted when microphone is ready and recording has actually started
    void recordingReady();
    // Emitted when recording is fully stopped and saved
    void recordingStopped();

private slots:
    void onStopTimerTimeout();

private:
    class Impl;
    Impl *d;
};

