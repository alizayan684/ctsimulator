import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true
    width: 1600
    height: 900
    title: "CT Simulator"
    color: "#0f172a"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: navBar
            Layout.fillWidth: true
            background: Rectangle { color: "#1e293b" }

            TabButton {
                text: "🔬  Simulator"
                font.bold: true
                width: implicitWidth
                background: Rectangle {
                    color: navBar.currentIndex === 0 ? "#334155" : "#1e293b"
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                contentItem: Text {
                    text: parent.text
                    color: navBar.currentIndex === 0 ? "#38bdf8" : "#94a3b8"
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            TabButton {
                text: "📚  Educational Hub"
                font.bold: true
                width: implicitWidth
                background: Rectangle {
                    color: navBar.currentIndex === 1 ? "#334155" : "#1e293b"
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                contentItem: Text {
                    text: parent.text
                    color: navBar.currentIndex === 1 ? "#818cf8" : "#94a3b8"
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: navBar.currentIndex

            MainView { }
            TutorialManager { }
        }
    }
}