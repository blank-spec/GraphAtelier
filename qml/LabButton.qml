import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: control
    property string symbol: ""
    property bool primary: false
    property bool dark: false
    property bool compact: false
    property bool selected: false
    readonly property color foreground: primary ? Theme.accentText : Theme.text

    implicitHeight: 42
    implicitWidth: contentItem.implicitWidth + (compact ? 22 : 28)
    hoverEnabled: true
    font.family: "Manrope"
    font.pixelSize: 12
    font.weight: Font.DemiBold
    opacity: enabled ? 1 : 0.38
    Accessible.name: text.length ? text : symbol

    function backgroundColor() {
        if (primary) {
            if (down) {
                return Theme.accentPressed;
            }

            return hovered ? Theme.accentHover : Theme.accent;
        }

        if (selected) {
            return Theme.selected;
        }

        if (down || hovered) {
            return Theme.hover;
        }

        return dark ? Theme.raised : Theme.surface;
    }

    contentItem: RowLayout {
        spacing: 8
        LineIcon {
            visible: control.symbol.length > 0
            kind: control.symbol
            ink: control.foreground
            Layout.alignment: Qt.AlignVCenter
        }
        Text {
            visible: control.text.length > 0
            text: control.text
            font: control.font
            color: control.foreground
            Layout.alignment: Qt.AlignVCenter
        }
    }

    background: Rectangle {
        radius: 8
        color: control.backgroundColor()
        border.width: control.visualFocus ? 2 : 1
        border.color: {
            if (control.visualFocus || control.primary) {
                return Theme.accent;
            }

            return control.selected ? Theme.currentStep : Theme.border;
        }
        Behavior on color {
            ColorAnimation {
                duration: 100
            }
        }
    }
}
