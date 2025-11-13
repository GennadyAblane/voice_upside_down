#include "projectserializer.h"

#include "../audio/audioproject.h"
#include "../utils/logger.h"
#include "../utils/pathutils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ProjectSerializer::ProjectSerializer(QObject *parent)
    : QObject(parent)
{
}

bool ProjectSerializer::save(const QString &directoryPath, const AudioProject &project, QString *errorString)
{
    LOG_INFO() << "Saving project to" << directoryPath;

    QDir dir(directoryPath);
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            const QString err = tr("Не удалось создать директорию проекта: %1").arg(directoryPath);
            if (errorString)
                *errorString = err;
            LOG_WARN() << "Failed to create project directory:" << directoryPath;
            return false;
        }
    }

    // Copy original file to project directory
    const QString originalPath = project.originalFilePath();
    if (!originalPath.isEmpty() && QFileInfo::exists(originalPath)) {
        const QString destOriginal = dir.absoluteFilePath(QFileInfo(originalPath).fileName());
        if (destOriginal != originalPath && !QFile::copy(originalPath, destOriginal)) {
            const QString err = tr("Не удалось скопировать исходный файл");
            if (errorString)
                *errorString = err;
            LOG_WARN() << "Failed to copy original file:" << originalPath << "to" << destOriginal;
            return false;
        }
        LOG_INFO() << "Copied original file to" << destOriginal;
    }

    // Copy segment recording files
    const QString cutsDir = PathUtils::defaultCutsRoot();
    for (const auto &segment : project.segments()) {
        if (segment.hasRecording && !segment.recordingPath.isEmpty() && QFileInfo::exists(segment.recordingPath)) {
            const QString destSegment = dir.absoluteFilePath(QFileInfo(segment.recordingPath).fileName());
            if (destSegment != segment.recordingPath && !QFile::copy(segment.recordingPath, destSegment)) {
                LOG_WARN() << "Failed to copy segment file:" << segment.recordingPath << "to" << destSegment;
            } else {
                LOG_INFO() << "Copied segment file to" << destSegment;
            }
        }
        if (segment.hasReverse && !segment.reversePath.isEmpty() && QFileInfo::exists(segment.reversePath)) {
            const QString destReverse = dir.absoluteFilePath(QFileInfo(segment.reversePath).fileName());
            if (destReverse != segment.reversePath && !QFile::copy(segment.reversePath, destReverse)) {
                LOG_WARN() << "Failed to copy reverse file:" << segment.reversePath << "to" << destReverse;
            } else {
                LOG_INFO() << "Copied reverse file to" << destReverse;
            }
        }
    }

    // Create JSON document
    QJsonObject root;
    root[QStringLiteral("projectName")] = project.projectName();
    root[QStringLiteral("originalFilePath")] = originalPath.isEmpty() ? QString() : QFileInfo(originalPath).fileName();
    root[QStringLiteral("segmentLengthSeconds")] = project.segmentLengthSeconds();

    QJsonArray segmentsArray;
    for (const auto &segment : project.segments()) {
        QJsonObject segObj;
        segObj[QStringLiteral("displayIndex")] = segment.displayIndex;
        segObj[QStringLiteral("startFrame")] = static_cast<qint64>(segment.startFrame);
        segObj[QStringLiteral("frameCount")] = static_cast<qint64>(segment.frameCount);
        segObj[QStringLiteral("hasRecording")] = segment.hasRecording;
        segObj[QStringLiteral("hasReverse")] = segment.hasReverse;
        segObj[QStringLiteral("recordingPath")] = segment.recordingPath.isEmpty() ? QString() : QFileInfo(segment.recordingPath).fileName();
        segObj[QStringLiteral("reversePath")] = segment.reversePath.isEmpty() ? QString() : QFileInfo(segment.reversePath).fileName();
        segmentsArray.append(segObj);
    }
    root[QStringLiteral("segments")] = segmentsArray;

    // Save JSON file
    const QString jsonPath = dir.absoluteFilePath(QStringLiteral("project.json"));
    QFile jsonFile(jsonPath);
    if (!jsonFile.open(QIODevice::WriteOnly)) {
        const QString err = tr("Не удалось открыть файл проекта для записи: %1").arg(jsonFile.errorString());
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Failed to open project JSON for writing:" << jsonFile.errorString();
        return false;
    }

    QJsonDocument doc(root);
    jsonFile.write(doc.toJson());
    jsonFile.close();

    LOG_INFO() << "Project saved successfully to" << jsonPath;
    return true;
}

bool ProjectSerializer::load(const QString &projectFilePath, AudioProject &project, QString *errorString)
{
    LOG_INFO() << "Loading project from" << projectFilePath;

    QFileInfo fileInfo(projectFilePath);
    if (!fileInfo.exists()) {
        const QString err = tr("Файл проекта не найден: %1").arg(projectFilePath);
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Project file not found:" << projectFilePath;
        return false;
    }

    QDir projectDir = fileInfo.absoluteDir();

    // Read JSON file
    QFile jsonFile(projectFilePath);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        const QString err = tr("Не удалось открыть файл проекта: %1").arg(jsonFile.errorString());
        if (errorString)
            *errorString = err;
        LOG_WARN() << "Failed to open project JSON:" << jsonFile.errorString();
        return false;
    }

    QByteArray jsonData = jsonFile.readAll();
    jsonFile.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        const QString err = tr("Ошибка парсинга JSON: %1").arg(parseError.errorString());
        if (errorString)
            *errorString = err;
        LOG_WARN() << "JSON parse error:" << parseError.errorString();
        return false;
    }

    QJsonObject root = doc.object();

    // Load project properties
    project.setProjectName(root[QStringLiteral("projectName")].toString());
    const QString originalFileName = root[QStringLiteral("originalFilePath")].toString();
    if (!originalFileName.isEmpty()) {
        const QString originalPath = projectDir.absoluteFilePath(originalFileName);
        project.setOriginalFilePath(originalPath);
        LOG_INFO() << "Original file path:" << originalPath;
    }

    project.setSegmentLengthSeconds(root[QStringLiteral("segmentLengthSeconds")].toInt(5));

    // Load segments
    QJsonArray segmentsArray = root[QStringLiteral("segments")].toArray();
    auto &segments = project.segments();
    segments.clear();

    for (const QJsonValue &value : segmentsArray) {
        QJsonObject segObj = value.toObject();
        SegmentInfo segment;
        segment.displayIndex = segObj[QStringLiteral("displayIndex")].toInt();
        segment.startFrame = static_cast<qint64>(segObj[QStringLiteral("startFrame")].toVariant().toLongLong());
        segment.frameCount = static_cast<qint64>(segObj[QStringLiteral("frameCount")].toVariant().toLongLong());
        segment.hasRecording = segObj[QStringLiteral("hasRecording")].toBool();
        segment.hasReverse = segObj[QStringLiteral("hasReverse")].toBool();

        const QString recordingFileName = segObj[QStringLiteral("recordingPath")].toString();
        if (!recordingFileName.isEmpty()) {
            segment.recordingPath = projectDir.absoluteFilePath(recordingFileName);
        }

        const QString reverseFileName = segObj[QStringLiteral("reversePath")].toString();
        if (!reverseFileName.isEmpty()) {
            segment.reversePath = projectDir.absoluteFilePath(reverseFileName);
        }

        segments.append(segment);
    }

    LOG_INFO() << "Project loaded successfully:" << project.projectName() << "segments:" << segments.size();
    return true;
}

