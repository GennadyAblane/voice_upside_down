import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import QtGraphicalEffects 1.15
import "components"
import "theme" as Theme

ApplicationWindow {
    id: window
    width: 1280
    height: 760
    visible: true
    title: qsTr("Voice Upside Down")
    font.family: Theme.Theme.fontFamily
    
    // Свойства для анимации градиента
    property color animatedBackgroundStart: Theme.Theme.colors.backgroundStart
    property color animatedBackgroundEnd: Theme.Theme.colors.backgroundEnd
    
    // Динамический перелив фона
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: animatedBackgroundStart }
            GradientStop { position: 1.0; color: animatedBackgroundEnd }
        }
    }
    
    
    // Анимация плавного изменения градиента
    SequentialAnimation on animatedBackgroundStart {
        running: true
        loops: Animation.Infinite
        ColorAnimation { to: Theme.Theme.colors.backgroundEnd; duration: 4000; easing.type: Easing.InOutQuad }
        ColorAnimation { to: Theme.Theme.colors.backgroundStart; duration: 4000; easing.type: Easing.InOutQuad }
    }
    SequentialAnimation on animatedBackgroundEnd {
        running: true
        loops: Animation.Infinite
        ColorAnimation { to: Theme.Theme.colors.backgroundStart; duration: 4000; easing.type: Easing.InOutQuad }
        ColorAnimation { to: Theme.Theme.colors.backgroundEnd; duration: 4000; easing.type: Easing.InOutQuad }
    }

    // Эффект мерцания снега по всему экрану
    Item {
        anchors.fill: parent
        z: 1000  // Поверх всего контента
        clip: false

        Repeater {
            model: 50  // Количество мерцающих точек
            Rectangle {
                id: sparkle
                property real sparkleSize: (index % 5) * 0.8 + 2  // Размер от 2 до 5.2 пикселей
                property real sparkleX: (index * 37) % (window.width - sparkleSize)  // Псевдослучайное распределение
                property real sparkleY: (index * 73) % (window.height - sparkleSize)
                property real sparkleDelay: (index * 47) % 2000  // Задержка от 0 до 2000 мс
                property real sparkleDuration: 500 + ((index * 31) % 1000)  // Длительность от 500 до 1500 мс
                property real sparkleMaxOpacity: 0.7 + ((index * 19) % 30) / 100.0  // Максимальная прозрачность от 0.7 до 1.0
                
                width: sparkleSize
                height: sparkleSize
                radius: sparkleSize / 2
                color: "#FFFFFF"  // белый цвет для мерцания
                x: sparkleX
                y: sparkleY
                
                // Анимация мерцания
                SequentialAnimation on opacity {
                    running: true
                    loops: Animation.Infinite
                    PauseAnimation { duration: sparkleDelay }
                    NumberAnimation { 
                        to: 0.0
                        duration: sparkleDuration
                        easing.type: Easing.InOutQuad
                    }
                    NumberAnimation { 
                        to: sparkleMaxOpacity
                        duration: sparkleDuration
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }
    }

    property var controller: appController

    header: Rectangle {
        height: 72
        gradient: Gradient {
            GradientStop { position: 0.0; color: animatedBackgroundStart }
            GradientStop { position: 1.0; color: animatedBackgroundEnd }
        }
        layer.enabled: true
        layer.effect: DropShadow {
            color: Theme.Theme.colors.shadowColor
            radius: 14
            samples: 16
            horizontalOffset: 0
            verticalOffset: 2
        }

        Label {
            anchors.centerIn: parent
            text: qsTr("Мастерская голоса")
            font.pixelSize: 28
            font.bold: true
            color: "#c41e3a"  // единый цвет текста
        }
    }

    footer: Rectangle {
        height: 36
        gradient: Gradient {
            GradientStop { position: 0.0; color: animatedBackgroundStart }
            GradientStop { position: 1.0; color: animatedBackgroundEnd }
        }
        layer.enabled: true
        layer.effect: DropShadow {
            color: Theme.Theme.colors.shadowColor
            radius: 12
            samples: 10
            horizontalOffset: 0
            verticalOffset: 1
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 24
            text: controller ? controller.statusMessage : qsTr("Готово")
            color: "#c41e3a"  // единый цвет текста
            font.pixelSize: Theme.Theme.baseFontSize
        }
        
        // Switch для управления воспроизведением оригиналов - в незаметном месте
        Switch {
            id: originalPlaybackSwitch
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 24
            visible: controller && controller.segmentModel && controller.segmentModel.hasData
            checked: controller ? controller.originalPlaybackEnabled : false
            opacity: 0.3  // Полупрозрачный для незаметности
            onToggled: {
                if (controller) {
                    controller.originalPlaybackEnabled = checked
                }
            }
        }
    }

    ColumnLayout {
        anchors {
            top: parent.top
            topMargin: header.height
            bottom: parent.bottom
            bottomMargin: footer.height
            left: parent.left
            right: parent.right
            margins: 24
        }
        spacing: 24

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 24

            Rectangle {
                Layout.preferredWidth: 280
                Layout.minimumWidth: 240
                Layout.maximumWidth: 320
                Layout.fillHeight: true
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Theme.Theme.colors.accentLight }
                    GradientStop { position: 1.0; color: Qt.lighter(Theme.Theme.colors.accentLight, 1.1) }
                }
                       radius: Theme.Theme.cornerRadius
                       layer.enabled: true
                       layer.effect: DropShadow {
                           color: Theme.Theme.colors.shadowColor
                           radius: 8
                           samples: 16
                           horizontalOffset: 0
                           verticalOffset: 3
                       }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        PrimaryButton {
                            Layout.fillWidth: true
                            text: qsTr("Загрузить файл")
                            onClicked: openFileDialog.open()
                        }
                        PrimaryButton {
                            Layout.fillWidth: true
                            text: controller && controller.sourceRecording ? qsTr("Выключить микрофон") : qsTr("Включить микрофон")
                            enabled: controller !== null
                            onClicked: {
                                if (!controller)
                                    return
                                if (controller.sourceRecording)
                                    controller.stopSourceRecording()
                                else
                                    controller.startSourceRecording()
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Label {
                            text: qsTr("Длина отрезка")
                            color: "#c41e3a"  // единый цвет текста
                            Layout.alignment: Qt.AlignVCenter
                        }
                        SpinBox {
                            id: segmentLengthSpin
                            Layout.preferredWidth: 100
                            from: 1
                            to: 10
                            enabled: controller && controller.canAdjustSegmentLength
                            value: controller ? controller.segmentLength : 5
                            onValueModified: controller ? controller.changeSegmentLength(value) : null
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: controller ? controller.currentSourceName : qsTr("Песня не выбрана")
                        color: "#c41e3a"  // единый цвет текста
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Theme.Theme.colors.accentLight }
                    GradientStop { position: 1.0; color: Qt.lighter(Theme.Theme.colors.accentLight, 1.1) }
                }
                       radius: Theme.Theme.cornerRadius
                       layer.enabled: true
                       layer.effect: DropShadow {
                           color: Theme.Theme.colors.shadowColor
                           radius: 8
                           samples: 16
                           horizontalOffset: 0
                           verticalOffset: 3
                       }
                
                // Ёлка в рамке с отрезками (за отрезками, но над фоном)
                // Показывается только если отрезков 2 или меньше
                Image {
                    id: treeImage
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 20
                    anchors.bottomMargin: 20
                    width: 300  // Исходный размер
                    height: 400  // Исходный размер
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/el.png"
                    opacity: 0.6
                    z: 0 // Над фоном, но под ListView
                    visible: segmentList.count <= 2  // Показывать только если 2 или меньше отрезков
                }

                ListView {
                    id: segmentList
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12
                    clip: true
                    z: 1 // Поверх ёлки
                    model: controller ? controller.segmentModel : null
                    
                    // Save scroll position to restore after model updates
                    property real savedContentY: 0
                    property bool restoringPosition: false
                    property bool userScrolling: false
                    
                    // Save position when user scrolls (not when programmatically changed)
                    onContentYChanged: {
                        if (!restoringPosition && contentY >= 0) {
                            // Always save current position (will be filtered by userScrolling flag)
                            if (userScrolling || Math.abs(contentY - savedContentY) > 5) {
                                // Only save if user is scrolling or significant change
                                savedContentY = contentY
                            }
                        }
                    }
                    
                    // Track when user is actively scrolling
                    onMovementStarted: {
                        userScrolling = true
                    }
                    
                    onMovementEnded: {
                        userScrolling = false
                        if (contentY >= 0) {
                            savedContentY = contentY
                        }
                    }
                    
                    // Also save position before model might reset
                    Component.onCompleted: {
                        // Save initial position
                        if (contentY >= 0) {
                            savedContentY = contentY
                        }
                    }
                    
                    // Restore position after model updates
                    onCountChanged: {
                        if (savedContentY > 0 && !restoringPosition && count > 0) {
                            restoreScrollPosition()
                        }
                    }
                    
                    // Also restore when model changes
                    onModelChanged: {
                        if (savedContentY > 0 && !restoringPosition && model && model.count > 0) {
                            restoreScrollPosition()
                        }
                    }
                    
                    // Function to restore scroll position
                    function restoreScrollPosition() {
                        if (restoringPosition)
                            return
                            
                        restoringPosition = true
                        // Use multiple attempts to ensure position is restored
                        var attempts = 0
                        function tryRestore() {
                            attempts++
                            if (segmentList.contentHeight > 0) {
                                if (segmentList.contentHeight >= segmentList.savedContentY) {
                                    segmentList.contentY = segmentList.savedContentY
                                    segmentList.restoringPosition = false
                                } else if (attempts < 3) {
                                    // Try again after a short delay if content height not ready
                                    Qt.callLater(tryRestore)
                                } else {
                                    // Fallback: scroll to bottom if saved position is too large
                                    segmentList.contentY = Math.max(0, segmentList.contentHeight - segmentList.height)
                                    segmentList.restoringPosition = false
                                }
                            } else if (attempts < 5) {
                                // Content height not ready yet, try again
                                Qt.callLater(tryRestore)
                            } else {
                                segmentList.restoringPosition = false
                            }
                        }
                        Qt.callLater(tryRestore)
                    }
                    
                    delegate: SegmentRow {
                        width: segmentList.width - 32
                        title: model.title
                        durationText: model.durationText
                        recorded: model.recorded
                        reversed: model.reversed
                        recordingActive: controller ? controller.isSegmentRecording(model.segmentIndex) : false
                        originalPlaying: controller ? controller.isSegmentOriginalPlaying(model.segmentIndex) : false
                        recordingPlaying: controller ? controller.isSegmentRecordingPlaying(model.segmentIndex) : false
                        reversePlaying: controller ? controller.isSegmentReversePlaying(model.segmentIndex) : false
                        enabled: controller ? controller.interactionsEnabled : false
                        originalPlaybackEnabled: controller ? controller.originalPlaybackEnabled : false

                        onRecordTriggered: controller ? controller.toggleSegmentRecording(model.segmentIndex) : null
                        onOriginalPlayTriggered: controller ? controller.toggleSegmentOriginalPlayback(model.segmentIndex) : null
                        onRecordingPlayTriggered: controller ? controller.toggleSegmentRecordedPlayback(model.segmentIndex) : null
                        onReversePlayTriggered: controller ? controller.toggleSegmentReversePlayback(model.segmentIndex) : null
                    }
                    ScrollBar.vertical: ScrollBar { }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            PrimaryButton {
                Layout.preferredWidth: 180
                text: qsTr("Склеить отрезки")
                enabled: controller ? controller.canGlue : false
                onClicked: controller ? controller.glueSegments() : null
            }
            PrimaryButton {
                Layout.preferredWidth: 200
                text: controller && controller.gluePlaybackActive ? qsTr("Стоп склейку") : qsTr("Воспроизвести склейку")
                secondary: true
                enabled: controller ? controller.canPlayGlue : false
                onClicked: controller ? controller.toggleGluePlayback() : null
            }
            PrimaryButton {
                Layout.preferredWidth: 200
                text: qsTr("Перевернуть песню")
                enabled: controller ? controller.canReverseSong : false
                onClicked: controller ? controller.reverseSong() : null
            }
            PrimaryButton {
                Layout.preferredWidth: 200
                text: controller && controller.reversePlaybackActive ? qsTr("Стоп реверс") : qsTr("Воспроизвести реверс")
                secondary: true
                enabled: controller ? controller.canPlayReverse : false
                onClicked: controller ? controller.toggleSongReversePlayback() : null
            }
            PrimaryButton {
                Layout.preferredWidth: 200
                text: qsTr("Сохранить результаты")
                secondary: true
                enabled: controller && controller.canSaveResults
                onClicked: saveResultsDialog.open()
            }
            Item { Layout.fillWidth: true }
        }
    }

    FileDialog {
        id: openFileDialog
        title: qsTr("Выберите аудиофайл")
        nameFilters: ["Audio files (*.mp3 *.wav)", "All files (*)"]
        onAccepted: {
            if (!controller)
                return;
            var url = fileUrl;
            if (!url)
                return;
            var path = url;
            if (url.toLocalFile !== undefined)
                path = url.toLocalFile();
            controller.loadAudioSource(path);
        }
    }

    FileDialog {
        id: saveResultsDialog
        title: qsTr("Сохранить результаты")
        nameFilters: ["WAV files (*.wav)", "All files (*)"]
        selectExisting: false
        defaultSuffix: "wav"
        onAccepted: {
            if (controller) {
                var filePath = "";
                
                // Convert URL to local file path (Windows example)
                if (fileUrl) {
                    if (typeof fileUrl.toLocalFile === "function") {
                        filePath = fileUrl.toLocalFile();
                    } else if (typeof fileUrl === "string") {
                        // Fallback: handle string manually
                        filePath = fileUrl;
                        if (filePath.indexOf("file://") === 0) {
                            filePath = filePath.substring(7); // Remove "file://"
                            while (filePath.length > 0 && filePath.charAt(0) === '/') {
                                filePath = filePath.substring(1);
                            }
                        }
                        try {
                            filePath = decodeURIComponent(filePath);
                        } catch (e) {
                            // If decode fails, use as-is
                        }
                    } else {
                        // Try toString() as last resort
                        try {
                            var urlString = String(fileUrl);
                            filePath = urlString;
                            if (filePath.indexOf("file://") === 0) {
                                filePath = filePath.substring(7);
                                while (filePath.length > 0 && filePath.charAt(0) === '/') {
                                    filePath = filePath.substring(1);
                                }
                            }
                            try {
                                filePath = decodeURIComponent(filePath);
                            } catch (e) {
                                // If decode fails, use as-is
                            }
                        } catch (e) {
                            console.error("Failed to convert fileUrl to string:", e);
                        }
                    }
                }
                
                // Ensure we have a valid string path
                if (!filePath || typeof filePath !== "string" || filePath.length === 0) {
                    console.error("Invalid file path:", filePath, "fileUrl type:", typeof fileUrl, "fileUrl:", fileUrl);
                    return;
                }
                
                controller.saveProjectTo(filePath);
            }
        }
    }

    // Snowflakes falling across entire screen - always visible
    Item {
        id: screenSnowflakes
        anchors.fill: parent
        visible: true // Always visible
        z: 9999 // Below dialog but above content

        // Snowflake component
        Component {
            id: screenSnowflakeStyle

            Rectangle {
                width: size
                height: size
                radius: size / 2
                color: "white"
                opacity: 0.7

                layer.enabled: true
                layer.effect: DropShadow {
                    color: "#80ffffff"
                    radius: size * 0.8
                    samples: 12
                    horizontalOffset: 0
                    verticalOffset: 0
                }

                readonly property real size: 5 // Reduced by 3 times (15 -> 5)
            }
        }

        // Animated snowflakes across entire screen - one at a time, randomly distributed
        Repeater {
            model: 50 // More snowflakes for better coverage
            delegate: Loader {
                id: screenSnowflakeLoader
                sourceComponent: screenSnowflakeStyle
                x: ((index * 73 + index * 41 + index * 19) % window.width)
                y: -5
                
                property real horizontalDrift: ((index % 7) - 3) * 1.2
                property real startX: ((index * 73 + index * 41 + index * 19) % window.width)
                property real startDelay: (index * 97) % 3000 // Random delay up to 3 seconds
                
                SequentialAnimation on y {
                    running: true // Always running
                    loops: Animation.Infinite
                    PauseAnimation { duration: screenSnowflakeLoader.startDelay }
                    NumberAnimation {
                        to: window.height + 5
                        duration: 4000 + (index % 7) * 1000
                        easing.type: Easing.Linear
                    }
                    PropertyAction {
                        target: screenSnowflakeLoader
                        property: "x"
                        value: ((index * 73 + index * 41 + index * 19 + (index % 13) * 23) % window.width)
                    }
                    PropertyAction {
                        target: screenSnowflakeLoader
                        property: "y"
                        value: -5
                    }
                    PropertyAction {
                        target: screenSnowflakeLoader
                        property: "startDelay"
                        value: (index * 97 + (index % 11) * 31) % 3000
                    }
                }
                
                SequentialAnimation on x {
                    running: true // Always running
                    loops: Animation.Infinite
                    PauseAnimation { duration: screenSnowflakeLoader.startDelay }
                    NumberAnimation {
                        to: screenSnowflakeLoader.startX + screenSnowflakeLoader.horizontalDrift * 40
                        duration: 4000 + (index % 7) * 1000
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: screenSnowflakeLoader.startX - screenSnowflakeLoader.horizontalDrift * 40
                        duration: 4000 + (index % 7) * 1000
                        easing.type: Easing.InOutSine
                    }
                }
            }
        }
    }

    // Recording dialog - shows when recording starts
    Item {
        id: recordingDialog
        anchors.fill: parent
        visible: controller && controller.recordingDialogVisible
        z: 10000 // On top of everything

        // Semi-transparent black overlay
        Rectangle {
            anchors.fill: parent
            color: "#80000000"
        }

        // Block all mouse interactions with background
        MouseArea {
            anchors.fill: parent
            enabled: recordingDialog.visible
            // Don't propagate clicks to background
            onClicked: {
                // Do nothing - just block clicks
            }
        }

        // Handle keyboard events
        Keys.onPressed: {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                event.accepted = true;
                if (controller) {
                    controller.stopCurrentRecording();
                }
            }
        }

        // Make dialog focusable to receive keyboard events
        focus: visible

        Rectangle {
            id: dialogContent
            anchors.centerIn: parent
            width: 400
            height: 250
            radius: 16
            color: controller && controller.recordingReady ? "#dbffed" : "#ffc2cc"
            clip: true // Clip snowflakes to dialog bounds
            
            // Debug: log color changes
            onColorChanged: {
                console.log("Dialog color changed to:", color, "recordingReady:", controller ? controller.recordingReady : "no controller")
            }
            
            Behavior on color {
                ColorAnimation { duration: 300 }
            }

            layer.enabled: true
            layer.effect: DropShadow {
                color: "#40000000"
                radius: 20
                samples: 32
                verticalOffset: 4
            }

            // Snowflakes falling on dialog - one at a time, randomly distributed
            Repeater {
                model: 20 // Number of snowflakes on dialog
                delegate: Loader {
                    id: dialogSnowflakeLoader
                    sourceComponent: screenSnowflakeStyle
                    x: ((index * 43 + index * 29 + index * 17) % (dialogContent.width - 5))
                    y: -5
                    
                    property real horizontalDrift: ((index % 7) - 3) * 0.8
                    property real startX: ((index * 43 + index * 29 + index * 17) % (dialogContent.width - 5))
                    property real startDelay: (index * 67) % 2000 // Random delay up to 2 seconds
                    
                    SequentialAnimation on y {
                        running: recordingDialog.visible
                        loops: Animation.Infinite
                        PauseAnimation { duration: dialogSnowflakeLoader.startDelay }
                        NumberAnimation {
                            to: dialogContent.height + 5
                            duration: 2500 + (index % 6) * 600
                            easing.type: Easing.Linear
                        }
                        PropertyAction {
                            target: dialogSnowflakeLoader
                            property: "x"
                            value: ((index * 43 + index * 29 + index * 17 + (index % 11) * 19) % (dialogContent.width - 5))
                        }
                        PropertyAction {
                            target: dialogSnowflakeLoader
                            property: "y"
                            value: -5
                        }
                        PropertyAction {
                            target: dialogSnowflakeLoader
                            property: "startDelay"
                            value: (index * 67 + (index % 9) * 23) % 2000
                        }
                    }
                    
                    SequentialAnimation on x {
                        running: recordingDialog.visible
                        loops: Animation.Infinite
                        PauseAnimation { duration: dialogSnowflakeLoader.startDelay }
                        NumberAnimation {
                            to: dialogSnowflakeLoader.startX + dialogSnowflakeLoader.horizontalDrift * 25
                            duration: 2500 + (index % 6) * 600
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            to: dialogSnowflakeLoader.startX - dialogSnowflakeLoader.horizontalDrift * 25
                            duration: 2500 + (index % 6) * 600
                            easing.type: Easing.InOutSine
                        }
                    }
                }
            }


            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 20

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: {
                        var ready = controller && controller.recordingReady;
                        console.log("Text binding evaluated, recordingReady:", ready);
                        return ready ? qsTr("Запись началась") : qsTr("Сейчас начнется запись");
                    }
                    font.pixelSize: 20
                    font.bold: true
                    color: "#333333"
                }

                PrimaryButton {
                    id: stopButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: qsTr("Стоп")
                    focus: true
                    onClicked: {
                        if (controller) {
                            controller.stopCurrentRecording();
                        }
                    }
                    
                    // Also handle keyboard events on button level
                    Keys.onPressed: {
                        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                            event.accepted = true;
                            if (controller) {
                                controller.stopCurrentRecording();
                            }
                        }
                    }
                }
            }
        }
    }
}

