#pragma once

#include "audiobuffer.h"

#include <QVector>
#include <QPair>

struct VolumeLevel
{
    qint64 startFrame = 0;
    qint64 frameCount = 0;
    double rmsLevel = 0.0;  // RMS level normalized to 0.0-1.0
    double peakLevel = 0.0; // Peak level normalized to 0.0-1.0
    bool isQuiet = false;
    bool isLoud = false;
};

class VolumeAnalyzer
{
public:
    // Analyze volume levels in the audio buffer
    // windowSizeMs: size of analysis window in milliseconds (default: 100ms)
    // quietThreshold: RMS level below which is considered quiet (0.0-1.0, default: 0.1)
    // loudThreshold: RMS level above which is considered loud (0.0-1.0, default: 0.7)
    static QVector<VolumeLevel> analyzeVolume(
        const AudioBuffer &buffer,
        int windowSizeMs = 100,
        double quietThreshold = 0.1,
        double loudThreshold = 0.7);

private:
    // Calculate RMS level for a PCM data chunk
    static double calculateRMS(const QByteArray &pcmData, const QAudioFormat &format);
    
    // Calculate peak level for a PCM data chunk
    static double calculatePeak(const QByteArray &pcmData, const QAudioFormat &format);
    
    // Convert sample value to normalized float (-1.0 to 1.0)
    static double sampleToFloat(qint16 sample);
};

