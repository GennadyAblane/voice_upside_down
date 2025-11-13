#pragma once

#include <QAudioFormat>
#include <QByteArray>
#include <QString>

namespace WavUtils {

bool readWavFile(const QString &filePath, QByteArray &pcmData, QAudioFormat &format, QString *errorString = nullptr);
bool writeWavFile(const QString &filePath, const QAudioFormat &format, const QByteArray &pcmData, QString *errorString = nullptr);

}
