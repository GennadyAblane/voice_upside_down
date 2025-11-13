pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property color backgroundStart: "#F9FAF8"    // молочно-белый
    readonly property color backgroundEnd: "#D0E7F9"      // светло-голубой
    readonly property color accentLight: "#C6D9F1"        // ледяной сиреневый
    readonly property color accentShiny: "#E6E8EB"        // светлое серебро
    readonly property color accentWarm: "#F5E8B6"         // очень светлое золото
    readonly property color textPrimary: "#FFFFFE"        // белый текст
    readonly property color shadowColor: "#A0B8D8"        // мягкая голубая тень
    
    // Для обратной совместимости (если где-то используются старые имена)
    readonly property color background: backgroundStart
    readonly property color sectionBackground: accentLight
    readonly property color button: accentShiny
    readonly property color buttonDisabled: "#7F7F7F"
    readonly property color borderHighlight: accentWarm
    readonly property color textSecondary: "#888888"
}

