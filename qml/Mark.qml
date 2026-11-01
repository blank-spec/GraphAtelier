import QtQuick
import QtQuick.Controls

CheckBox {
    id: control
    spacing: 9
    font.family: "Manrope"
    font.pixelSize: 11
    padding: 6

    indicator: Rectangle {
        x: control.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        width: 18
        height: 18
        radius: 4
        color: control.checked ? Theme.selected : Theme.surface
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? Theme.accent : Theme.secondary

        LineIcon {
            anchors.centerIn: parent
            kind: "check"
            ink: Theme.currentStep
            visible: control.checked
            scale: 0.8
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: Theme.secondary
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
    }
}
