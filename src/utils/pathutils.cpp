#include "pathutils.h"

#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>

namespace PathUtils {

static QString ensureTrailingSlash(const QString &path)
{
    if (path.endsWith(QDir::separator()))
        return path;
    return path + QDir::separator();
}

QString cleanName(const QString &rawName)
{
    QString cleaned = rawName;
    cleaned.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_]")), QStringLiteral("_"));
    cleaned = cleaned.trimmed();
    if (cleaned.isEmpty())
        cleaned = QStringLiteral("song");
    return cleaned;
}

QString ensureDirectory(const QString &path)
{
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    return dir.absolutePath();
}

QString applicationDataRoot()
{
    const QString base = QCoreApplication::applicationDirPath();
    return ensureDirectory(base);
}

QString defaultProjectsRoot()
{
    return ensureDirectory(applicationDataRoot() + QDir::separator() + QStringLiteral("projects"));
}

QString defaultCutsRoot()
{
    return ensureDirectory(applicationDataRoot() + QDir::separator() + QStringLiteral("cut"));
}

QString defaultResultsRoot()
{
    return ensureDirectory(applicationDataRoot() + QDir::separator() + QStringLiteral("result song"));
}

QString defaultTempRoot()
{
    return ensureDirectory(applicationDataRoot() + QDir::separator() + QStringLiteral("temp recordings"));
}

QString composeSegmentFile(const QString &baseDir, int segmentIndex)
{
    return ensureTrailingSlash(baseDir) + QStringLiteral("segment_%1.wav").arg(segmentIndex, 2, 10, QLatin1Char('0'));
}

QString composeSegmentReverseFile(const QString &baseDir, int segmentIndex)
{
    return ensureTrailingSlash(baseDir) + QStringLiteral("segment_%1_reverse.wav").arg(segmentIndex, 2, 10, QLatin1Char('0'));
}

QString composeSongFile(const QString &baseDir, const QString &songName)
{
    return ensureTrailingSlash(baseDir) + QStringLiteral("song_%1.wav").arg(songName);
}

QString composeReverseSongFile(const QString &baseDir, const QString &songName)
{
    return ensureTrailingSlash(baseDir) + QStringLiteral("reverse_%1.wav").arg(songName);
}

}

