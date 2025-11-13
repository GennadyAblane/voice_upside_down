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

    signal recordTriggered()
    signal originalPlayTriggered()
    signal recordingPlayTriggered()
    signal reversePlayTriggered()

    implicitHeight: 120  // Increased to accommodate taller buttons
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
                color: "#c41e3a"  // единый цвет текста
                font.family: Theme.Theme.fontFamily
                font.pixelSize: Theme.Theme.baseFontSize
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }  // Spacer

            Label {
                text: recorded ? qsTr("Записан") : qsTr("Не записан")
                color: "#c41e3a"  // единый цвет текста
            }

            Label {
                text: reversed ? qsTr("Реверс готов") : ""
                color: "#c41e3a"  // единый цвет текста
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
                enabled: root.enabled
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

