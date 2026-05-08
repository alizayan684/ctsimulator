import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

Item {
    id: root
    objectName: "mainView"
    anchors.fill: parent

    // ── State ────────────────────────────────────────────────────────────
    property bool comparisonMode: false

    // ── File Dialogs ─────────────────────────────────────────────────────
    FileDialog {
        id: phantomFileDialog
        title: "Select Phantom File"
        nameFilters: ["Phantom files (*.txt *.raw)", "All files (*)"]
        onAccepted: {
            ctController.loadPhantom(selectedFile)
            phantomCombo.currentIndex = 1
        }
    }

    FileDialog {
        id: exportFileDialog
        title: "Export Reconstruction Image"
        nameFilters: ["PNG images (*.png)", "TIFF images (*.tiff *.tif)", "JPEG images (*.jpg)", "All files (*)"]
        fileMode: FileDialog.SaveFile
        onAccepted: ctController.exportImage(selectedFile)
    }

    FileDialog {
        id: exportSinogramDialog
        title: "Export Sinogram"
        nameFilters: ["PNG images (*.png)", "JPEG images (*.jpg)", "All files (*)"]
        fileMode: FileDialog.SaveFile
        onAccepted: ctController.exportSinogram(selectedFile)
    }

    FileDialog {
        id: saveSessionDialog
        title: "Save Session"
        nameFilters: ["JSON files (*.json)"]
        fileMode: FileDialog.SaveFile
        onAccepted: ctController.saveSession(selectedFile)
    }

    FileDialog {
        id: loadSessionDialog
        title: "Load Session"
        nameFilters: ["JSON files (*.json)"]
        onAccepted: ctController.loadSession(selectedFile)
    }

    // ── Error Dialog ─────────────────────────────────────────────────────
    Dialog {
        id: errorDialog
        title: "Error"
        modal: true
        anchors.centerIn: parent
        width: 360

        property string errorMessage: ""

        background: Rectangle {
            color: "#1e293b"
            radius: 12
            border.color: "#ef4444"
            border.width: 1
        }

        Label {
            text: errorDialog.errorMessage
            color: "#f1f5f9"
            wrapMode: Text.Wrap
            width: parent.width - 40
        }

        standardButtons: Dialog.Ok
    }

    // ── Signal Connections ───────────────────────────────────────────────
    Connections {
        target: ctController
        function onError(message) {
            errorDialog.errorMessage = message
            errorDialog.open()
        }
        function onPhantomLoadedChanged() {
            if (ctController.phantomLoaded) {
                phantomImage.source = "image://ct/phantom?" + Date.now()
                phantomThumb.source = "image://ct/phantom?" + Date.now()
                reconstructionImage.visible = false
            }
        }
        function onSinogramReady() {
            sinogramImage.source = "image://ct/sinogram?" + Date.now()
        }
        function onReconstructionUpdated(iteration) {
            reconstructionImage.source = "image://ct/reconstruction?" + iteration + "_" + Date.now()
            reconstructionImage.visible = true
        }
        function onReconstructionFinished() {
            reconstructionImage.source = "image://ct/reconstruction?done_" + Date.now()
            reconstructionImage.visible = true
        }
    }

    // ── Main Layout ─────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: "#0f172a"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            // ════════════════════════════════════════════════════════════
            // 🔵 LEFT SIDEBAR — Phantom & Session
            // ════════════════════════════════════════════════════════════
            Rectangle {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                radius: 12
                color: "#1e293b"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    // Section: Phantom Selection
                    Label {
                        text: "🔬 Phantom Selection"
                        color: "#38bdf8"
                        font.bold: true
                        font.pixelSize: 14
                    }

                    ComboBox {
                        id: phantomCombo
                        Layout.fillWidth: true
                        model: ["Shepp-Logan", "Custom"]
                        onActivated: {
                            if (currentText === "Shepp-Logan") {
                                ctController.generateSheppLogan()
                            }
                        }

                        background: Rectangle {
                            radius: 8
                            color: "#334155"
                            border.color: "#475569"
                            border.width: 1
                        }
                    }

                    Button {
                        text: "📂  Upload Phantom"
                        Layout.fillWidth: true
                        onClicked: phantomFileDialog.open()

                        background: Rectangle {
                            radius: 8
                            color: parent.hovered ? "#4a5e7a" : "#334155"
                            border.color: "#475569"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#e2e8f0"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    // Phantom info
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        radius: 8
                        color: "#0f172a"
                        visible: ctController.phantomLoaded

                        Column {
                            anchors.centerIn: parent
                            spacing: 4

                            Image {
                                id: phantomThumb
                                source: ctController.phantomLoaded ? "image://ct/phantom?" + Date.now() : ""
                                width: 56; height: 56
                                fillMode: Image.PreserveAspectFit
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Label {
                                text: "Phantom loaded ✓"
                                color: "#4ade80"
                                font.pixelSize: 11
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }

                    // Divider
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

                    // Section: View Options
                    Label {
                        text: "👁 View Options"
                        color: "#38bdf8"
                        font.bold: true
                        font.pixelSize: 14
                    }

                    Button {
                        text: comparisonMode ? "✦  Single View" : "⇔  Compare View"
                        Layout.fillWidth: true
                        onClicked: comparisonMode = !comparisonMode

                        background: Rectangle {
                            radius: 8
                            color: parent.hovered ? "#6366f1" : "#4f46e5"
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.bold: true
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Item { Layout.fillHeight: true }

                    // Divider
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

                    // Section: Session
                    Label {
                        text: "💾 Session"
                        color: "#38bdf8"
                        font.bold: true
                        font.pixelSize: 14
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            text: "Load"
                            Layout.fillWidth: true
                            onClicked: loadSessionDialog.open()

                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? "#4a5e7a" : "#334155"
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                            contentItem: Text {
                                text: parent.text; color: "#e2e8f0"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Button {
                            text: "Save"
                            Layout.fillWidth: true
                            onClicked: saveSessionDialog.open()

                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? "#4a5e7a" : "#334155"
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                            contentItem: Text {
                                text: parent.text; color: "#e2e8f0"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    // Undo / Redo buttons
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            text: "↩ Undo"
                            Layout.fillWidth: true
                            enabled: ctController.canUndo
                            onClicked: ctController.undo()

                            background: Rectangle {
                                radius: 8
                                color: parent.enabled ? (parent.hovered ? "#4a5e7a" : "#334155") : "#1e293b"
                                opacity: parent.enabled ? 1.0 : 0.5
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                            contentItem: Text {
                                text: parent.text; color: parent.enabled ? "#e2e8f0" : "#64748b"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Button {
                            text: "↪ Redo"
                            Layout.fillWidth: true
                            enabled: ctController.canRedo
                            onClicked: ctController.redo()

                            background: Rectangle {
                                radius: 8
                                color: parent.enabled ? (parent.hovered ? "#4a5e7a" : "#334155") : "#1e293b"
                                opacity: parent.enabled ? 1.0 : 0.5
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                            contentItem: Text {
                                text: parent.text; color: parent.enabled ? "#e2e8f0" : "#64748b"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
            }

            // ════════════════════════════════════════════════════════════
            // 🟣 CENTER — Image Views
            // ════════════════════════════════════════════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                // ── Main image area ──────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 12
                    color: "#020617"
                    clip: true

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        // PHANTOM VIEW (visible in comparison mode, or when no reconstruction)
                        Rectangle {
                            Layout.fillWidth: comparisonMode
                            Layout.fillHeight: true
                            visible: comparisonMode
                            radius: 8
                            color: "#0f172a"

                            Column {
                                anchors.fill: parent
                                anchors.margins: 6

                                Label {
                                    text: "Original Phantom"
                                    color: "#94a3b8"
                                    font.pixelSize: 12
                                    font.bold: true
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }

                                Item {
                                    width: parent.width - 12
                                    height: parent.height - 30
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    clip: true
                                    Image {
                                        id: phantomCompareImage
                                        source: ctController.phantomLoaded ? "image://ct/phantom?" + Date.now() : ""
                                        width: parent.width
                                        height: parent.height
                                        x: (parent.width - width) / 2
                                        y: (parent.height - height) / 2
                                        fillMode: Image.PreserveAspectFit

                                        PinchHandler {
                                            target: phantomCompareImage
                                            minimumScale: 1.0
                                            maximumScale: 10.0
                                        }
                                        DragHandler {
                                            target: phantomCompareImage
                                        }
                                        TapHandler {
                                            onDoubleTapped: {
                                                phantomCompareImage.scale = 1.0
                                                phantomCompareImage.x = (phantomCompareImage.parent.width - phantomCompareImage.width) / 2
                                                phantomCompareImage.y = (phantomCompareImage.parent.height - phantomCompareImage.height) / 2
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Divider in comparison mode
                        Rectangle {
                            width: 2
                            Layout.fillHeight: true
                            color: "#4f46e5"
                            visible: comparisonMode
                        }

                        // RECONSTRUCTION / PHANTOM VIEW
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 8
                            color: "#0f172a"

                            Column {
                                anchors.fill: parent
                                anchors.margins: 6

                                Label {
                                    text: comparisonMode ? "Reconstruction" : (reconstructionImage.visible ? "Reconstruction View" : "Phantom View")
                                    color: "#94a3b8"
                                    font.pixelSize: 12
                                    font.bold: true
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }

                                // Phantom (shown when no reconstruction yet)
                                Item {
                                    width: parent.width - 12
                                    height: parent.height - 30
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    visible: ctController.phantomLoaded && !reconstructionImage.visible && !comparisonMode
                                    clip: true
                                    Image {
                                        id: phantomImage
                                        width: parent.width
                                        height: parent.height
                                        x: (parent.width - width) / 2
                                        y: (parent.height - height) / 2
                                        fillMode: Image.PreserveAspectFit

                                        PinchHandler {
                                            target: phantomImage
                                            minimumScale: 1.0
                                            maximumScale: 10.0
                                        }
                                        DragHandler {
                                            target: phantomImage
                                        }
                                        TapHandler {
                                            onDoubleTapped: {
                                                phantomImage.scale = 1.0
                                                phantomImage.x = (phantomImage.parent.width - phantomImage.width) / 2
                                                phantomImage.y = (phantomImage.parent.height - phantomImage.height) / 2
                                            }
                                        }
                                    }
                                }

                                // Reconstruction image
                                Item {
                                    width: parent.width - 12
                                    height: parent.height - 30
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    visible: reconstructionImage.visible
                                    clip: true
                                    Image {
                                        id: reconstructionImage
                                        width: parent.width
                                        height: parent.height
                                        x: (parent.width - width) / 2
                                        y: (parent.height - height) / 2
                                        fillMode: Image.PreserveAspectFit
                                        visible: false

                                        PinchHandler {
                                            target: reconstructionImage
                                            minimumScale: 1.0
                                            maximumScale: 10.0
                                        }
                                        DragHandler {
                                            target: reconstructionImage
                                        }
                                        TapHandler {
                                            onDoubleTapped: {
                                                reconstructionImage.scale = 1.0
                                                reconstructionImage.x = (reconstructionImage.parent.width - reconstructionImage.width) / 2
                                                reconstructionImage.y = (reconstructionImage.parent.height - reconstructionImage.height) / 2
                                            }
                                        }
                                    }
                                }

                                // Busy indicator when sinogram is generating
                                BusyIndicator {
                                    anchors.centerIn: parent
                                    running: ctController.sinogramGenerating
                                    visible: ctController.sinogramGenerating
                                    width: 48; height: 48
                                }

                                Label {
                                    text: "Generating sinogram…"
                                    color: "#94a3b8"
                                    font.pixelSize: 13
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.bottom
                                    anchors.bottomMargin: 20
                                    visible: ctController.sinogramGenerating
                                }

                                // Busy indicator when reconstruction is running
                                BusyIndicator {
                                    anchors.centerIn: parent
                                    running: ctController.reconstructionRunning
                                    visible: ctController.reconstructionRunning
                                    width: 48; height: 48
                                }

                                Label {
                                    text: "Reconstructing…"
                                    color: "#94a3b8"
                                    font.pixelSize: 13
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.bottom
                                    anchors.bottomMargin: 20
                                    visible: ctController.reconstructionRunning
                                }
                            }
                        }
                    }
                }

                // ── Sinogram strip ───────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 140
                    radius: 12
                    color: "#020617"

                    Label {
                        text: "Sinogram"
                        color: "#94a3b8"
                        font.pixelSize: 12
                        font.bold: true
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.margins: 10
                    }

                    Image {
                        id: sinogramImage
                        anchors.centerIn: parent
                        width: parent.width * 0.95
                        height: parent.height * 0.8
                        fillMode: Image.PreserveAspectFit
                    }

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: ctController.sinogramGenerating
                        visible: ctController.sinogramGenerating
                        width: 32; height: 32
                    }
                }

                // ── Progress & Action Bar ────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    spacing: 12

                    // Progress info
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 10
                        color: "#1e293b"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: "Progress"
                                    color: "#94a3b8"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                                Item { Layout.fillWidth: true }
                                Label {
                                    text: ctController.reconstructionRunning
                                          ? "Iteration " + ctController.currentIteration + "/" + ctController.iterations
                                          : (ctController.progress >= 1.0 ? "Complete" : "Idle")
                                    color: ctController.reconstructionRunning ? "#38bdf8" : "#64748b"
                                    font.pixelSize: 12
                                }
                            }

                            ProgressBar {
                                id: reconstructionProgressBar
                                Layout.fillWidth: true
                                value: ctController.progress
                                background: Rectangle {
                                    implicitHeight: 6
                                    radius: 3
                                    color: "#334155"
                                }
                                contentItem: Item {
                                    implicitHeight: 6
                                    Rectangle {
                                        width: reconstructionProgressBar.visualPosition * parent.width
                                        height: parent.height
                                        radius: 3
                                        gradient: Gradient {
                                            orientation: Gradient.Horizontal
                                            GradientStop { position: 0.0; color: "#6366f1" }
                                            GradientStop { position: 1.0; color: "#38bdf8" }
                                        }
                                    }
                                }
                            }

                            Label {
                                text: "Residual: " + ctController.currentResidual.toFixed(4)
                                color: "#64748b"
                                font.pixelSize: 11
                                visible: ctController.reconstructionRunning || ctController.currentResidual > 0
                            }
                        }
                    }

                    // Action buttons
                    Rectangle {
                        Layout.preferredWidth: 260
                        Layout.fillHeight: true
                        radius: 10
                        color: "#1e293b"

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 8

                            Button {
                                text: "▶ Start"
                                enabled: ctController.phantomLoaded && !ctController.reconstructionRunning && !ctController.sinogramGenerating
                                onClicked: ctController.startReconstruction()

                                background: Rectangle {
                                    radius: 8
                                    implicitWidth: 80
                                    implicitHeight: 36
                                    color: parent.enabled ? (parent.hovered ? "#16a34a" : "#15803d") : "#1e293b"
                                    opacity: parent.enabled ? 1.0 : 0.5
                                    Behavior on color { ColorAnimation { duration: 120 } }
                                }
                                contentItem: Text {
                                    text: parent.text; color: "white"
                                    font.bold: true; font.pixelSize: 13
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            Button {
                                text: "⏹ Stop"
                                enabled: ctController.reconstructionRunning
                                onClicked: ctController.stopReconstruction()

                                background: Rectangle {
                                    radius: 8
                                    implicitWidth: 80
                                    implicitHeight: 36
                                    color: parent.enabled ? (parent.hovered ? "#dc2626" : "#b91c1c") : "#1e293b"
                                    opacity: parent.enabled ? 1.0 : 0.5
                                    Behavior on color { ColorAnimation { duration: 120 } }
                                }
                                contentItem: Text {
                                    text: parent.text; color: "white"
                                    font.bold: true; font.pixelSize: 13
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                }
            }

            // ════════════════════════════════════════════════════════════
            // 🟠 RIGHT PANEL — Controls
            // ════════════════════════════════════════════════════════════
            Rectangle {
                Layout.preferredWidth: 270
                Layout.fillHeight: true
                radius: 12
                color: "#1e293b"

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 14
                    contentHeight: rightColumn.implicitHeight
                    clip: true

                    ColumnLayout {
                        id: rightColumn
                        width: parent.width
                        spacing: 8

                        // ── Acquisition Controls ─────────────────────
                        Label {
                            text: "⚡ Acquisition"
                            color: "#38bdf8"
                            font.bold: true; font.pixelSize: 14
                        }

                        Label {
                            text: "Projections: " + projectionsSlider.value.toFixed(0)
                            color: "#e2e8f0"; font.pixelSize: 12
                        }
                        Slider {
                            id: projectionsSlider
                            Layout.fillWidth: true
                            from: 1; to: 720; value: 180; stepSize: 1
                        }

                        Label {
                            text: "Voltage: " + voltageSlider.value.toFixed(0) + " kV"
                            color: "#e2e8f0"; font.pixelSize: 12
                        }
                        Slider {
                            id: voltageSlider
                            Layout.fillWidth: true
                            from: 80; to: 150; value: 120; stepSize: 1
                        }

                        Label {
                            text: "Current: " + currentSlider.value.toFixed(0) + " mA"
                            color: "#e2e8f0"; font.pixelSize: 12
                        }
                        Slider {
                            id: currentSlider
                            Layout.fillWidth: true
                            from: 10; to: 300; value: 100; stepSize: 1
                        }

                        Button {
                            text: "📊  Generate Sinogram"
                            Layout.fillWidth: true
                            enabled: ctController.phantomLoaded && !ctController.reconstructionRunning && !ctController.sinogramGenerating
                            onClicked: ctController.generateSinogram(
                                           projectionsSlider.value,
                                           voltageSlider.value,
                                           currentSlider.value)

                            background: Rectangle {
                                radius: 8
                                color: parent.enabled ? (parent.hovered ? "#6366f1" : "#4f46e5") : "#1e293b"
                                opacity: parent.enabled ? 1.0 : 0.5
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                            contentItem: Text {
                                text: parent.text; color: "white"
                                font.bold: true; font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

                        // ── Algorithm Selection ──────────────────────
                        Label {
                            text: "🧮 Algorithm"
                            color: "#38bdf8"
                            font.bold: true; font.pixelSize: 14
                        }

                        Connections {
                            target: ctController
                            function onCurrentAlgorithmChanged() {
                                algorithmCombo.currentIndex = algorithmCombo.model.indexOf(ctController.currentAlgorithm)
                            }
                        }

                        ComboBox {
                            id: algorithmCombo
                            Layout.fillWidth: true
                            model: ["ART", "SIRT", "FBP", "MLEM"]
                            onActivated: ctController.currentAlgorithm = model[currentIndex]

                            background: Rectangle {
                                radius: 8
                                color: "#334155"
                                border.color: "#475569"
                                border.width: 1
                            }
                        }

                        // Filter (FBP only)
                        Label {
                            text: "Filter:"
                            color: "#e2e8f0"; font.pixelSize: 12
                            visible: algorithmCombo.currentText === "FBP"
                        }
                        Connections {
                            target: ctController
                            function onCurrentFilterChanged() {
                                filterCombo.currentIndex = filterCombo.model.indexOf(ctController.currentFilter)
                            }
                        }

                        ComboBox {
                            id: filterCombo
                            Layout.fillWidth: true
                            model: ["Ram-Lak", "Shepp-Logan", "Cosine", "Hamming", "Hann"]
                            visible: algorithmCombo.currentText === "FBP"
                            onActivated: ctController.currentFilter = model[currentIndex]

                            background: Rectangle {
                                radius: 8
                                color: "#334155"
                                border.color: "#475569"
                                border.width: 1
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

                        // ── Reconstruction Parameters ────────────────
                        Label {
                            text: "⚙ Parameters"
                            color: "#38bdf8"
                            font.bold: true; font.pixelSize: 14
                            visible: algorithmCombo.currentText !== "FBP"
                        }

                        // Presets
                        Label {
                            text: "Preset:"
                            color: "#e2e8f0"; font.pixelSize: 12
                            visible: algorithmCombo.currentText !== "FBP"
                        }
                        ComboBox {
                            id: presetCombo
                            Layout.fillWidth: true
                            model: ["Custom", "Quick Preview", "Standard", "High Quality"]
                            visible: algorithmCombo.currentText !== "FBP"
                            onActivated: {
                                if (currentText !== "Custom") {
                                    ctController.applyPreset(currentText)
                                }
                            }

                            background: Rectangle {
                                radius: 8
                                color: "#334155"
                                border.color: "#475569"
                                border.width: 1
                            }
                        }

                        Label {
                            text: "Iterations: " + iterationsSlider.value.toFixed(0)
                            color: "#e2e8f0"; font.pixelSize: 12
                            visible: algorithmCombo.currentText !== "FBP"
                        }
                        Slider {
                            id: iterationsSlider
                            Layout.fillWidth: true
                            from: 1; to: 200; value: ctController.iterations; stepSize: 1
                            visible: algorithmCombo.currentText !== "FBP"
                            onReleased: {
                                ctController.iterations = Math.floor(value)
                                presetCombo.currentIndex = 0  // Switch to "Custom"
                            }
                        }

                        Label {
                            text: "Relaxation: " + relaxationSlider.value.toFixed(3)
                            color: "#e2e8f0"; font.pixelSize: 12
                            visible: algorithmCombo.currentText !== "FBP"
                        }
                        Slider {
                            id: relaxationSlider
                            Layout.fillWidth: true
                            from: 0.01; to: 1.0; value: ctController.relaxation
                            visible: algorithmCombo.currentText !== "FBP"
                            onReleased: {
                                ctController.relaxation = value
                                presetCombo.currentIndex = 0
                            }
                        }

                        Label {
                            text: "Update Every: " + showEverySlider.value.toFixed(0)
                            color: "#e2e8f0"; font.pixelSize: 12
                            visible: algorithmCombo.currentText !== "FBP"
                        }
                        Slider {
                            id: showEverySlider
                            Layout.fillWidth: true
                            from: 1; to: 50; value: ctController.showEvery; stepSize: 1
                            visible: algorithmCombo.currentText !== "FBP"
                            onReleased: {
                                ctController.showEvery = Math.floor(value)
                                presetCombo.currentIndex = 0
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

                        // ── Export ────────────────────────────────────
                        Label {
                            text: "📤 Export"
                            color: "#38bdf8"
                            font.bold: true; font.pixelSize: 14
                        }

                        Button {
                            text: "Export Reconstruction"
                            Layout.fillWidth: true
                            onClicked: exportFileDialog.open()

                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? "#4a5e7a" : "#334155"
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                            contentItem: Text {
                                text: parent.text; color: "#e2e8f0"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Button {
                            text: "Export Sinogram"
                            Layout.fillWidth: true
                            onClicked: exportSinogramDialog.open()

                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? "#4a5e7a" : "#334155"
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                            contentItem: Text {
                                text: parent.text; color: "#e2e8f0"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
            }
        }
    }
}