import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    visible: true
    width: 2000
    height: 900
    Component.onCompleted: {
            console.log("Window size:", width, height)
        }

    MainView { }   // 👈 ده اللي فيه التصميم
}