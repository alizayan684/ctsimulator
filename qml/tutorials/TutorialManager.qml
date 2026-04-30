import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CT.Visualization 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 820
    title: "CT Simulator — Educational Hub"

    // ─── Design Tokens ────────────────────────────────────────────────────────
    readonly property color bg:        "#0b1120"
    readonly property color surface:   "#111827"
    readonly property color card:      "#1e293b"
    readonly property color border:    "#2d3f57"
    readonly property color accent:    "#38bdf8"
    readonly property color accent2:   "#818cf8"
    readonly property color success:   "#34d399"
    readonly property color textPri:   "#f1f5f9"
    readonly property color textSec:   "#94a3b8"
    readonly property color textMuted: "#475569"

    // Sidebar entries
    readonly property var pages: [
        { icon: "①", label: "Introduction to CT" },
        { icon: "②", label: "How Projections Work" },
        { icon: "③", label: "Algorithms Compared" },
        { icon: "④", label: "Noise & Artifacts" },
        { icon: "⑤", label: "Parameter Optimization" }
    ]

    property int currentPage: 0

    background: Rectangle { color: root.bg }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── Sidebar ──────────────────────────────────────────────────────────
        Rectangle {
            Layout.preferredWidth: 270
            Layout.fillHeight: true
            color: root.surface

            Rectangle {
                width: 1; height: parent.height
                anchors.right: parent.right
                color: root.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 0

                // Logo / Title
                Item {
                    Layout.fillWidth: true
                    height: 80

                    Rectangle {
                        anchors.fill: parent
                        color: Qt.rgba(56/255, 189/255, 248/255, 0.07)
                    }
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2
                        Text {
                            text: "CT Simulator"
                            color: root.accent
                            font.pixelSize: 18
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Text {
                            text: "Educational Hub"
                            color: root.textMuted
                            font.pixelSize: 12
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Nav items
                Repeater {
                    model: root.pages
                    delegate: Item {
                        Layout.fillWidth: true
                        height: 58

                        Rectangle {
                            anchors.fill: parent
                            color: currentPage === index ? Qt.rgba(56/255,189/255,248/255,0.12) : "transparent"
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }
                        Rectangle {
                            width: 3; height: parent.height
                            anchors.left: parent.left
                            color: currentPage === index ? root.accent : "transparent"
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 22
                            anchors.rightMargin: 12
                            spacing: 14
                            Text {
                                text: modelData.icon
                                color: currentPage === index ? root.accent : root.textMuted
                                font.pixelSize: 18
                                Behavior on color { ColorAnimation { duration: 150 } }
                            }
                            Text {
                                text: modelData.label
                                color: currentPage === index ? root.textPri : root.textSec
                                font.pixelSize: 14
                                font.weight: currentPage === index ? Font.SemiBold : Font.Normal
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                Behavior on color { ColorAnimation { duration: 150 } }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.currentPage = index
                            onEntered: if (currentPage !== index) parent.children[0].color = Qt.rgba(255,255,255,0.04)
                            onExited:  if (currentPage !== index) parent.children[0].color = "transparent"
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // Footer
                Rectangle {
                    Layout.fillWidth: true
                    height: 48
                    color: Qt.rgba(0,0,0,0.2)
                    Text {
                        anchors.centerIn: parent
                        text: "© Member 4 — Visualization & Tutorials"
                        color: root.textMuted
                        font.pixelSize: 11
                    }
                }
            }
        }

        // ── Page Area ─────────────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                anchors.fill: parent
                currentIndex: root.currentPage

                IntroductionTutorial  { accent: root.accent; accent2: root.accent2; card: root.card; border: root.border; textPri: root.textPri; textSec: root.textSec; textMuted: root.textMuted; bg: root.bg }
                ProjectionsTutorial   { accent: root.accent; accent2: root.accent2; card: root.card; border: root.border; textPri: root.textPri; textSec: root.textSec; textMuted: root.textMuted; bg: root.bg }
                AlgorithmsTutorial    { accent: root.accent; accent2: root.accent2; card: root.card; border: root.border; textPri: root.textPri; textSec: root.textSec; textMuted: root.textMuted; bg: root.bg }
                NoiseTutorial         { accent: root.accent; accent2: root.accent2; card: root.card; border: root.border; textPri: root.textPri; textSec: root.textSec; textMuted: root.textMuted; bg: root.bg }
                OptimizationTutorial  { accent: root.accent; accent2: root.accent2; card: root.card; border: root.border; textPri: root.textPri; textSec: root.textSec; textMuted: root.textMuted; bg: root.bg }
            }
        }
    }
}
