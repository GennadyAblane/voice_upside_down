import QtQuick 2.15
import QtQuick.Controls 2.15
import "../theme" as Theme

Item {
    id: root
    property string text: ""
    property color backgroundColor: Theme.Theme.colors.sectionBackground
    property color textColor: Theme.Theme.colors.textPrimary
    property bool visibleBadge: text.length > 0

    implicitHeight: visibleBadge ? 22 : 0
    implicitWidth: visibleBadge ? label.implicitWidth + Theme.Theme.normalSpacing * 2 : 0
    visible: visibleBadge

    Rectangle {
        anchors.fill: parent
        radius: Theme.Theme.cornerRadius
        color: backgroundColor
        border.width: 1
        border.color: Theme.Theme.colors.borderHighlight
    }

    Label {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: textColor
        font.family: Theme.Theme.fontFamily
        font.pixelSize: Theme.Theme.captionSize
        font.bold: true
    }
}

