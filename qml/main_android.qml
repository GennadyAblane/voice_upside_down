import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import QtGraphicalEffects 1.15
import "components"
import "theme" as Theme

ApplicationWindow {
    id: window
    // Для Android используем полный экран
    width: Qt.platform.os === "android" ? Screen.width : 1280
    height: Qt.platform.os === "android" ? Screen.height : 760
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
        z: 1000
        clip: false

        Repeater {
            model: 50
            Rectangle {
                id: sparkle
                property real sparkleSize: (index % 5) * 0.8 + 2
                property real sparkleX: (index * 37) % (window.width - sparkleSize)
                property real sparkleY: (index * 73) % (window.height - sparkleSize)
                property real sparkleDelay: (index * 47) % 2000
                property real sparkleDuration: 500 + ((index * 31) % 1000)
                property real sparkleMaxOpacity: 0.7 + ((index * 19) % 30) / 100.0

                width: sparkleSize
                height: sparkleSize
                radius: sparkleSize / 2
                color: "#FFFFFF"
                x: sparkleX
                y: sparkleY

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
        height: Qt.platform.os === "android" ? 56 : 72
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
            font.pixelSize: Qt.platform.os === "android" ? 20 : 28
            font.bold: true
            color: "#c41e3a"
        }
    }

    footer: Rectangle {
        height: Qt.platform.os === "android" ? 48 : 36
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
            anchors.leftMargin: Qt.platform.os === "android" ? 12 : 24
            anchors.right: parent.right
            anchors.rightMargin: Qt.platform.os === "android" ? 12 : 24
            text: controller ? controller.statusMessage : qsTr("Готово")
            color: "#c41e3a"
            font.pixelSize: Qt.platform.os === "android" ? Theme.Theme.captionSize : Theme.Theme.baseFontSize
            elide: Text.ElideRight
        }
    }

    // Вертикальный layout для Android
    ColumnLayout {
        anchors {
            top: parent.top
            topMargin: header.height
            bottom: parent.bottom
            bottomMargin: footer.height
            left: parent.left
            right: parent.right
            margins: Qt.platform.os === "android" ? 12 : 24
        }
        spacing: Qt.platform.os === "android" ? 12 : 24

        // Панель управления (компактная для мобильного)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Qt.platform.os === "android" ? 280 : 200
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
                anchors.margins: Qt.platform.os === "android" ? 12 : 16
                spacing: Qt.platform.os === "android" ? 10 : 16

                // Кнопки проектов
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Qt.platform.os === "android" ? 8 : 12
                    PrimaryButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Qt.platform.os === "android" ? 50 : 60
                        text: qsTr("Открыть проект")
                        secondary: true
                        onClicked: openProjectDialog.open()
                    }
                    PrimaryButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Qt.platform.os === "android" ? 50 : 60
                        text: qsTr("Сохранить проект")
                        secondary: true
                        enabled: controller && controller.projectReady
                        onClicked: saveProjectDialog.open()
                    }
                }

                // Кнопки загрузки
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Qt.platform.os === "android" ? 8 : 12
                    PrimaryButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Qt.platform.os === "android" ? 50 : 60
                        text: qsTr("Загрузить файл")
                        onClicked: openFileDialog.open()
                    }
                    PrimaryButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Qt.platform.os === "android" ? 50 : 60
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

                // Имя файла
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: controller ? controller.currentSourceName : qsTr("Песня не выбрана")
                    color: "#c41e3a"
                    font.pixelSize: Qt.platform.os === "android" ? Theme.Theme.captionSize : Theme.Theme.baseFontSize
                    elide: Text.ElideRight
                    maximumLineCount: 2
                }

                // Длина отрезка
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Qt.platform.os === "android" ? 8 : 12
                    Label {
                        text: qsTr("Длина отрезка")
                        color: "#c41e3a"
                        font.pixelSize: Qt.platform.os === "android" ? Theme.Theme.captionSize : Theme.Theme.baseFontSize
                        Layout.alignment: Qt.AlignVCenter
                    }
                    SpinBox {
                        id: segmentLengthSpin
                        Layout.preferredWidth: Qt.platform.os === "android" ? 80 : 100
                        Layout.preferredHeight: Qt.platform.os === "android" ? 40 : 50
                        from: 1
                        to: 10
                        enabled: controller && controller.canAdjustSegmentLength
                        value: controller ? controller.segmentLength : 5
                        onValueModified: controller ? controller.changeSegmentLength(value) : null
                    }
                    Item { Layout.fillWidth: true }
                }
            }
        }

        // Список отрезков (занимает оставшееся место)
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

            ListView {
                id: segmentList
                anchors.fill: parent
                anchors.margins: Qt.platform.os === "android" ? 12 : 16
                spacing: Qt.platform.os === "android" ? 8 : 12
                clip: true
                model: controller ? controller.segmentModel : null
                
                // Save scroll position to restore after model updates
                property real savedContentY: 0
                property bool restoringPosition: false
                property bool userScrolling: false
                
                onContentYChanged: {
                    if (!restoringPosition && contentY >= 0) {
                        if (userScrolling || Math.abs(contentY - savedContentY) > 5) {
                            savedContentY = contentY
                        }
                    }
                }
                
                onMovementStarted: {
                    userScrolling = true
                }
                
                onMovementEnded: {
                    userScrolling = false
                    if (contentY >= 0) {
                        savedContentY = contentY
                    }
                }
                
                Component.onCompleted: {
                    if (contentY >= 0) {
                        savedContentY = contentY
                    }
                }
                
                onCountChanged: {
                    if (savedContentY > 0 && !restoringPosition && count > 0) {
                        restoreScrollPosition()
                    }
                }
                
                onModelChanged: {
                    if (savedContentY > 0 && !restoringPosition && model && model.count > 0) {
                        restoreScrollPosition()
                    }
                }
                
                function restoreScrollPosition() {
                    if (restoringPosition)
                        return
                        
                    restoringPosition = true
                    var attempts = 0
                    function tryRestore() {
                        attempts++
                        if (segmentList.contentHeight > 0) {
                            if (segmentList.contentHeight >= segmentList.savedContentY) {
                                segmentList.contentY = segmentList.savedContentY
                                segmentList.restoringPosition = false
                            } else if (attempts < 3) {
                                Qt.callLater(tryRestore)
                            } else {
                                segmentList.contentY = Math.max(0, segmentList.contentHeight - segmentList.height)
                                segmentList.restoringPosition = false
                            }
                        } else if (attempts < 5) {
                            Qt.callLater(tryRestore)
                        } else {
                            segmentList.restoringPosition = false
                        }
                    }
                    Qt.callLater(tryRestore)
                }
                
                delegate: SegmentRow {
                    width: segmentList.width - (Qt.platform.os === "android" ? 24 : 32)
                    title: model.title
                    durationText: model.durationText
                    recorded: model.recorded
                    reversed: model.reversed
                    recordingActive: controller ? controller.isSegmentRecording(model.segmentIndex) : false
                    originalPlaying: controller ? controller.isSegmentOriginalPlaying(model.segmentIndex) : false
                    recordingPlaying: controller ? controller.isSegmentRecordingPlaying(model.segmentIndex) : false
                    reversePlaying: controller ? controller.isSegmentReversePlaying(model.segmentIndex) : false
                    enabled: controller ? controller.interactionsEnabled : false

                    onRecordTriggered: controller ? controller.toggleSegmentRecording(model.segmentIndex) : null
                    onOriginalPlayTriggered: controller ? controller.toggleSegmentOriginalPlayback(model.segmentIndex) : null
                    onRecordingPlayTriggered: controller ? controller.toggleSegmentRecordedPlayback(model.segmentIndex) : null
                    onReversePlayTriggered: controller ? controller.toggleSegmentReversePlayback(model.segmentIndex) : null
                }
                ScrollBar.vertical: ScrollBar { 
                    policy: ScrollBar.AsNeeded
                }
            }
        }

        // Нижняя панель действий (горизонтальная прокрутка для мобильного)
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Qt.platform.os === "android" ? 60 : 50
            clip: true
            
            RowLayout {
                width: Math.max(parent.width, implicitWidth)
                spacing: Qt.platform.os === "android" ? 8 : 12

                PrimaryButton {
                    Layout.preferredWidth: Qt.platform.os === "android" ? 140 : 180
                    Layout.preferredHeight: Qt.platform.os === "android" ? 50 : 60
                    text: qsTr("Склеить отрезки")
                    enabled: controller ? controller.canGlue : false
                    onClicked: controller ? controller.glueSegments() : null
                }
                PrimaryButton {
                    Layout.preferredWidth: Qt.platform.os === "android" ? 140 : 200
                    Layout.preferredHeight: Qt.platform.os === "android" ? 50 : 60
                    text: controller && controller.gluePlaybackActive ? qsTr("Стоп склейку") : qsTr("Воспроизвести склейку")
                    secondary: true
                    enabled: controller ? controller.canPlayGlue : false
                    onClicked: controller ? controller.toggleGluePlayback() : null
                }
                PrimaryButton {
                    Layout.preferredWidth: Qt.platform.os === "android" ? 140 : 200
                    Layout.preferredHeight: Qt.platform.os === "android" ? 50 : 60
                    text: qsTr("Перевернуть песню")
                    enabled: controller ? controller.canReverseSong : false
                    onClicked: controller ? controller.reverseSong() : null
                }
                PrimaryButton {
                    Layout.preferredWidth: Qt.platform.os === "android" ? 140 : 200
                    Layout.preferredHeight: Qt.platform.os === "android" ? 50 : 60
                    text: controller && controller.reversePlaybackActive ? qsTr("Стоп реверс") : qsTr("Воспроизвести реверс")
                    secondary: true
                    enabled: controller ? controller.canPlayReverse : false
                    onClicked: controller ? controller.toggleSongReversePlayback() : null
                }
            }
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
        id: saveProjectDialog
        title: qsTr("Сохранить проект")
        selectExisting: false
        selectFolder: true
        onAccepted: controller ? controller.saveProjectTo(fileUrl.toLocalFile()) : null
    }

    FileDialog {
        id: openProjectDialog
        title: qsTr("Открыть проект")
        nameFilters: ["Voice Upside Down Project (*.vups)", "All files (*)"]
        onAccepted: controller ? controller.openProjectFrom(fileUrl.toLocalFile()) : null
    }
}

