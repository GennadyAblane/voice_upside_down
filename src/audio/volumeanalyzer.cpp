#include "volumeanalyzer.h"

#include <QtGlobal>
#include <cmath>
#include <algorithm>

namespace {
qint64 bytesPerSample(const QAudioFormat &format)
{
    if (!format.isValid())
        return 0;
    const int sampleSize = format.sampleSize();
    if (sampleSize <= 0)
        return 0;
    return sampleSize / 8;
}

qint64 bytesPerFrame(const QAudioFormat &format)
{
    const qint64 sampleBytes = bytesPerSample(format);
    if (sampleBytes <= 0)
        return 0;
    return sampleBytes * format.channelCount();
}
} // namespace

QVector<VolumeLevel> VolumeAnalyzer::analyzeVolume(
    const AudioBuffer &buffer,
    int windowSizeMs,
    double quietThreshold,
    double loudThreshold)
{
    QVector<VolumeLevel> levels;
    
    if (!buffer.format().isValid() || buffer.frameCount() == 0) {
        return levels;
    }
    
    const QAudioFormat &format = buffer.format();
    const QByteArray &data = buffer.data();
    const qint64 sampleRate = format.sampleRate();
    const int channels = format.channelCount();
    
    if (sampleRate <= 0 || channels <= 0) {
        return levels;
    }
    
    // Calculate window size in frames
    const qint64 windowFrames = (windowSizeMs * sampleRate) / 1000;
    if (windowFrames <= 0) {
        return levels;
    }
    
    const qint64 totalFrames = buffer.frameCount();
    const qint64 frameBytes = bytesPerFrame(format);
    
    if (frameBytes <= 0) {
        return levels;
    }
    
    // Analyze in windows
    for (qint64 startFrame = 0; startFrame < totalFrames; startFrame += windowFrames) {
        const qint64 framesInWindow = qMin(windowFrames, totalFrames - startFrame);
        const qint64 startByte = startFrame * frameBytes;
        const qint64 windowBytes = framesInWindow * frameBytes;
        
        if (startByte + windowBytes > data.size()) {
            break;
        }
        
        QByteArray windowData = QByteArray::fromRawData(
            data.constData() + startByte,
            static_cast<int>(windowBytes));
        
        VolumeLevel level;
        level.startFrame = startFrame;
        level.frameCount = framesInWindow;
        level.rmsLevel = calculateRMS(windowData, format);
        level.peakLevel = calculatePeak(windowData, format);
        level.isQuiet = level.rmsLevel < quietThreshold;
        level.isLoud = level.rmsLevel > loudThreshold;
        
        levels.append(level);
    }
    
    return levels;
}

double VolumeAnalyzer::calculateRMS(const QByteArray &pcmData, const QAudioFormat &format)
{
    if (pcmData.isEmpty() || !format.isValid()) {
        return 0.0;
    }
    
    const int sampleSize = format.sampleSize();
    const int channels = format.channelCount();
    
    if (sampleSize != 16 || format.sampleType() != QAudioFormat::SignedInt) {
        // Only support 16-bit signed integer for now
        return 0.0;
    }
    
    const qint16 *samples = reinterpret_cast<const qint16 *>(pcmData.constData());
    const int sampleCount = pcmData.size() / sizeof(qint16);
    
    if (sampleCount == 0) {
        return 0.0;
    }
    
    double sumSquares = 0.0;
    int validSamples = 0;
    
    // For multi-channel audio, we'll use the first channel or average
    // For simplicity, let's use first channel
    for (int i = 0; i < sampleCount; i += channels) {
        const qint16 sample = samples[i];
        const double normalized = sampleToFloat(sample);
        sumSquares += normalized * normalized;
        validSamples++;
    }
    
    if (validSamples == 0) {
        return 0.0;
    }
    
    const double rms = std::sqrt(sumSquares / validSamples);
    return rms;
}

double VolumeAnalyzer::calculatePeak(const QByteArray &pcmData, const QAudioFormat &format)
{
    if (pcmData.isEmpty() || !format.isValid()) {
        return 0.0;
    }
    
    const int sampleSize = format.sampleSize();
    const int channels = format.channelCount();
    
    if (sampleSize != 16 || format.sampleType() != QAudioFormat::SignedInt) {
        return 0.0;
    }
    
    const qint16 *samples = reinterpret_cast<const qint16 *>(pcmData.constData());
    const int sampleCount = pcmData.size() / sizeof(qint16);
    
    if (sampleCount == 0) {
        return 0.0;
    }
    
    double maxAbs = 0.0;
    
    // Find peak across all channels
    for (int i = 0; i < sampleCount; i += channels) {
        const qint16 sample = samples[i];
        const double normalized = std::abs(sampleToFloat(sample));
        if (normalized > maxAbs) {
            maxAbs = normalized;
        }
    }
    
    return maxAbs;
}

double VolumeAnalyzer::sampleToFloat(qint16 sample)
{
    // Convert 16-bit signed integer (-32768 to 32767) to float (-1.0 to 1.0)
    // Normalize to -1.0 to 1.0 range
    const double maxValue = 32768.0;
    return static_cast<double>(sample) / maxValue;
}

