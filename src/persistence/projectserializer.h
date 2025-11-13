#pragma once

#include <QObject>
#include <QString>

class AudioProject;

class ProjectSerializer : public QObject
{
    Q_OBJECT
public:
    explicit ProjectSerializer(QObject *parent = nullptr);

    bool save(const QString &directoryPath, const AudioProject &project, QString *errorString = nullptr);
    bool load(const QString &projectFilePath, AudioProject &project, QString *errorString = nullptr);
};

