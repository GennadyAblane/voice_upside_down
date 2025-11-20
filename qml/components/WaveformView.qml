import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme" as Theme

Item {
    id: root
    property var volumeData: []  // Массив объектов с {startMs, endMs, rmsLevel, isQuiet, isLoud}
    property double noiseThreshold: 0.1  // Порог шума
    property bool showSegments: false  // Показывать ли границы отрезков
    property var segments: []  // Массив отрезков для отображения
    property bool interactive: true  // Можно ли взаимодействовать с waveform
    property double playbackPositionMs: -1  // Позиция воспроизведения в миллисекундах (-1 если не воспроизводится)
    property double segmentStartMs: 0  // Начало сегмента в миллисекундах (для расчета относительной позиции)
    property bool showTrimBoundaries: false  // Показывать ли линии обрезки
    property double trimStartMs: -1.0  // Начало обрезки в миллисекундах (-1 если не установлено)
    property double trimEndMs: -1.0  // Конец обрезки в миллисекундах (-1 если не установлено)
    
    // Helper property to trigger repaint when segments change
    property int _segmentsVersion: 0
    
    signal segmentBoundaryChanged(int segmentIndex, double startMs, double endMs)
    signal trimBoundaryChanged(double trimStartMs, double trimEndMs)
    
    implicitHeight: 200
    
    Rectangle {
        anchors.fill: parent
        color: Theme.Theme.colors.sectionBackground
        radius: Theme.Theme.cornerRadius
        border.color: Theme.Theme.colors.shadowColor
        border.width: 1
        
        // Waveform visualization
        Canvas {
            id: waveformCanvas
            anchors.fill: parent
            anchors.margins: 8
            
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                
                if (!volumeData || volumeData.length === 0)
                    return
                
                var maxDuration = 0
                for (var i = 0; i < volumeData.length; i++) {
                    if (volumeData[i].endMs > maxDuration) {
                        maxDuration = volumeData[i].endMs
                    }
                }
                
                if (maxDuration === 0)
                    return
                
                var scaleX = width / maxDuration
                var centerY = height / 2
                var maxAmplitude = height / 2 - 10
                
                // Draw noise threshold line
                ctx.strokeStyle = "#ff6b6b"
                ctx.lineWidth = 1
                ctx.setLineDash([5, 5])
                var thresholdY = centerY - (noiseThreshold * maxAmplitude)
                ctx.beginPath()
                ctx.moveTo(0, thresholdY)
                ctx.lineTo(width, thresholdY)
                ctx.stroke()
                ctx.setLineDash([])
                
                // Draw waveform as bars (simpler and more efficient)
                ctx.fillStyle = "#c41e3a"
                for (var i = 0; i < volumeData.length; i++) {
                    var level = volumeData[i]
                    if (!level) continue
                    var x = level.startMs * scaleX
                    var nextX = i < volumeData.length - 1 ? (volumeData[i + 1] ? volumeData[i + 1].startMs * scaleX : width) : width
                    var w = Math.max(1, nextX - x)
                    var amplitude = (level.rmsLevel || 0) * maxAmplitude
                    var barHeight = Math.max(1, amplitude * 2)
                    var y = centerY - amplitude
                    
                    ctx.fillRect(x, y, w, barHeight)
                }
                
                // Draw quiet/loud regions
                for (var i = 0; i < volumeData.length; i++) {
                    var level = volumeData[i]
                    var x = level.startMs * scaleX
                    var nextX = i < volumeData.length - 1 ? volumeData[i + 1].startMs * scaleX : width
                    var w = nextX - x
                    
                    if (level.isQuiet) {
                        ctx.fillStyle = "#ff6b6b40"  // Красный с прозрачностью для тишины
                        ctx.fillRect(x, 0, w, height)
                    }
                }
                
                // Draw segment boundaries if enabled
                if (showSegments) {
                    var segsToDraw = root.interactive && editableSegments.length > 0 ? editableSegments : segments
                    if (segsToDraw) {
                        ctx.strokeStyle = "#4ecdc4"
                        ctx.lineWidth = 2
                        ctx.setLineDash([5, 5])  // Dashed lines
                        for (var i = 0; i < segsToDraw.length; i++) {
                            var seg = segsToDraw[i]
                            var x = seg.startMs * scaleX
                            ctx.beginPath()
                            ctx.moveTo(x, 0)
                            ctx.lineTo(x, height)
                            ctx.stroke()
                            
                            // Draw end boundary
                            if (seg.endMs) {
                                var endX = seg.endMs * scaleX
                                ctx.beginPath()
                                ctx.moveTo(endX, 0)
                                ctx.lineTo(endX, height)
                                ctx.stroke()
                            }
                        }
                        ctx.setLineDash([])
                    }
                }
                
                // Draw trim boundaries if enabled
                // Use editable values if they are set (>= 0), otherwise use property values
                var currentTrimStartMs = editableTrimStartMs >= 0 ? editableTrimStartMs : trimStartMs
                var currentTrimEndMs = editableTrimEndMs >= 0 ? editableTrimEndMs : trimEndMs
                
                if (showTrimBoundaries && currentTrimStartMs >= 0 && currentTrimEndMs >= 0) {
                    ctx.strokeStyle = "#9b59b6"  // Фиолетовый цвет для линий обрезки
                    ctx.lineWidth = 2
                    ctx.setLineDash([8, 4])  // Dashed lines
                    
                    // Draw start trim line
                    var trimStartX = currentTrimStartMs * scaleX
                    if (trimStartX >= 0 && trimStartX <= width) {
                        ctx.beginPath()
                        ctx.moveTo(trimStartX, 0)
                        ctx.lineTo(trimStartX, height)
                        ctx.stroke()
                    }
                    
                    // Draw end trim line
                    var trimEndX = currentTrimEndMs * scaleX
                    if (trimEndX >= 0 && trimEndX <= width) {
                        ctx.beginPath()
                        ctx.moveTo(trimEndX, 0)
                        ctx.lineTo(trimEndX, height)
                        ctx.stroke()
                    }
                    
                    ctx.setLineDash([])
                }
                
                // Draw playback position indicator
                if (playbackPositionMs >= 0) {
                    // Calculate absolute position: segmentStartMs + playbackPositionMs
                    var absolutePositionMs = segmentStartMs + playbackPositionMs
                    var positionX = absolutePositionMs * scaleX
                    
                    if (positionX >= 0 && positionX <= width) {
                        ctx.strokeStyle = "#ffd700"  // Золотой цвет для индикатора
                        ctx.lineWidth = 3
                        ctx.beginPath()
                        ctx.moveTo(positionX, 0)
                        ctx.lineTo(positionX, height)
                        ctx.stroke()
                        
                        // Draw a small triangle at the top
                        ctx.fillStyle = "#ffd700"
                        ctx.beginPath()
                        ctx.moveTo(positionX, 0)
                        ctx.lineTo(positionX - 6, 10)
                        ctx.lineTo(positionX + 6, 10)
                        ctx.closePath()
                        ctx.fill()
                    }
                }
            }
            
            Connections {
                target: root
                function onVolumeDataChanged() { 
                    console.log("WaveformView: volumeData changed, length:", root.volumeData ? root.volumeData.length : 0)
                    waveformCanvas.requestPaint() 
                }
                function onNoiseThresholdChanged() { waveformCanvas.requestPaint() }
                function onShowSegmentsChanged() { waveformCanvas.requestPaint() }
                function on_SegmentsVersionChanged() { waveformCanvas.requestPaint() }
                function onPlaybackPositionMsChanged() { waveformCanvas.requestPaint() }
                function onSegmentStartMsChanged() { waveformCanvas.requestPaint() }
                function onEditableSegmentsChanged() { waveformCanvas.requestPaint() }
                function onShowTrimBoundariesChanged() { waveformCanvas.requestPaint() }
                function onTrimStartMsChanged() { waveformCanvas.requestPaint() }
                function onTrimEndMsChanged() { waveformCanvas.requestPaint() }
                function onEditableTrimStartMsChanged() { waveformCanvas.requestPaint() }
                function onEditableTrimEndMsChanged() { waveformCanvas.requestPaint() }
                function onDraggedTrimBoundaryChanged() { waveformCanvas.requestPaint() }
            }
            
            Component.onCompleted: {
                console.log("WaveformView: Component.onCompleted, volumeData length:", root.volumeData ? root.volumeData.length : 0)
                requestPaint()
            }
        }
    }
    
    // Interactive area for adjusting boundaries
    property var editableSegments: []  // Editable copy of segments
    property int draggedSegmentIndex: -1  // Index of segment being dragged
    property double dragStartX: 0
    property double dragStartMs: 0
    
    // Trim boundary editing
    property int draggedTrimBoundary: -1  // -1 = none, 0 = start, 1 = end
    property double editableTrimStartMs: -1.0
    property double editableTrimEndMs: -1.0
    
    // Initialize editable segments when segments change
    onSegmentsChanged: {
        _segmentsVersion++
        if (root.interactive && root.showSegments) {
            editableSegments = []
            if (segments) {
                for (var i = 0; i < segments.length; i++) {
                    editableSegments.push({
                        index: segments[i].index,
                        startMs: segments[i].startMs,
                        endMs: segments[i].endMs
                    })
                }
            }
        }
    }
    
    onTrimStartMsChanged: {
        editableTrimStartMs = trimStartMs
    }
    onTrimEndMsChanged: {
        editableTrimEndMs = trimEndMs
    }
    
    MouseArea {
        anchors.fill: parent
        enabled: root.interactive && (root.showSegments || root.showTrimBoundaries)
        hoverEnabled: true
        property double scaleX: {
            if (!volumeData || volumeData.length === 0) return 1
            var maxDuration = 0
            for (var i = 0; i < volumeData.length; i++) {
                if (volumeData[i].endMs > maxDuration) {
                    maxDuration = volumeData[i].endMs
                }
            }
            return maxDuration > 0 ? width / maxDuration : 1
        }
        
        function findSegmentAtX(x) {
            if (!editableSegments || editableSegments.length === 0) return -1
            var ms = x / scaleX
            var threshold = 10 / scaleX  // 10ms threshold for clicking
            
            // Check start boundaries
            for (var i = 0; i < editableSegments.length; i++) {
                var seg = editableSegments[i]
                if (Math.abs(ms - seg.startMs) < threshold) {
                    return i  // Return segment index
                }
            }
            
            // Check end boundaries
            for (var i = 0; i < editableSegments.length; i++) {
                var seg = editableSegments[i]
                if (seg.endMs && Math.abs(ms - seg.endMs) < threshold) {
                    return i  // Return segment index
                }
            }
            
            return -1
        }
        
        function findTrimBoundaryAtX(x) {
            if (!root.showTrimBoundaries || editableTrimStartMs < 0 || editableTrimEndMs < 0) return -1
            var ms = x / scaleX
            var threshold = 10 / scaleX  // 10ms threshold for clicking
            
            // Check start trim boundary
            if (Math.abs(ms - editableTrimStartMs) < threshold) {
                return 0  // Start boundary
            }
            
            // Check end trim boundary
            if (Math.abs(ms - editableTrimEndMs) < threshold) {
                return 1  // End boundary
            }
            
            return -1
        }
        
        onPressed: {
            if (!root.interactive) return
            
            // First check for trim boundaries
            if (root.showTrimBoundaries) {
                var trimBoundary = findTrimBoundaryAtX(mouseX)
                if (trimBoundary >= 0) {
                    draggedTrimBoundary = trimBoundary
                    dragStartX = mouseX
                    if (trimBoundary === 0) {
                        dragStartMs = editableTrimStartMs
                    } else {
                        dragStartMs = editableTrimEndMs
                    }
                    return
                }
            }
            
            // Then check for segment boundaries
            if (root.showSegments) {
                var segIndex = findSegmentAtX(mouseX)
                if (segIndex >= 0) {
                    draggedSegmentIndex = segIndex
                    dragStartX = mouseX
                    var seg = editableSegments[segIndex]
                    // Determine if dragging start or end
                    var ms = mouseX / scaleX
                    if (Math.abs(ms - seg.startMs) < Math.abs(ms - (seg.endMs || 0))) {
                        dragStartMs = seg.startMs
                    } else {
                        dragStartMs = seg.endMs || 0
                    }
                }
            }
        }
        
        onPositionChanged: {
            if (pressed) {
                var deltaX = mouseX - dragStartX
                var deltaMs = deltaX / scaleX
                var newMs = dragStartMs + deltaMs
                
                // Clamp to valid range
                var maxDuration = 0
                if (volumeData && volumeData.length > 0) {
                    for (var i = 0; i < volumeData.length; i++) {
                        if (volumeData[i].endMs > maxDuration) {
                            maxDuration = volumeData[i].endMs
                        }
                    }
                }
                newMs = Math.max(0, Math.min(maxDuration, newMs))
                
                // Handle trim boundary dragging
                if (draggedTrimBoundary >= 0) {
                    if (draggedTrimBoundary === 0) {
                        // Dragging start trim boundary
                        editableTrimStartMs = newMs
                        // Ensure start >= 0 and start < end
                        if (editableTrimStartMs < 0) editableTrimStartMs = 0
                        if (editableTrimEndMs >= 0 && editableTrimStartMs >= editableTrimEndMs) {
                            editableTrimStartMs = Math.max(0, editableTrimEndMs - 100)  // 100ms minimum
                        }
                    } else {
                        // Dragging end trim boundary
                        editableTrimEndMs = newMs
                        // Ensure end > start and end <= maxDuration
                        if (editableTrimEndMs > maxDuration) editableTrimEndMs = maxDuration
                        if (editableTrimStartMs >= 0 && editableTrimStartMs >= editableTrimEndMs) {
                            editableTrimEndMs = Math.min(maxDuration, editableTrimStartMs + 100)  // 100ms minimum
                        }
                    }
                    waveformCanvas.requestPaint()
                    return
                }
                
                // Handle segment boundary dragging
                if (draggedSegmentIndex >= 0) {
                    var seg = editableSegments[draggedSegmentIndex]
                    var startMs = dragStartMs
                    
                    // Update start or end boundary based on which one was clicked
                    var clickedMs = dragStartX / scaleX
                    var isDraggingStart = Math.abs(clickedMs - seg.startMs) < Math.abs(clickedMs - (seg.endMs || clickedMs + 1000))
                    
                    if (isDraggingStart) {
                        // Dragging start boundary
                        seg.startMs = newMs
                        // Ensure start >= 0 and start < end
                        if (seg.startMs < 0) seg.startMs = 0
                        if (seg.endMs && seg.startMs >= seg.endMs) {
                            seg.startMs = Math.max(0, seg.endMs - 100)  // 100ms minimum
                        }
                    } else {
                        // Dragging end boundary
                        seg.endMs = newMs
                        // Ensure end > start and end <= maxDuration
                        if (seg.endMs > maxDuration) seg.endMs = maxDuration
                        if (seg.startMs >= seg.endMs) {
                            seg.endMs = Math.min(maxDuration, seg.startMs + 100)  // 100ms minimum
                        }
                    }
                    
                    _segmentsVersion++
                    waveformCanvas.requestPaint()
                }
            }
        }
        
        onReleased: {
            // Handle trim boundary release
            if (draggedTrimBoundary >= 0) {
                root.trimBoundaryChanged(editableTrimStartMs, editableTrimEndMs)
                // Reset dragged state after emitting signal
                draggedTrimBoundary = -1
                // Force repaint to show final position
                waveformCanvas.requestPaint()
                return
            }
            
            // Handle segment boundary release
            if (draggedSegmentIndex >= 0) {
                // Emit signal with updated boundaries
                var boundaries = []
                for (var i = 0; i < editableSegments.length; i++) {
                    boundaries.push(editableSegments[i].startMs)
                }
                // Add final end boundary
                if (editableSegments.length > 0) {
                    var lastSeg = editableSegments[editableSegments.length - 1]
                    if (lastSeg.endMs) {
                        boundaries.push(lastSeg.endMs)
                    }
                }
                root.segmentBoundaryChanged(draggedSegmentIndex, editableSegments[draggedSegmentIndex].startMs, editableSegments[draggedSegmentIndex].endMs || 0)
                draggedSegmentIndex = -1
            }
        }
    }
}

