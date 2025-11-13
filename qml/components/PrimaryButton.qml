import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15
import "../theme" as Theme

Item {
    id: root
    property alias text: label.text
    property bool active: false
    property bool enabled: true
    property bool secondary: false
    property alias font: label.font
    signal clicked()

    // Fixed width to prevent button resizing when text changes
    // Reduced width to fit 4 buttons in segment row
    property int fixedWidth: 140
    implicitWidth: fixedWidth
    implicitHeight: 60  // Increased height for two-line text

    Rectangle {
        id: background
        anchors.fill: parent
        color: secondary ? Theme.Theme.colors.backgroundStart : Theme.Theme.colors.accentShiny
        radius: Theme.Theme.cornerRadius
        opacity: enabled ? 1.0 : 0.5
        layer.enabled: true
        layer.effect: DropShadow {
            color: Theme.Theme.colors.shadowColor
            radius: 12
            samples: 15
            horizontalOffset: 0
            verticalOffset: 4
        }

        // Свечение при наведении
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            enabled: root.enabled
            onEntered: background.color = Theme.Theme.colors.accentWarm
            onExited: background.color = secondary ? Theme.Theme.colors.backgroundStart : Theme.Theme.colors.accentShiny
            onClicked: root.clicked()
            cursorShape: Qt.PointingHandCursor
        }

        Label {
            id: label
            anchors.fill: parent
            anchors.margins: 4
            text: "Label"
            color: "#c41e3a"  // единый цвет текста
            font.family: Theme.Theme.fontFamily
            font.pixelSize: Theme.Theme.baseFontSize
            font.bold: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            maximumLineCount: 2
            elide: Text.ElideRight
        }
    }
}

