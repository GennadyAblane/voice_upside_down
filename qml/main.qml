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
        height: 860
        minimumHeight: 800
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
        
        // Кнопка настроек с иконкой
        Button {
            id: settingsButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 16
            width: 40
            height: 40
            background: Rectangle {
                color: settingsButton.pressed ? "#e0e0e0" : "transparent"
                radius: 4
            }
            
            Image {
                id: settingsIcon
                anchors.centerIn: parent
                width: 32
                height: 32
                source: "qrc:/1.png"
                fillMode: Image.PreserveAspectFit
            }
            
            onClicked: {
                // Переключить иконку на 800мс
                settingsIcon.source = "qrc:/2.png"
                iconSwitchTimer.start()
                // Открыть диалог настроек
                settingsDialog.visible = true
            }
            
            Timer {
                id: iconSwitchTimer
                interval: 800
                onTriggered: {
                    settingsIcon.source = "qrc:/1.png"
                }
            }
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
                        id: segmentNoiseControls
                        Layout.fillWidth: true
                        spacing: 8
                        
                        // Timer для принудительного обновления waveform при изменении порога
                        Timer {
                            id: segmentWaveformUpdateTimer
                            interval: 10
                            repeat: false
                            onTriggered: {
                                // Принудительно эмитируем сигнал volumeSettingsChanged
                                if (controller) {
                                    // Временно изменяем значение на достаточную величину, чтобы пройти проверку в C++
                                    var currentValue = controller.segmentNoiseThreshold
                                    var tempValue = currentValue < 0.5 ? currentValue + 0.001 : currentValue - 0.001
                                    controller.segmentNoiseThreshold = tempValue
                                    Qt.callLater(function() {
                                        controller.segmentNoiseThreshold = currentValue
                                    })
                                }
                            }
                        }
                        
                        function updateSegmentNoise(value, options) {
                            var clamped = Math.max(0.0001, Math.min(1.0, value || 0.0))
                            if (!(options && options.skipController) && controller) {
                                var oldValue = controller.segmentNoiseThreshold
                                controller.segmentNoiseThreshold = clamped
                                // Если изменение слишком мало (меньше 0.001), принудительно обновляем через Timer
                                if (Math.abs(oldValue - clamped) < 0.001 && Math.abs(oldValue - clamped) > 0.0000001) {
                                    segmentWaveformUpdateTimer.start()
                                }
                            }
                            var dB = 20 * Math.log10(clamped)
                            // Конвертируем дБ в линейную позицию бегунка (от 0.0 до 1.0)
                            // dB от -80 до 0, slider от 0.0 до 1.0
                            var sliderValue = (dB + 80) / 80
                            if (!(options && options.skipSlider)) {
                                segmentNoiseSlider.updatingFromInput = true
                                segmentNoiseSlider.value = Math.max(0.0, Math.min(1.0, sliderValue))
                                segmentNoiseSlider.updatingFromInput = false
                            }
                            if (!(options && options.skipSpinBox)) {
                                segmentNoiseDbInput.updatingFromSlider = true
                                segmentNoiseDbInput.value = Math.round(dB)  // Конвертируем в целое число дБ
                                segmentNoiseDbInput.updatingFromSlider = false
                            }
                        }
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

                    // Длина отрезка
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Label {
                            text: qsTr("Длина отрезка")
                            color: "#c41e3a"
                            font.pixelSize: 14
                            Layout.alignment: Qt.AlignVCenter
                        }
                        SpinBox {
                            id: segmentLengthSpin
                            Layout.preferredWidth: 120
                            from: 1
                            to: 10
                            enabled: controller && controller.canAdjustSegmentLength
                            value: controller ? controller.segmentLength : 5
                            onValueModified: controller ? controller.changeSegmentLength(value) : null
                            
                            // Уменьшаем размер кнопок +/-
                            up.indicator.implicitWidth: 20
                            down.indicator.implicitWidth: 20
                        }
                    }
                    
                    // Шум записи
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Label {
                            text: qsTr("Шум записи")
                            color: "#c41e3a"
                            font.pixelSize: 14
                            Layout.alignment: Qt.AlignVCenter
                        }
                        SpinBox {
                            id: segmentNoiseDbInput
                            Layout.preferredWidth: 120
                            from: -80
                            to: 0
                            stepSize: 1
                            editable: true
                            property bool updatingFromSlider: false
                            
                            // Уменьшаем размер кнопок +/-
                            up.indicator.implicitWidth: 20
                            down.indicator.implicitWidth: 20
                            
                            function dBToValue(dB) { return Math.pow(10, dB / 20) }
                            
                            // Отображать значение в дБ (value хранится напрямую, например -20 для -20 дБ)
                            property real dBValue: value
                            
                            // Функция для форматирования значения в текст
                            function formatValue(val) {
                                return val.toString()
                            }
                            
                            // Функция для парсинга текста в значение
                            function parseValue(text) {
                                var dB = parseInt(text)
                                if (isNaN(dB)) return -20
                                dB = Math.max(-80, Math.min(0, dB))
                                return dB
                            }
                            
                            // Кастомный contentItem для отображения значения в дБ
                            contentItem: TextInput {
                                text: segmentNoiseDbInput.formatValue(segmentNoiseDbInput.value)
                                font: segmentNoiseDbInput.font
                                color: segmentNoiseDbInput.palette.text
                                selectionColor: segmentNoiseDbInput.palette.highlight
                                selectedTextColor: segmentNoiseDbInput.palette.highlightedText
                                horizontalAlignment: Qt.AlignHCenter
                                verticalAlignment: Qt.AlignVCenter
                                readOnly: !segmentNoiseDbInput.editable
                                validator: IntValidator {
                                    bottom: -80
                                    top: 0
                                }
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                
                                onEditingFinished: {
                                    if (!segmentNoiseDbInput.updatingFromSlider) {
                                        var newValue = segmentNoiseDbInput.parseValue(text)
                                        if (newValue !== segmentNoiseDbInput.value) {
                                            segmentNoiseDbInput.value = newValue
                                        }
                                    }
                                }
                            }
                            
                            Component.onCompleted: {
                                if (controller) {
                                    segmentNoiseControls.updateSegmentNoise(controller.segmentNoiseThreshold, { skipController: true, skipSlider: false, skipSpinBox: false })
                                } else {
                                    value = -20  // -20 дБ
                                }
                            }
                            
                            Connections {
                                target: controller
                                function onVolumeSettingsChanged() {
                                    if (controller) {
                                        segmentNoiseControls.updateSegmentNoise(controller.segmentNoiseThreshold, { skipController: true })
                                    }
                                }
                            }
                            
                            onValueChanged: {
                                if (updatingFromSlider)
                                    return
                                var dB = dBValue
                                var linearValue = dBToValue(dB)
                                var clampedValue = Math.max(0.0001, Math.min(1.0, linearValue))
                                // Конвертируем dB в позицию бегунка (0.0-1.0 соответствует -80 до 0 dB)
                                var sliderPos = (dB + 80) / 80
                                console.log("SegmentNoise SpinBox changed: dB =", dB, "linear value =", clampedValue, "slider position =", sliderPos)
                                segmentNoiseSlider.updatingFromInput = true
                                segmentNoiseSlider.value = Math.max(0.0, Math.min(1.0, sliderPos))
                                segmentNoiseSlider.updatingFromInput = false
                                segmentNoiseControls.updateSegmentNoise(clampedValue, { skipSpinBox: true })
                            }
                        }
                        
                        Label {
                            text: qsTr("dB")
                            color: "#c41e3a"
                            font.pixelSize: 14
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                    
                    // Бегунок для шума записи
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Label {
                            text: qsTr("min")
                            Layout.preferredWidth: 30
                            color: "#c41e3a"
                            font.pixelSize: 12
                        }
                        
                        Slider {
                            id: segmentNoiseSlider
                            Layout.fillWidth: true
                            from: 0.0
                            to: 1.0
                            stepSize: 0.001
                            property bool updatingFromInput: false
                            // Инициализация: конвертируем линейное значение в позицию бегунка
                            Component.onCompleted: {
                                if (controller) {
                                    var linearValue = controller.segmentNoiseThreshold
                                    var dB = 20 * Math.log10(linearValue)
                                    var sliderPos = (dB + 80) / 80
                                    value = Math.max(0.0, Math.min(1.0, sliderPos))
                                } else {
                                    value = 0.75  // Примерно -20 дБ
                                }
                            }
                            onValueChanged: {
                                if (updatingFromInput)
                                    return
                                // Конвертируем позицию бегунка (0.0-1.0) в дБ (-80 до 0), затем в линейное значение
                                var dB = -80 + (value * 80)
                                var linearValue = Math.pow(10, dB / 20)
                                console.log("SegmentNoise Slider changed: slider position =", value, "dB =", dB, "linear value =", linearValue)
                                segmentNoiseControls.updateSegmentNoise(linearValue, { skipSlider: true })
                            }
                        }
                        
                        Label {
                            text: qsTr("max")
                            Layout.preferredWidth: 30
                            color: "#c41e3a"
                            font.pixelSize: 12
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
                Layout.minimumHeight: 520
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
                        segmentIndex: model.segmentIndex
                        segmentController: controller

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
    
    // Диалоговое окно настроек
    Item {
        id: settingsDialog
        anchors.fill: parent
        visible: false
        z: 10001 // Above recording dialog
        
        // Данные для waveform (обновляются при изменении порога или загрузке аудио)
        property var waveformVolumeData: []
        property var waveformSegments: []
        property var editedBoundaries: []  // Временные границы, редактируемые пользователем
        
        // Обновить данные waveform при открытии диалога или изменении порога
        onVisibleChanged: {
            if (visible && controller && controller.projectReady) {
                updateWaveformData()
                // Инициализировать editedBoundaries текущими границами
                editedBoundaries = []
                var segData = controller.getSegmentDataForWaveform()
                for (var i = 0; i < segData.length; i++) {
                    editedBoundaries.push(segData[i].startMs)
                }
                // Добавить конечную границу
                if (segData.length > 0) {
                    var lastSeg = segData[segData.length - 1]
                    if (lastSeg.endMs) {
                        editedBoundaries.push(lastSeg.endMs)
                    }
                }
            } else if (!visible) {
                // При закрытии применяем изменения
                applyBoundaryChanges()
            }
        }
        
        function applyBoundaryChanges() {
            if (!controller || !controller.projectReady) return
            
            // Собрать все границы из editableSegments waveformView
            var boundaries = []
            if (waveformView.editableSegments && waveformView.editableSegments.length > 0) {
                for (var i = 0; i < waveformView.editableSegments.length; i++) {
                    boundaries.push(waveformView.editableSegments[i].startMs)
                }
                // Добавить конечную границу
                var lastSeg = waveformView.editableSegments[waveformView.editableSegments.length - 1]
                if (lastSeg.endMs) {
                    boundaries.push(lastSeg.endMs)
                }
            } else {
                // Если не было редактирования, использовать текущие границы
                boundaries = editedBoundaries
            }
            
            if (boundaries.length > 0) {
                controller.recreateSegmentsFromBoundaries(boundaries)
            }
        }
        
        function updateWaveformData() {
            if (!controller || !controller.projectReady) {
                settingsDialog.waveformVolumeData = []
                settingsDialog.waveformSegments = []
                waveformView._segmentsVersion++
                return
            }
            
            // Получить данные анализа громкости
            var volumeAnalysis = controller.analyzeVolume(50, originalNoiseSlider.value, 0.7)
            settingsDialog.waveformVolumeData = volumeAnalysis
            
            // Получить данные отрезков
            var segData = controller.getSegmentDataForWaveform()
            settingsDialog.waveformSegments = segData
            waveformView._segmentsVersion++
            
            // Обновить editedBoundaries при обновлении данных
            if (settingsDialog.visible) {
                editedBoundaries = []
                for (var i = 0; i < segData.length; i++) {
                    editedBoundaries.push(segData[i].startMs)
                }
                // Добавить конечную границу
                if (segData.length > 0) {
                    var lastSeg = segData[segData.length - 1]
                    if (lastSeg.endMs) {
                        editedBoundaries.push(lastSeg.endMs)
                    }
                }
            }
        }
        
        // Semi-transparent black overlay
        Rectangle {
            anchors.fill: parent
            color: "#80000000"
        }
        
        // Block all mouse interactions with background
        MouseArea {
            anchors.fill: parent
            enabled: settingsDialog.visible
            onClicked: {
                // Close dialog when clicking on overlay
                settingsDialog.visible = false
            }
        }
        
        Rectangle {
            id: settingsDialogContent
            anchors.centerIn: parent
            width: 700
            height: 600
            color: Theme.Theme.colors.sectionBackground
            radius: Theme.Theme.cornerRadius
            
            layer.enabled: true
            layer.effect: DropShadow {
                color: "#40000000"
                radius: 20
                samples: 32
                verticalOffset: 4
            }
            
            // Prevent clicks inside dialog from closing it
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    // Do nothing - just block clicks from reaching overlay
                }
            }
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 20
                
                // Waveform визуализация (показывается только если есть загруженное/записанное аудио)
                WaveformView {
                    id: waveformView
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    visible: controller && controller.projectReady
                    volumeData: settingsDialog.waveformVolumeData
                    noiseThreshold: originalNoiseSlider.value
                    showSegments: true
                    segments: settingsDialog.waveformSegments
                    interactive: true
                    
                    // Обработка изменений границ
                    onSegmentBoundaryChanged: {
                        // Границы обновляются в реальном времени в editableSegments
                        // Применение произойдет при закрытии диалога
                    }
                }
                
                // Регулятор для оригинала
                ColumnLayout {
                    id: originalNoiseControls
                    Layout.fillWidth: true
                    spacing: 8
                    
                    function updateOriginalNoise(value, options) {
                        var clamped = Math.max(0.0001, Math.min(1.0, value || 0.0))
                        if (!(options && options.skipController) && controller) {
                            if (controller.originalNoiseThreshold !== clamped) {
                                controller.originalNoiseThreshold = clamped
                            }
                        }
                        var dB = 20 * Math.log10(clamped)
                        // Конвертируем дБ в линейную позицию бегунка (от 0.0 до 1.0)
                        // dB от -80 до 0, slider от 0.0 до 1.0
                        var sliderValue = (dB + 80) / 80
                        if (!(options && options.skipSlider)) {
                            originalNoiseSlider.updatingFromInput = true
                            originalNoiseSlider.value = Math.max(0.0, Math.min(1.0, sliderValue))
                            originalNoiseSlider.updatingFromInput = false
                        }
                        if (!(options && options.skipSpinBox)) {
                            originalNoiseDbInput.updatingFromSlider = true
                            originalNoiseDbInput.value = Math.round(dB)  // Конвертируем в целое число дБ
                            originalNoiseDbInput.updatingFromSlider = false
                        }
                        if (settingsDialog.visible && !(options && options.skipWaveformUpdate)) {
                            settingsDialog.updateWaveformData()
                        }
                    }
                    
                    // Строка с меткой и SpinBox
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        
                        Label {
                            text: qsTr("Шум оригинал")
                            color: "#c41e3a"
                            font.pixelSize: 16
                            font.bold: true
                            Layout.alignment: Qt.AlignVCenter
                        }
                        
                        // SpinBox для ввода в ДБ
                        SpinBox {
                            id: originalNoiseDbInput
                            Layout.preferredWidth: 120
                            from: -80
                            to: 0
                            stepSize: 1
                            editable: true
                            property bool updatingFromSlider: false
                            
                            // Уменьшаем размер кнопок +/-
                            up.indicator.implicitWidth: 20
                            down.indicator.implicitWidth: 20
                            
                            // Отображать значение в дБ (value хранится напрямую, например -20 для -20 дБ)
                            property real dBValue: value
                            
                            // Функция конвертации ДБ в значение
                            function dBToValue(dB) {
                                return Math.pow(10, dB / 20)
                            }
                            
                            // Функция для форматирования значения в текст
                            function formatValue(val) {
                                return val.toString()
                            }
                            
                            // Функция для парсинга текста в значение
                            function parseValue(text) {
                                var dB = parseInt(text)
                                if (isNaN(dB)) return -20
                                dB = Math.max(-80, Math.min(0, dB))
                                return dB
                            }
                            
                            // Кастомный contentItem для отображения значения в дБ
                            contentItem: TextInput {
                                text: originalNoiseDbInput.formatValue(originalNoiseDbInput.value)
                                font: originalNoiseDbInput.font
                                color: originalNoiseDbInput.palette.text
                                selectionColor: originalNoiseDbInput.palette.highlight
                                selectedTextColor: originalNoiseDbInput.palette.highlightedText
                                horizontalAlignment: Qt.AlignHCenter
                                verticalAlignment: Qt.AlignVCenter
                                readOnly: !originalNoiseDbInput.editable
                                validator: IntValidator {
                                    bottom: -80
                                    top: 0
                                }
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                
                                onEditingFinished: {
                                    if (!originalNoiseDbInput.updatingFromSlider) {
                                        var newValue = originalNoiseDbInput.parseValue(text)
                                        if (newValue !== originalNoiseDbInput.value) {
                                            originalNoiseDbInput.value = newValue
                                        }
                                    }
                                }
                            }
                            
                            // Инициализация значения
                            Component.onCompleted: {
                                if (controller) {
                                    originalNoiseControls.updateOriginalNoise(controller.originalNoiseThreshold, { skipController: true })
                                } else {
                                    value = -20  // -20 дБ
                                }
                            }
                            
                            // Обновление при изменении контроллера
                            Connections {
                                target: controller
                                function onVolumeSettingsChanged() {
                                    if (controller) {
                                        originalNoiseControls.updateOriginalNoise(controller.originalNoiseThreshold, { skipController: true })
                                    }
                                }
                            }
                            
                            onValueChanged: {
                                if (updatingFromSlider)
                                    return
                                var dB = dBValue
                                var linearValue = dBToValue(dB)
                                var clampedValue = Math.max(0.0001, Math.min(1.0, linearValue))
                                // Конвертируем dB в позицию бегунка (0.0-1.0 соответствует -80 до 0 dB)
                                var sliderPos = (dB + 80) / 80
                                console.log("OriginalNoise SpinBox changed: dB =", dB, "linear value =", clampedValue, "slider position =", sliderPos)
                                originalNoiseSlider.updatingFromInput = true
                                originalNoiseSlider.value = Math.max(0.0, Math.min(1.0, sliderPos))
                                originalNoiseSlider.updatingFromInput = false
                                originalNoiseControls.updateOriginalNoise(clampedValue, { skipSpinBox: true })
                                // Обновляем waveform напрямую для высокой чувствительности
                                if (settingsDialog.visible && controller && controller.projectReady) {
                                    settingsDialog.updateWaveformData()
                                }
                            }
                        }
                        
                        Label {
                            text: qsTr("dB")
                            color: "#c41e3a"
                            font.pixelSize: 16
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                    
                    // Бегунок без рамки (работает линейно по дБ от -80 до 0)
                    Slider {
                        id: originalNoiseSlider
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        stepSize: 0.001
                        property bool updatingFromInput: false
                        // Инициализация: конвертируем линейное значение в позицию бегунка
                        Component.onCompleted: {
                            if (controller) {
                                var linearValue = controller.originalNoiseThreshold
                                var dB = 20 * Math.log10(linearValue)
                                var sliderPos = (dB + 80) / 80
                                value = Math.max(0.0, Math.min(1.0, sliderPos))
                            } else {
                                value = 0.75  // Примерно -20 дБ
                            }
                        }
                        onValueChanged: {
                            if (updatingFromInput)
                                return
                            // Конвертируем позицию бегунка (0.0-1.0) в дБ (-80 до 0), затем в линейное значение
                            var dB = -80 + (value * 80)
                            var linearValue = Math.pow(10, dB / 20)
                            console.log("OriginalNoise Slider changed: slider position =", value, "dB =", dB, "linear value =", linearValue)
                            originalNoiseControls.updateOriginalNoise(linearValue, { skipSlider: true })
                            // Обновляем waveform напрямую для высокой чувствительности
                            if (settingsDialog.visible && controller && controller.projectReady) {
                                settingsDialog.updateWaveformData()
                            }
                        }
                    }
                }
                
                Item { Layout.fillHeight: true }
                
                // Кнопка закрытия
                PrimaryButton {
                    Layout.fillWidth: true
                    text: qsTr("Закрыть")
                    onClicked: settingsDialog.visible = false
                }
            }
        }
        
        // Заголовок диалога
        Rectangle {
            anchors.left: settingsDialogContent.left
            anchors.right: settingsDialogContent.right
            anchors.bottom: settingsDialogContent.top
            anchors.bottomMargin: -16
            height: 40
            color: Theme.Theme.colors.sectionBackground
            radius: Theme.Theme.cornerRadius
            border.color: Theme.Theme.colors.shadowColor
            border.width: 1
            
            Label {
                anchors.centerIn: parent
                text: qsTr("Настройки")
                font.pixelSize: 18
                font.bold: true
                color: "#c41e3a"
            }
        }
    }
}
