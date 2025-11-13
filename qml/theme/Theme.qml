pragma Singleton
import QtQuick 2.15
import "." as ThemeElements

QtObject {
    readonly property QtObject colors: ThemeElements.Colors

    readonly property string fontFamily: ""  // Use default system font
    readonly property int baseFontSize: 14
    readonly property int captionSize: 12
    readonly property int headingSize: 20

    readonly property real cornerRadius: 6
    readonly property real smallSpacing: 6
    readonly property real normalSpacing: 12
    readonly property real largeSpacing: 20
}

