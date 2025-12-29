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
                    ctx.strokeStyle = "#00bd52"
                    ctx.lineWidth = 2
                    ctx.setLineDash([5, 5])  // Dashed lines
                    
                    // NEW LOGIC: Draw boundaries directly from boundaries array
                    if (root.interactive && boundaries && boundaries.length > 0) {
                        for (var i = 0; i < boundaries.length; i++) {
                            var boundaryMs = boundaries[i]
                            var x = boundaryMs * scaleX
                            ctx.beginPath()
                            ctx.moveTo(x, 0)
                            ctx.lineTo(x, height)
                            ctx.stroke()
                        }
                    } else {
                        // Fallback: draw from segments (for non-interactive mode)
                        var segsToDraw = segments
                        if (segsToDraw) {
                            var drawnBoundaries = []
                            for (var i = 0; i < segsToDraw.length; i++) {
                                var seg = segsToDraw[i]
                                var startX = seg.startMs * scaleX
                                
                                if (drawnBoundaries.indexOf(startX) < 0) {
                                    ctx.beginPath()
                                    ctx.moveTo(startX, 0)
                                    ctx.lineTo(startX, height)
                                    ctx.stroke()
                                    drawnBoundaries.push(startX)
                                }
                                
                                if (seg.endMs) {
                                    var endX = seg.endMs * scaleX
                                    if (endX !== startX && drawnBoundaries.indexOf(endX) < 0) {
                                        ctx.beginPath()
                                        ctx.moveTo(endX, 0)
                                        ctx.lineTo(endX, height)
                                        ctx.stroke()
                                        drawnBoundaries.push(endX)
                                    }
                                }
                            }
                        }
                    }
                    ctx.setLineDash([])
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
    property var boundaries: []  // Array of boundary positions in ms (shared boundaries between segments)
    property bool manualBoundaries: false  // Flag indicating that boundaries were set manually
    
    // Dragging state
    property int draggedBoundaryIndex: -1  // Index of boundary being dragged (-1 if none)
    property double dragStartX: 0  // Mouse X position when dragging started
    property double dragStartMs: 0  // Boundary position in ms when dragging started
    property double dragScaleX: 1.0  // ScaleX at the start of dragging (to keep it constant)
    
    // OLD LOGIC - COMMENTED OUT
    // property int draggedSegmentIndex: -1  // Index of segment being dragged
    // property bool isDraggingStartBoundary: false  // true if dragging start, false if end
    
    // Trim boundary editing
    property int draggedTrimBoundary: -1  // -1 = none, 0 = start, 1 = end
    property double editableTrimStartMs: -1.0
    property double editableTrimEndMs: -1.0
    
    // Flag to force recreation of boundaries (e.g., when segment length changes)
    property bool forceRecreateBoundaries: false
    
    // Initialize editable segments and boundaries when segments change
    onSegmentsChanged: {
        _segmentsVersion++
        if (root.interactive && root.showSegments) {
            // If boundaries are already set and we don't need to force recreation, don't recreate them
            if (!forceRecreateBoundaries && boundaries && boundaries.length > 0) {
                console.log("Boundaries already set (count:", boundaries.length, "), skipping recreation. Current boundaries:", boundaries)
                // Just update editableSegments structure to match current segments
                // But keep the existing boundaries - they are the source of truth
                editableSegments = []
                if (segments && segments.length > 0) {
                    for (var i = 0; i < segments.length; i++) {
                        editableSegments.push({
                            index: segments[i].index,
                            startMs: segments[i].startMs,
                            endMs: segments[i].endMs
                        })
                    }
                    // Update segments to match boundaries (boundaries are source of truth)
                    updateSegmentsFromBoundaries()
                }
                return
            }
            
            // Reset force flag
            forceRecreateBoundaries = false
            
            editableSegments = []
            boundaries = []
            
            if (segments && segments.length > 0) {
                console.log("=== NEW LOGIC: Initializing from", segments.length, "segments ===")
                
                // Copy segments
                for (var i = 0; i < segments.length; i++) {
                    editableSegments.push({
                        index: segments[i].index,
                        startMs: segments[i].startMs,
                        endMs: segments[i].endMs
                    })
                }
                
                // Extract unique boundaries from segments
                // Segments are stored in reverse order (end to start)
                // For N segments, we need N+1 boundaries (start, N-1 between, end)
                var boundaryArray = []
                
                if (editableSegments.length > 0) {
                    // Add start boundary (first segment's start, should be 0 or close to it)
                    var firstSeg = editableSegments[editableSegments.length - 1]  // First segment in time
                    boundaryArray.push(firstSeg.startMs)
                    
                    // Add all boundaries between segments (endMs = startMs of next)
                    // Since segments are in reverse order, we go from end to start
                    for (var i = 0; i < editableSegments.length - 1; i++) {
                        var currSeg = editableSegments[i]      // Later in time
                        var nextSeg = editableSegments[i + 1] // Earlier in time
                        
                        // The boundary is shared: currSeg.endMs should equal nextSeg.startMs
                        // Use the average if they differ slightly (due to rounding)
                        var boundaryMs
                        if (Math.abs(currSeg.endMs - nextSeg.startMs) < 1.0) {
                            // They're close enough, use average
                            boundaryMs = (currSeg.endMs + nextSeg.startMs) / 2.0
                        } else {
                            // They differ significantly, prefer endMs (more reliable)
                            boundaryMs = currSeg.endMs || nextSeg.startMs
                        }
                        
                        if (boundaryMs >= 0) {
                            boundaryArray.push(boundaryMs)
                        }
                    }
                    
                    // Always add end of last segment (even if it's 0, it should be the end of audio)
                    var lastSeg = editableSegments[0]  // Last segment in time
                    if (lastSeg.endMs >= 0) {
                        boundaryArray.push(lastSeg.endMs)
                    }
                }
                
                // Sort and remove duplicates (with small tolerance for floating point)
                boundaryArray.sort(function(a, b) { return a - b })
                var uniqueBoundaries = []
                for (var i = 0; i < boundaryArray.length; i++) {
                    var b = boundaryArray[i]
                    // Check if this boundary is too close to the previous one
                    if (uniqueBoundaries.length === 0 || Math.abs(b - uniqueBoundaries[uniqueBoundaries.length - 1]) > 0.1) {
                        uniqueBoundaries.push(b)
                    }
                }
                
                boundaries = uniqueBoundaries
                
                // Validate: for N segments, we should have N+1 boundaries
                var expectedBoundaries = editableSegments.length + 1
                if (boundaries.length !== expectedBoundaries) {
                    console.warn("WARNING: Expected", expectedBoundaries, "boundaries for", editableSegments.length, 
                                "segments, but got", boundaries.length)
                }
                
                console.log("Extracted", boundaries.length, "boundaries from", editableSegments.length, "segments (expected", expectedBoundaries, "):", boundaries)
                
                // Update segments to use synchronized boundaries
                updateSegmentsFromBoundaries()
            }
        }
    }
    
    // Update editableSegments from boundaries array
    function updateSegmentsFromBoundaries() {
        if (!boundaries || boundaries.length < 2) {
            return
        }
        
        // If editableSegments is empty or has wrong count, recreate it from boundaries
        // For N boundaries, we should have N-1 segments
        var expectedSegCount = boundaries.length - 1
        
        if (!editableSegments || editableSegments.length !== expectedSegCount) {
            console.log("Recreating editableSegments: expected", expectedSegCount, "segments from", boundaries.length, "boundaries")
            editableSegments = []
            // Create segments from boundaries (in reverse order to match storage)
            for (var i = boundaries.length - 2; i >= 0; i--) {
                editableSegments.push({
                    index: editableSegments.length + 1,  // Will be updated from actual segments if available
                    startMs: boundaries[i],
                    endMs: boundaries[i + 1]
                })
            }
            // If we have actual segments data, copy the index
            if (segments && segments.length === editableSegments.length) {
                for (var j = 0; j < editableSegments.length; j++) {
                    var reverseIdx = editableSegments.length - 1 - j
                    editableSegments[j].index = segments[reverseIdx].index
                }
            }
        } else {
            // Update existing segments from boundaries
            // Segments are stored in reverse order (end to start of song)
            // Boundaries are in forward order (start to end of song)
            var segCount = editableSegments.length
            var boundaryCount = boundaries.length
            
            // Map boundaries to segments (reverse order)
            for (var i = 0; i < segCount; i++) {
                var seg = editableSegments[i]
                // Convert reverse index to forward index
                var forwardIndex = segCount - 1 - i
                
                // Each segment uses two consecutive boundaries
                var startBoundaryIdx = forwardIndex
                var endBoundaryIdx = forwardIndex + 1
                
                if (startBoundaryIdx >= 0 && startBoundaryIdx < boundaryCount) {
                    seg.startMs = boundaries[startBoundaryIdx]
                }
                if (endBoundaryIdx >= 0 && endBoundaryIdx < boundaryCount) {
                    seg.endMs = boundaries[endBoundaryIdx]
                }
            }
        }
        
        _segmentsVersion++
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
        acceptedButtons: Qt.LeftButton | Qt.RightButton
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
        
        // NEW LOGIC: Find boundary at X position
        function findBoundaryAtX(x) {
            if (!boundaries || boundaries.length === 0) return -1
            var ms = x / scaleX
            var threshold = 10 / scaleX  // 10ms threshold for clicking
            
            // Find the closest boundary
            for (var i = 0; i < boundaries.length; i++) {
                if (Math.abs(ms - boundaries[i]) < threshold) {
                    return i
                }
            }
            return -1
        }
        
        // OLD LOGIC - COMMENTED OUT
        /*
        function findSegmentAtX(x) {
            if (!editableSegments || editableSegments.length === 0) return -1
            var ms = x / scaleX
            var threshold = 10 / scaleX  // 10ms threshold for clicking
            
            // Check boundaries - prioritize finding the exact boundary position
            // A boundary can be both endMs of one segment and startMs of the next
            for (var i = 0; i < editableSegments.length; i++) {
                var seg = editableSegments[i]
                
                // Check if this is a start boundary (but not the first segment's start)
                if (i > 0 && Math.abs(ms - seg.startMs) < threshold) {
                    // This is a shared boundary: endMs of previous = startMs of current
                    return i  // Return current segment index, dragging start
                }
                
                // Check if this is an end boundary
                if (seg.endMs && Math.abs(ms - seg.endMs) < threshold) {
                    // This is a shared boundary: endMs of current = startMs of next (if exists)
                    return i  // Return current segment index, dragging end
                }
            }
            
            // Check first segment's start boundary (only if it's not at 0)
            if (editableSegments.length > 0) {
                var firstSeg = editableSegments[0]
                if (firstSeg.startMs > 0 && Math.abs(ms - firstSeg.startMs) < threshold) {
                    return 0  // Return first segment index, dragging start
                }
            }
            
            return -1
        }
        */
        
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
            
            // Handle right click for context menu
            if (mouse.button === Qt.RightButton && root.showSegments) {
                var boundaryIndex = findBoundaryAtX(mouseX)
                var clickMs = mouseX / scaleX
                
                // Get max duration for validation
                var maxDuration = 0
                if (volumeData && volumeData.length > 0) {
                    maxDuration = volumeData[volumeData.length - 1].endMs
                }
                
                // Show context menu at mouse position
                contextMenu.boundaryIndex = boundaryIndex
                contextMenu.clickMs = clickMs
                contextMenu.maxDuration = maxDuration
                contextMenu.popup()
                return
            }
            
            // Left click handling
            if (mouse.button !== Qt.LeftButton) return
            
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
            
            // NEW LOGIC: Check for segment boundaries
            if (root.showSegments) {
                var boundaryIndex = findBoundaryAtX(mouseX)
                if (boundaryIndex >= 0) {
                    draggedBoundaryIndex = boundaryIndex
                    dragStartX = mouseX
                    dragScaleX = scaleX  // Save scaleX at the start of dragging
                    dragStartMs = boundaries[boundaryIndex]  // Current boundary position
                    console.log("NEW: Started dragging boundary", boundaryIndex, "at", dragStartMs, "ms")
                }
            }
            
            // OLD LOGIC - COMMENTED OUT
            /*
            // Then check for segment boundaries
            if (root.showSegments) {
                var segIndex = findSegmentAtX(mouseX)
                if (segIndex >= 0) {
                    draggedSegmentIndex = segIndex
                    dragStartX = mouseX
                    dragScaleX = scaleX  // Save scaleX at the start of dragging
                    var seg = editableSegments[segIndex]
                    var ms = mouseX / scaleX  // Real position of cursor in milliseconds
                    
                    // Determine if dragging start or end boundary
                    // Check which boundary is closer
                    var distToStart = Math.abs(ms - seg.startMs)
                    var distToEnd = seg.endMs ? Math.abs(ms - seg.endMs) : 1000000
                    
                    if (distToStart < distToEnd && segIndex > 0) {
                        // Dragging start boundary (shared with previous segment's end)
                        isDraggingStartBoundary = true
                        // Use the actual cursor position, not the stored value
                        dragStartMs = ms
                        console.log("Started dragging start boundary of segment", segIndex, "at cursor", ms, "ms (stored:", seg.startMs, "ms)")
                    } else if (seg.endMs) {
                        // Dragging end boundary (shared with next segment's start)
                        isDraggingStartBoundary = false
                        // Use the actual cursor position, not the stored value
                        dragStartMs = ms
                        console.log("Started dragging end boundary of segment", segIndex, "at cursor", ms, "ms (stored:", seg.endMs, "ms)")
                    } else {
                        // Fallback: dragging start
                        isDraggingStartBoundary = true
                        dragStartMs = ms
                        console.log("Started dragging start boundary (fallback) of segment", segIndex, "at cursor", ms, "ms")
                    }
                }
            }
            */
        }
        
        onPositionChanged: {
            if (pressed) {
                // Use saved scaleX to ensure consistent calculation during dragging
                var deltaX = mouseX - dragStartX
                var deltaMs = deltaX / dragScaleX
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
                
                // NEW LOGIC: Handle boundary dragging
                if (draggedBoundaryIndex >= 0) {
                    // Calculate new position
                    var deltaX = mouseX - dragStartX
                    var deltaMs = deltaX / dragScaleX
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
                    
                    // Ensure boundary doesn't cross adjacent boundaries
                    if (draggedBoundaryIndex > 0 && newMs <= boundaries[draggedBoundaryIndex - 1]) {
                        newMs = boundaries[draggedBoundaryIndex - 1] + 100  // 100ms minimum gap
                    }
                    if (draggedBoundaryIndex < boundaries.length - 1 && newMs >= boundaries[draggedBoundaryIndex + 1]) {
                        newMs = boundaries[draggedBoundaryIndex + 1] - 100  // 100ms minimum gap
                    }
                    
                    // Update boundary position
                    boundaries[draggedBoundaryIndex] = newMs
                    manualBoundaries = true  // Mark as manually adjusted

                    // Update segments from boundaries
                    updateSegmentsFromBoundaries()

                    waveformCanvas.requestPaint()
                }
                
                // OLD LOGIC - COMMENTED OUT
                /*
                // Handle segment boundary dragging
                if (draggedSegmentIndex >= 0) {
                    var seg = editableSegments[draggedSegmentIndex]
                    
                    if (isDraggingStartBoundary) {
                        // Dragging start boundary - this is shared with previous segment's end
                        // Update both: current segment's start and previous segment's end
                        var prevIndex = draggedSegmentIndex - 1
                        
                        if (prevIndex >= 0 && prevIndex < editableSegments.length) {
                            var prevSeg = editableSegments[prevIndex]
                            var oldPrevEnd = prevSeg.endMs
                            var oldCurrStart = seg.startMs
                            
                            // Update both boundaries to the same value
                            prevSeg.endMs = newMs
                            seg.startMs = newMs
                            
                            // Ensure boundaries are valid
                            if (newMs < 0) {
                                newMs = 0
                                prevSeg.endMs = 0
                                seg.startMs = 0
                            }
                            
                            // Ensure previous segment's start < end
                            if (prevSeg.startMs >= prevSeg.endMs) {
                                var minPrevEnd = prevSeg.startMs + 100  // 100ms minimum
                                prevSeg.endMs = minPrevEnd
                                seg.startMs = minPrevEnd
                                newMs = minPrevEnd
                            }
                            
                            // Ensure current segment's start < end
                            if (seg.endMs && seg.startMs >= seg.endMs) {
                                var maxCurrStart = seg.endMs - 100  // 100ms minimum
                                seg.startMs = maxCurrStart
                                prevSeg.endMs = maxCurrStart
                                newMs = maxCurrStart
                            }
                            
                            // Log only if actually changed
                            if (oldPrevEnd !== prevSeg.endMs || oldCurrStart !== seg.startMs) {
                                console.log("Dragging boundary: prev[" + prevIndex + "].endMs =", prevSeg.endMs, 
                                          "curr[" + draggedSegmentIndex + "].startMs =", seg.startMs,
                                          "mouseX =", mouseX, "deltaX =", deltaX)
                            }
                        } else {
                            // First segment - only update start
                            var oldStart = seg.startMs
                            seg.startMs = newMs
                            if (seg.startMs < 0) seg.startMs = 0
                            if (seg.endMs && seg.startMs >= seg.endMs) {
                                seg.startMs = Math.max(0, seg.endMs - 100)  // 100ms minimum
                            }
                            if (oldStart !== seg.startMs) {
                                console.log("Dragging first segment start:", seg.startMs, "ms")
                            }
                        }
                    } else {
                        // Dragging end boundary - this is shared with next segment's start
                        // Update both: current segment's end and next segment's start
                        var nextIndex = draggedSegmentIndex + 1
                        
                        if (nextIndex < editableSegments.length) {
                            var nextSeg = editableSegments[nextIndex]
                            var oldCurrEnd = seg.endMs
                            var oldNextStart = nextSeg.startMs
                            
                            // Update both boundaries to the same value
                            seg.endMs = newMs
                            nextSeg.startMs = newMs
                            
                            // Ensure boundaries are valid
                            if (newMs > maxDuration) {
                                newMs = maxDuration
                                seg.endMs = maxDuration
                                nextSeg.startMs = maxDuration
                            }
                            
                            // Ensure current segment's start < end
                            if (seg.startMs >= seg.endMs) {
                                var minCurrEnd = seg.startMs + 100  // 100ms minimum
                                seg.endMs = minCurrEnd
                                nextSeg.startMs = minCurrEnd
                                newMs = minCurrEnd
                            }
                            
                            // Ensure next segment's start < end
                            if (nextSeg.endMs && nextSeg.startMs >= nextSeg.endMs) {
                                var maxNextStart = nextSeg.endMs - 100  // 100ms minimum
                                nextSeg.startMs = maxNextStart
                                seg.endMs = maxNextStart
                                newMs = maxNextStart
                            }
                            
                            // Log only if actually changed
                            if (oldCurrEnd !== seg.endMs || oldNextStart !== nextSeg.startMs) {
                                console.log("Dragging boundary: curr[" + draggedSegmentIndex + "].endMs =", seg.endMs,
                                          "next[" + nextIndex + "].startMs =", nextSeg.startMs,
                                          "mouseX =", mouseX, "deltaX =", deltaX)
                            }
                        } else {
                            // Last segment - only update end
                            var oldEnd = seg.endMs
                            seg.endMs = newMs
                            if (seg.endMs > maxDuration) seg.endMs = maxDuration
                            if (seg.startMs >= seg.endMs) {
                                seg.endMs = Math.min(maxDuration, seg.startMs + 100)  // 100ms minimum
                            }
                            if (oldEnd !== seg.endMs) {
                                console.log("Dragging last segment end:", seg.endMs, "ms")
                            }
                        }
                    }
                    
                    _segmentsVersion++
                    waveformCanvas.requestPaint()
                }
                */
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
            
            // NEW LOGIC: Handle boundary release
            if (draggedBoundaryIndex >= 0) {
                console.log("NEW: Boundary drag released. Final boundaries:", boundaries)
                // Update segments from boundaries to ensure sync
                updateSegmentsFromBoundaries()
                // Emit signal with updated boundary info
                if (editableSegments && editableSegments.length > 0 && draggedBoundaryIndex < editableSegments.length) {
                    var seg = editableSegments[draggedBoundaryIndex < editableSegments.length ? editableSegments.length - 1 - draggedBoundaryIndex : 0]
                    root.segmentBoundaryChanged(seg.index || draggedBoundaryIndex, seg.startMs, seg.endMs || 0)
                }
                draggedBoundaryIndex = -1
                waveformCanvas.requestPaint()
            }
        }
    }
    
    // Context menu for adding/removing boundaries
    Menu {
        id: contextMenu
        property int boundaryIndex: -1  // -1 if clicking on empty space, >= 0 if clicking on boundary
        property double clickMs: 0  // Position in ms where user clicked
        property double maxDuration: 0  // Maximum duration for validation
        
        MenuItem {
            text: contextMenu.boundaryIndex >= 0 ? qsTr("Удалить метку") : qsTr("Добавить метку")
            enabled: root.showSegments
            onTriggered: {
                if (contextMenu.boundaryIndex >= 0) {
                    removeBoundary(contextMenu.boundaryIndex)
                } else {
                    addBoundary(contextMenu.clickMs, contextMenu.maxDuration)
                }
            }
        }
    }
    
    // Function to add a boundary at specified position
    function addBoundary(ms, maxDuration) {
        if (!boundaries || boundaries.length === 0) {
            // Initialize with start and end if empty
            boundaries = [0, maxDuration]
            manualBoundaries = true
            updateSegmentsFromBoundaries()
            waveformCanvas.requestPaint()
            return
        }
        
        // Clamp to valid range
        ms = Math.max(0, Math.min(maxDuration, ms))
        
        // Check if boundary already exists at this position (within 10ms threshold)
        for (var i = 0; i < boundaries.length; i++) {
            if (Math.abs(boundaries[i] - ms) < 10) {
                console.log("Boundary already exists at", ms, "ms")
                return
            }
        }
        
        // Add boundary and sort
        boundaries.push(ms)
        boundaries.sort(function(a, b) { return a - b })
        
        manualBoundaries = true
        updateSegmentsFromBoundaries()
        waveformCanvas.requestPaint()
        console.log("Added boundary at", ms, "ms. Total boundaries:", boundaries.length)
    }
    
    // Function to remove a boundary at specified index
    function removeBoundary(index) {
        if (!boundaries || boundaries.length <= 2) {
            console.log("Cannot remove boundary: minimum 2 boundaries required (start and end)")
            return
        }
        
        if (index < 0 || index >= boundaries.length) {
            console.log("Invalid boundary index:", index)
            return
        }
        
        // Don't allow removing first (0) or last boundary
        if (index === 0 || index === boundaries.length - 1) {
            console.log("Cannot remove start or end boundary")
            return
        }
        
        boundaries.splice(index, 1)
        manualBoundaries = true
        updateSegmentsFromBoundaries()
        waveformCanvas.requestPaint()
        console.log("Removed boundary at index", index, ". Remaining boundaries:", boundaries.length)
    }
}

