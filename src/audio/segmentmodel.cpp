#include "segmentmodel.h"

#include "audioproject.h"

#include <QTime>
#include "../utils/logger.h"

namespace {
QString durationToText(qint64 durationMs)
{
    QTime time(0, 0);
    time = time.addMSecs(static_cast<int>(durationMs));
    return time.toString(QStringLiteral("m:ss"));
}
}

SegmentModel::SegmentModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void SegmentModel::setProject(AudioProject *project)
{
    if (m_project == project)
        return;

    if (m_project) {
        disconnect(m_project, nullptr, this, nullptr);
    }

    beginResetModel();
    m_project = project;
    m_lastKnownRowCount = 0;  // Reset tracked count
    endResetModel();

    if (m_project) {
        connect(m_project, &AudioProject::segmentsUpdated, this, &SegmentModel::onSegmentsUpdated);
    }

    emit hasDataChanged();
}

AudioProject *SegmentModel::project() const
{
    return m_project;
}

bool SegmentModel::hasData() const
{
    return m_project && !m_project->segments().isEmpty();
}

int SegmentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_project)
        return 0;
    return m_project->segments().size();
}

QVariant SegmentModel::data(const QModelIndex &index, int role) const
{
    if (!m_project || !index.isValid())
        return {};

    const auto &segments = m_project->segments();
    if (index.row() < 0 || index.row() >= segments.size())
        return {};

    const auto &segment = segments.at(index.row());
    switch (role) {
    case TitleRole:
        return segment.title();
    case DurationRole:
        return segment.durationMs;
    case DurationTextRole:
        return durationToText(segment.durationMs);
    case RecordedRole:
        return segment.hasRecording;
    case ReverseRole:
        return segment.hasReverse;
    case RecordingPathRole:
        return segment.recordingPath;
    case ReversePathRole:
        return segment.reversePath;
    case IndexRole:
        return segment.displayIndex;
    default:
        break;
    }
    return {};
}

QHash<int, QByteArray> SegmentModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TitleRole] = "title";
    roles[DurationRole] = "duration";
    roles[DurationTextRole] = "durationText";
    roles[RecordedRole] = "recorded";
    roles[ReverseRole] = "reversed";
    roles[RecordingPathRole] = "recordingPath";
    roles[ReversePathRole] = "reversePath";
    roles[IndexRole] = "segmentIndex";
    return roles;
}

void SegmentModel::onSegmentsUpdated()
{
    if (!m_project) {
        LOG_INFO() << "SegmentModel: project is null, resetting model";
        m_lastKnownRowCount = 0;
        beginResetModel();
        endResetModel();
        emit hasDataChanged();
        return;
    }
    
    const int newRowCount = m_project->segments().size();
    
    LOG_INFO() << "SegmentModel: segments updated, last known count:" << m_lastKnownRowCount << "new count:" << newRowCount;
    
    // If row count changed, we need to reset the model
    // This happens when segments are first created or when segment length changes
    if (newRowCount != m_lastKnownRowCount) {
        LOG_INFO() << "SegmentModel: row count changed from" << m_lastKnownRowCount << "to" << newRowCount << ", resetting model";
        beginResetModel();
        m_lastKnownRowCount = newRowCount;
        endResetModel();
    } else if (newRowCount > 0) {
        // Row count is the same, just data changed - use dataChanged to preserve scroll position
        LOG_INFO() << "SegmentModel: row count same (" << newRowCount << "), using dataChanged to preserve scroll";
        const QModelIndex topLeft = index(0, 0);
        const QModelIndex bottomRight = index(newRowCount - 1, 0);
        if (topLeft.isValid() && bottomRight.isValid()) {
            emit dataChanged(topLeft, bottomRight);
        } else {
            // Indices not valid yet, reset model
            LOG_INFO() << "SegmentModel: indices not valid, resetting model";
            beginResetModel();
            m_lastKnownRowCount = newRowCount;
            endResetModel();
        }
    } else {
        // Model is empty
        LOG_INFO() << "SegmentModel: model is empty, resetting";
        m_lastKnownRowCount = 0;
        beginResetModel();
        endResetModel();
    }
    
    emit hasDataChanged();
}

