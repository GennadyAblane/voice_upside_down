#pragma once

#include <QAudioFormat>

// Qt 6 audio API helper functions

#include <QAudioDevice>
#include <QMediaDevices>
#include <QAudioSink>
#include <QAudioSource>

namespace Qt6Compat {

// Get sample size in bits from QAudioFormat
inline int sampleSize(const QAudioFormat &format) {
    switch (format.sampleFormat()) {
    case QAudioFormat::Int16:
        return 16;
    case QAudioFormat::Int32:
        return 32;
    case QAudioFormat::Float:
        return 32;
    case QAudioFormat::UInt8:
        return 8;
    default:
        return 0;
    }
}

// Set sample size by setting appropriate sample format
inline void setSampleSize(QAudioFormat &format, int size) {
    if (size == 16) {
        format.setSampleFormat(QAudioFormat::Int16);
    } else if (size == 32) {
        format.setSampleFormat(QAudioFormat::Int32);
    } else if (size == 8) {
        format.setSampleFormat(QAudioFormat::UInt8);
    }
}

// Check if format is signed integer
inline bool isSignedInt(const QAudioFormat &format) {
    return format.sampleFormat() == QAudioFormat::Int16 || 
           format.sampleFormat() == QAudioFormat::Int32;
}

// Set format to signed 16-bit integer
inline void setSignedInt(QAudioFormat &format) {
    format.setSampleFormat(QAudioFormat::Int16);
}

// Set PCM format (16-bit signed integer)
inline void setPcmFormat(QAudioFormat &format) {
    format.setSampleFormat(QAudioFormat::Int16);
}

} // namespace Qt6Compat

