import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

ApplicationWindow {
    visible: true
    width: 2000
    height: 900
    Component.onCompleted: {
            console.log("Window size:", width, height)
        }

    MainView { }   // 👈 ده اللي فيه التصميم
}