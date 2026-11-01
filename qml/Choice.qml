import QtQuick
import QtQuick.Controls

ComboBox {
    id: control
    implicitWidth: 95
    implicitHeight: 40
    font.family: "Manrope"
    font.pixelSize: 12
    leftPadding: 12
    rightPadding: 30

    contentItem: Text {
        text: control.displayText
        font: control.font
        color: Theme.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: LineIcon {
        kind: "chevron"
        x: control.width - width - 8
        anchors.verticalCenter: parent.verticalCenter
    }

    background: Rectangle {
        color: control.hovered ? Theme.hover : Theme.surface
        radius: 7
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? Theme.accent : Theme.border
    }

    delegate: ItemDelegate {
        id: option
        required property var modelData
        required property int index
        readonly property bool chosen: control.currentIndex === index
        width: control.popup.availableWidth
        height: 36
        hoverEnabled: true
        text: String(modelData)
        highlighted: control.highlightedIndex === index
        font: control.font

        contentItem: Text {
            text: option.text
            font: option.font
            color: option.chosen ? Theme.text : Theme.text
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }

        background: Rectangle {
            radius: 4
            color: option.chosen ? Theme.selected : option.hovered || option.highlighted ? Theme.hover : Theme.surface
        }
    }

    popup: Popup {
        y: control.height + 4
        width: control.width
        padding: 4
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 300)
        onOpened: optionList.positionViewAtIndex(control.currentIndex, ListView.Center)
        contentItem: ListView {
            id: optionList
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {
                contentItem: Rectangle {
                    implicitWidth: 3
                    radius: 1.5
                    color: Theme.secondary
                }
            }
        }
        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: 8
        }
    }
}
