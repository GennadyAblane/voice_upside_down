#pragma once

#include <QAbstractListModel>

class AudioProject;

class SegmentModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY hasDataChanged)
public:
    enum SegmentRoles {
        TitleRole = Qt::UserRole + 1,
        DurationRole,
        DurationTextRole,
        RecordedRole,
        ReverseRole,
        RecordingPathRole,
        ReversePathRole,
        IndexRole
    };
    Q_ENUM(SegmentRoles)

    explicit SegmentModel(QObject *parent = nullptr);

    void setProject(AudioProject *project);
    AudioProject *project() const;

    bool hasData() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void hasDataChanged();

private slots:
    void onSegmentsUpdated();

private:
    AudioProject *m_project = nullptr;
    int m_lastKnownRowCount = 0;  // Track previous row count to detect changes
};

