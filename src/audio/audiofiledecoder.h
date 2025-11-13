#pragma once

#include "audiobuffer.h"

#include <QObject>

class AudioFileDecoder : public QObject
{
    Q_OBJECT
public:
    explicit AudioFileDecoder(QObject *parent = nullptr);

    bool decodeFile(const QString &filePath, AudioBuffer &outBuffer, QString *errorString = nullptr);
};

