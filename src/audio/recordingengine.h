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

private:
    class Impl;
    Impl *d;
};

