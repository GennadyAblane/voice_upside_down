import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15
import "../theme" as Theme

Item {
    id: root
    property string title: ""
    property string durationText: ""
    property bool recorded: false
    property bool reversed: false
    property bool recordingActive: false
    property bool originalPlaying: false
    property bool recordingPlaying: false
    property bool reversePlaying: false
    property bool enabled: true
    property bool originalPlaybackEnabled: false
    property int segmentIndex: -1
    property var segmentController: null

    signal recordTriggered()
    signal originalPlayTriggered()
    signal recordingPlayTriggered()
    signal reversePlayTriggered()

    implicitHeight: 200
    implicitWidth: parent ? parent.width : 800

    Rectangle {
        anchors.fill: parent
        color: Theme.Theme.colors.sectionBackground
        radius: Theme.Theme.cornerRadius
        layer.enabled: true
        layer.effect: DropShadow {
            color: Theme.Theme.colors.shadowColor
            radius: 12
            samples: 20
            horizontalOffset: 0
            verticalOffset: 4
            spread: 0.1
        }
        clip: true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.Theme.normalSpacing
        spacing: 6  // Reduced spacing to lift buttons higher

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.Theme.smallSpacing

            Label {
                text: title
                font.family: Theme.Theme.fontFamily
                font.pixelSize: Theme.Theme.headingSize
                color: "#c41e3a"  // единый цвет текста
                Layout.fillWidth: false
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Label {
                text: qsTr("Длительность: %1").arg(durationText)
                color: "#c41e3a"
                font.family: Theme.Theme.fontFamily
                font.pixelSize: Theme.Theme.baseFontSize
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            Label {
                text: recorded ? qsTr("Записан") : qsTr("Не записан")
                color: "#c41e3a"
            }

            Label {
                text: reversed ? qsTr("Реверс готов") : ""
                color: "#c41e3a"
            }
        }

        // Waveform во всю ширину отрезка
        WaveformView {
            id: segmentWaveform
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            visible: root.recorded && root.segmentController && root.segmentIndex >= 0
            noiseThreshold: root.segmentController ? root.segmentController.segmentNoiseThreshold : 0.1
            showSegments: false
            segments: []
            interactive: true  // Enable interaction for trim boundaries
            showTrimBoundaries: root.recorded && root.segmentController && root.segmentIndex >= 0
            
            // Playback position tracking
            playbackPositionMs: {
                if (!root.segmentController || root.segmentIndex < 0)
                    return -1
                // Check if this segment is being played
                var activeIndex = root.segmentController.activePlaybackSegmentIndex
                if (activeIndex === root.segmentIndex) {
                    return root.segmentController.playbackPositionMs
                }
                return -1
            }
            segmentStartMs: {
                if (!root.segmentController || root.segmentIndex < 0)
                    return 0
                // For original playback, we need the segment start in the original audio
                var isOriginal = root.segmentController.isPlayingOriginalSegment
                if (isOriginal && root.segmentController.activePlaybackSegmentIndex === root.segmentIndex) {
                    return root.segmentController.getSegmentStartMs(root.segmentIndex)
                }
                // For recorded playback, we need to account for trimmed start
                // The waveform shows the full recording, but playback starts from trimmed position
                if (!isOriginal && root.segmentController.activePlaybackSegmentIndex === root.segmentIndex) {
                    return root.segmentController.getSegmentRecordingTrimmedStartMs(root.segmentIndex)
                }
                return 0
            }

            property var segmentWaveformData: []
            property var trimBoundaries: ({trimStartMs: -1.0, trimEndMs: -1.0})

            function updateWaveform() {
                if (!root.segmentController || root.segmentIndex < 0 || !root.recorded) {
                    segmentWaveformData = []
                    volumeData = []
                    trimBoundaries = {trimStartMs: -1.0, trimEndMs: -1.0}
                    return
                }
                var data = root.segmentController.analyzeSegmentRecording(root.segmentIndex, 100)
                segmentWaveformData = data
                volumeData = data
                
                // Update trim boundaries
                var boundaries = root.segmentController.getSegmentTrimBoundaries(root.segmentIndex)
                trimBoundaries = boundaries
                trimStartMs = boundaries.trimStartMs || -1.0
                trimEndMs = boundaries.trimEndMs || -1.0
            }
            
            trimStartMs: trimBoundaries.trimStartMs || -1.0
            trimEndMs: trimBoundaries.trimEndMs || -1.0

            Component.onCompleted: {
                if (root.recorded) {
                    Qt.callLater(updateWaveform)
                }
            }

            Connections {
                target: root
                function onRecordedChanged() {
                    if (root.recorded) {
                        Qt.callLater(segmentWaveform.updateWaveform)
                    } else {
                        segmentWaveform.segmentWaveformData = []
                        segmentWaveform.volumeData = []
                    }
                }
            }

            Connections {
                target: root.segmentController
                enabled: root.segmentController !== null
                function onVolumeSettingsChanged() {
                    if (root.recorded && root.segmentIndex >= 0) {
                        Qt.callLater(segmentWaveform.updateWaveform)
                    }
                }
            }
            
            Connections {
                target: segmentWaveform
                function onTrimBoundaryChanged(trimStartMs, trimEndMs) {
                    if (root.segmentController && root.segmentIndex >= 0 && trimStartMs >= 0 && trimEndMs >= 0) {
                        root.segmentController.setSegmentTrimBoundaries(root.segmentIndex, trimStartMs, trimEndMs)
                    }
                }
            }
            
            Connections {
                target: root.segmentController
                enabled: root.segmentController !== null
                function onSegmentsUpdated() {
                    if (root.recorded && root.segmentIndex >= 0) {
                        Qt.callLater(segmentWaveform.updateWaveform)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.Theme.smallSpacing

            PrimaryButton {
                Layout.preferredWidth: 140
                Layout.fillWidth: false
                text: recordingActive ? qsTr("Стоп") : qsTr("Записать")
                enabled: root.enabled
                onClicked: root.recordTriggered()
            }

            PrimaryButton {
                Layout.preferredWidth: 140
                Layout.fillWidth: false
                text: recordingPlaying ? qsTr("Стоп") : qsTr("Воспроизвести\nзапись")
                secondary: true
                enabled: root.enabled && recorded
                onClicked: root.recordingPlayTriggered()
            }

            PrimaryButton {
                Layout.preferredWidth: 140
                Layout.fillWidth: false
                text: originalPlaying ? qsTr("Стоп") : qsTr("Воспроизвести\nоригинал")
                secondary: true
                enabled: root.enabled && root.originalPlaybackEnabled
                onClicked: root.originalPlayTriggered()
            }

            PrimaryButton {
                Layout.preferredWidth: 140
                Layout.fillWidth: false
                text: reversePlaying ? qsTr("Стоп") : qsTr("Реверс\nоригинал")
                secondary: true
                enabled: root.enabled
                onClicked: root.reversePlayTriggered()
            }
        }
    }
}
