#pragma once

#include <QString>
#include <QStringList>

namespace PathUtils {

QString cleanName(const QString &rawName);
QString ensureDirectory(const QString &path);
QString applicationDataRoot();
QString defaultProjectsRoot();
QString defaultCutsRoot();
QString defaultResultsRoot();
QString defaultTempRoot();
QString composeSegmentFile(const QString &baseDir, int segmentIndex);
QString composeSegmentReverseFile(const QString &baseDir, int segmentIndex);
QString composeSongFile(const QString &baseDir, const QString &songName);
QString composeReverseSongFile(const QString &baseDir, const QString &songName);

}

