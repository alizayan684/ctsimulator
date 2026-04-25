import QtQuick 2.15
import QtQuick.Layouts 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    objectName: "mainView"
    anchors.fill: parent

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
                    }

                    Loader {
                        sourceComponent: styledButton
                        onLoaded: {
                            item.backendName = "Upload Phantom"
                            item.objectName = "uploadPhantomButton"
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
                        anchors.margins: 10
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
                        anchors.margins: 10
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
                                value: 0.7
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
                                }
                            }

                            Loader {
                                sourceComponent: styledButton
                                onLoaded: {
                                    item.backendName = "Stop Reconstruction"
                                    item.objectName = "stopButton"
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

                    Slider {
                        id: projectionsSlider
                        objectName: "projectionsSlider"
                        from: 0; to: 720; value: 180
                    }

                    Slider {
                        id: voltageSlider
                        objectName: "voltageSlider"
                        from: 80; to: 150; value: 120
                    }

                    Slider {
                        id: currentSlider
                        objectName: "currentSlider"
                        from: 10; to: 300; value: 100
                    }

                    Label {
                        text: "Algorithm Selection"
                        objectName: "algorithmLabel"
                        color: "white"
                    }

                    ComboBox {
                        id: algorithmCombo
                        objectName: "algorithmComboBox"
                        model: ["FBP", "SIRT"]
                    }

                    Item { Layout.fillHeight: true }

                    Loader {
                        sourceComponent: styledButton
                        onLoaded: {
                            item.backendName = "Export Image"
                            item.objectName = "exportImageButton"
                        }
                    }

                    Loader {
                        sourceComponent: styledButton
                        onLoaded: {
                            item.backendName = "Export DICOM"
                            item.objectName = "exportDicomButton"
                        }
                    }
                }
            }
        }
    }
}