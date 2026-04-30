import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

Item {
    id: root
    objectName: "mainView"
    anchors.fill: parent

    // File dialog for loading phantom
    FileDialog {
        id: phantomFileDialog
        title: "Select Phantom File"
        nameFilters: ["Phantom files (*.txt *.raw)", "All files (*)"]
        onAccepted: {
            ctController.loadPhantom(selectedFile)
        }
    }

    // File dialog for exporting image
    FileDialog {
        id: exportFileDialog
        title: "Export Image"
        nameFilters: ["PNG images (*.png)", "JPEG images (*.jpg)", "All files (*)"]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            ctController.exportImage(selectedFile)
        }
    }

    // Error dialog
    Dialog {
        id: errorDialog
        title: "Error"
        modal: true
        anchors.centerIn: parent
        width: 300

        property string errorMessage: ""

        Label {
            text: errorDialog.errorMessage
            color: "white"
            wrapMode: Text.Wrap
            width: parent.width - 40
        }

        standardButtons: Dialog.Ok
    }

    Connections {
        target: ctController
        function onError(message) {
            errorDialog.errorMessage = message
            errorDialog.open()
        }
        function onPhantomLoadedChanged() {
            if (ctController.phantomLoaded) {
                phantomImage.source = "image://ct/phantom"
            }
        }
        function onSinogramReady() {
            sinogramImage.source = "image://ct/sinogram"
        }
        function onReconstructionUpdated(iteration) {
            reconstructionImage.source = "image://ct/reconstruction"
        }
        function onReconstructionFinished() {
            reconstructionImage.source = "image://ct/reconstruction"
        }
    }

    // 🎨 reusable button style
    Component {
        id: styledButton

        Button {
            property string backendName: ""
            text: backendName

            background: Rectangle {
                radius: 8
                color: "#3a86ff"
            }

            contentItem: Text {
                text: parent.text
                color: "white"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#0f172a"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            // 🔵 LEFT SIDEBAR
            Rectangle {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                radius: 12
                color: "#1e293b"
                objectName: "sidebarPanel"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Label {
                        text: "Phantom Selection"
                        objectName: "phantomLabel"
                        color: "white"
                    }

                    ComboBox {
                        id: phantomCombo
                        objectName: "phantomComboBox"
                        model: ["Shepp-Logan"]
                        onActivated: ctController.generateSheppLogan()
                    }

                    Loader {
                        sourceComponent: styledButton
                        onLoaded: {
                            item.backendName = "Upload Phantom"
                            item.objectName = "uploadPhantomButton"
                            item.onClicked.connect(function() { phantomFileDialog.open() })
                        }
                    }

                    Label { text: "Acquisition"; color: "white" }
                    Label { text: "Algorithm"; color: "white" }
                    Label { text: "Reconstruction"; color: "white" }

                    Item { Layout.fillHeight: true }

                    Loader {
                        sourceComponent: styledButton
                        onLoaded: {
                            item.backendName = "Load Session"
                            item.objectName = "loadSessionButton"
                        }
                    }

                    Loader {
                        sourceComponent: styledButton
                        onLoaded: {
                            item.backendName = "Save Session"
                            item.objectName = "saveSessionButton"
                        }
                    }
                }
            }

            // 🟣 CENTER
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 12
                    color: "#020617"
                    objectName: "reconstructionView"

                    Label {
                        text: "Reconstruction View"
                        objectName: "reconstructionLabel"
                        color: "white"
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.margins: 10
                    }

                    Image {
                        id: phantomImage
                        anchors.centerIn: parent
                        width: parent.width * 0.9
                        height: parent.height * 0.9
                        fillMode: Image.PreserveAspectFit
                        visible: ctController.phantomLoaded
                    }

                    Image {
                        id: reconstructionImage
                        anchors.centerIn: parent
                        width: parent.width * 0.9
                        height: parent.height * 0.9
                        fillMode: Image.PreserveAspectFit
                        visible: !phantomImage.visible
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 140
                    radius: 12
                    color: "#020617"
                    objectName: "sinogramView"

                    Label {
                        text: "Sinogram View"
                        objectName: "sinogramLabel"
                        color: "white"
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
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    spacing: 12

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: "#1e293b"
                        objectName: "progressPanel"

                        Column {
                            anchors.centerIn: parent
                            spacing: 6

                            Text {
                                text: "Reconstruction Progress"
                                objectName: "progressLabel"
                                color: "white"
                            }

                            ProgressBar {
                                id: reconstructionProgressBar
                                objectName: "progressBar"
                                width: 200
                                value: ctController.progress
                                visible: ctController.reconstructionRunning
                            }
                        }
                    }

                    Rectangle {
                        width: 200
                        radius: 10
                        color: "#1e293b"
                        objectName: "actionPanel"

                        Column {
                            anchors.centerIn: parent
                            spacing: 6

                            Loader {
                                sourceComponent: styledButton
                                onLoaded: {
                                    item.backendName = "Start Reconstruction"
                                    item.objectName = "startButton"
                                    item.enabled = Qt.binding(function() { return ctController.phantomLoaded && !ctController.reconstructionRunning })
                                    item.onClicked.connect(function() { ctController.startReconstruction() })
                                }
                            }

                            Loader {
                                sourceComponent: styledButton
                                onLoaded: {
                                    item.backendName = "Stop Reconstruction"
                                    item.objectName = "stopButton"
                                    item.enabled = Qt.binding(function() { return ctController.reconstructionRunning })
                                    item.onClicked.connect(function() { ctController.stopReconstruction() })
                                }
                            }
                        }
                    }
                }
            }

            // 🟠 RIGHT PANEL
            Rectangle {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                radius: 12
                color: "#1e293b"
                objectName: "rightPanel"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Label {
                        text: "Acquisition Controls"
                        objectName: "acquisitionControlsLabel"
                        color: "white"
                    }

                    Label {
                        text: "Projections: " + projectionsSlider.value.toFixed(0)
                        color: "white"
                    }
                    Slider {
                        id: projectionsSlider
                        objectName: "projectionsSlider"
                        from: 1; to: 720; value: 180
                    }

                    Label {
                        text: "Voltage: " + voltageSlider.value.toFixed(0) + " kV"
                        color: "white"
                    }
                    Slider {
                        id: voltageSlider
                        objectName: "voltageSlider"
                        from: 80; to: 150; value: 120
                    }

                    Label {
                        text: "Current: " + currentSlider.value.toFixed(0) + " mA"
                        color: "white"
                    }
                    Slider {
                        id: currentSlider
                        objectName: "currentSlider"
                        from: 10; to: 300; value: 100
                    }

                    Loader {
                        sourceComponent: styledButton
                        onLoaded: {
                            item.backendName = "Generate Sinogram"
                            item.objectName = "generateSinogramButton"
                            item.enabled = Qt.binding(function() { return ctController.phantomLoaded && !ctController.reconstructionRunning })
                            item.onClicked.connect(function() {
                                ctController.generateSinogram(projectionsSlider.value, voltageSlider.value, currentSlider.value)
                            })
                        }
                    }

                    Label {
                        text: "Algorithm Selection"
                        objectName: "algorithmLabel"
                        color: "white"
                    }

                    ComboBox {
                        id: algorithmCombo
                        objectName: "algorithmComboBox"
                        model: ["ART", "SIRT"]
                        currentIndex: model.indexOf(ctController.currentAlgorithm)
                        onCurrentTextChanged: ctController.currentAlgorithm = currentText
                    }

                    Item { Layout.fillHeight: true }

                    Loader {
                        sourceComponent: styledButton
                        onLoaded: {
                            item.backendName = "Export Image"
                            item.objectName = "exportImageButton"
                            item.onClicked.connect(function() { exportFileDialog.open() })
                        }
                    }

                    Loader {
                        sourceComponent: styledButton
                        onLoaded: {
                            item.backendName = "Export DICOM"
                            item.objectName = "exportDicomButton"
                            item.onClicked.connect(function() { ctController.exportDICOM("") })  // Will show error dialog
                        }
                    }
                }
            }
        }
    }
}