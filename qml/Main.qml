import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Atelier 1.0

ApplicationWindow {
    id: window
    width: 1440
    height: 920
    minimumWidth: 1120
    minimumHeight: 760
    visible: true
    title: "Graph Atelier — вариант " + lab.variant
    color: Theme.window
    font.family: "Manrope"
    palette.window: Theme.window
    palette.base: Theme.surface
    palette.button: Theme.raised
    palette.text: Theme.text
    palette.windowText: Theme.text
    palette.buttonText: Theme.text
    palette.highlight: Theme.selected
    palette.highlightedText: Theme.text
    palette.mid: Theme.border
    property bool playing: false
    property bool motion: true
    property bool allEdges: false
    property bool weights: true
    property int intervalMs: 550
    readonly property var emptyCell: ({
                                          value: 0,
                                          fresh: false,
                                          original: false
                                      })
    readonly property var reachability: lab.cells
    readonly property bool graphFullscreen: visibility === Window.FullScreen
    property int windowedVisibility: Window.Windowed

    function toggleGraphFullscreen() {
        if (graphFullscreen) {
            if (windowedVisibility === Window.Maximized) {
                showMaximized();
            } else {
                showNormal();
            }
        } else {
            windowedVisibility = visibility;
            showFullScreen();
        }
        fullscreenAction.forceActiveFocus(Qt.ShortcutFocusReason);
    }

    function pause() {
        playing = false;
    }
    function vertices(count) {
        return Array.from({
                              length: count
                          }, (_, i) => "v" + (i + 1));
    }

    Connections {
        target: lab
        function onChanged() {
            if (lab.step === lab.stepCount - 1) {
                window.playing = false;
            }
        }
        function onStatusChanged() {
            notice.open();
        }
    }

    Timer {
        interval: window.intervalMs
        running: window.playing
        repeat: true
        onTriggered: lab.next()
    }

    Shortcut {
        sequence: "F11"
        onActivated: window.toggleGraphFullscreen()
    }
    Shortcut {
        sequence: "Escape"
        enabled: window.graphFullscreen
        onActivated: window.toggleGraphFullscreen()
    }
    Shortcut {
        sequence: "Space"
        onActivated: playAction.clicked()
    }
    Shortcut {
        sequence: "Right"
        onActivated: {
            pause();
            lab.next();
        }
    }
    Shortcut {
        sequence: "Left"
        onActivated: {
            pause();
            lab.previous();
        }
    }
    Shortcut {
        sequence: "Home"
        onActivated: {
            pause();
            lab.reset();
        }
    }
    Shortcut {
        sequence: "End"
        onActivated: {
            pause();
            lab.finish();
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Сохранить результаты варианта " + lab.variant
        onAccepted: lab.exportResults(selectedFolder)
    }

    Popup {
        id: notice
        x: (window.width - width) / 2
        y: window.height - height - 28
        width: Math.min(740, window.width - 80)
        padding: 18
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        contentItem: Text {
            text: lab.status
            color: Theme.text
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
        background: Rectangle {
            color: Theme.raised
            radius: 10
        }
        Timer {
            interval: 7000
            running: notice.opened
            onTriggered: notice.close()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: window.graphFullscreen ? 16 : 28
        spacing: 16

        RowLayout {
            visible: !window.graphFullscreen
            Layout.fillWidth: true
            spacing: 14

            Rectangle {
                width: 42
                height: 42
                radius: 12
                color: Theme.selected
                LineIcon {
                    anchors.centerIn: parent
                    kind: "graph"
                    ink: Theme.currentStep
                    scale: 1.4
                }
            }
            ColumnLayout {
                spacing: 1
                Text {
                    text: "Graph Atelier"
                    color: Theme.text
                    font.pixelSize: 23
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Лаборатория маршрутов и связей"
                    color: Theme.muted
                    font.pixelSize: 11
                }
            }
            Item {
                Layout.fillWidth: true
            }
            Text {
                text: "Вариант"
                color: Theme.secondary
                font.pixelSize: 12
            }
            Choice {
                objectName: "variantChoice"
                model: Array.from({
                                      length: 20
                                  }, (_, i) => (i + 1).toString().padStart(2, "0"))
                currentIndex: lab.variant - 1
                onActivated: {
                    pause();
                    lab.variant = currentIndex + 1;
                }
                Accessible.name: "Номер варианта"
                implicitWidth: 76
            }
            LabButton {
                text: "Экспорт"
                symbol: "export"
                primary: true
                onClicked: {
                    pause();
                    folderDialog.open();
                }
            }
        }

        Rectangle {
            visible: !window.graphFullscreen
            Layout.fillWidth: true
            height: 1
            color: Theme.divider
        }

        RowLayout {
            visible: !window.graphFullscreen
            Layout.fillWidth: true
            spacing: 22

            Text {
                text: lab.mode === 0 ? "Найти кратчайший путь." : "Увидеть каждую связь."
                color: Theme.text
                font.pixelSize: window.width < 1250 ? 26 : 30
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 4
                LabButton {
                    text: "Дейкстра"
                    selected: lab.mode === 0
                    onClicked: {
                        pause();
                        lab.mode = 0;
                    }
                }
                LabButton {
                    text: "Уоршелл"
                    selected: lab.mode === 1
                    onClicked: {
                        pause();
                        lab.mode = 1;
                    }
                }
            }
        }

        RowLayout {
            visible: !window.graphFullscreen
            Layout.fillWidth: true
            spacing: 14

            Text {
                text: lab.mode === 0 ? "От" : "Замыкание"
                font.pixelSize: 12
                color: Theme.secondary
            }
            Choice {
                objectName: "sourceChoice"
                visible: lab.mode === 0
                model: window.vertices(15)
                currentIndex: lab.source
                onActivated: {
                    pause();
                    lab.source = currentIndex;
                }
                Accessible.name: "Начальная вершина"
            }
            Choice {
                visible: lab.mode === 1
                implicitWidth: 205
                model: ["A+ · пути длины ≥ 1", "A* · включая длину 0"]
                currentIndex: lab.reflexive ? 1 : 0
                onActivated: {
                    pause();
                    lab.reflexive = currentIndex === 1;
                }
                Accessible.name: "Режим транзитивного замыкания"
            }
            Text {
                visible: lab.mode === 0
                text: "до"
                font.pixelSize: 12
                color: Theme.secondary
            }
            Choice {
                visible: lab.mode === 0
                model: window.vertices(15)
                currentIndex: lab.target
                onActivated: lab.target = currentIndex
                Accessible.name: "Конечная вершина"
            }
            Item {
                Layout.fillWidth: true
            }
            Text {
                text: lab.nodeCount + " вершин  /  " + lab.edgeCount + " дуг  /  " + (lab.mode === 0 ? "веса > 0" :
                                                                                                       "орграф")
                color: Theme.secondary
                font.pixelSize: 12
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 22

            Rectangle {
                id: graphPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 520
                Layout.preferredWidth: 850
                color: Theme.canvas
                radius: 16
                border.color: Theme.divider
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: lab.mode === 0 ? (window.allEdges ? "Взвешенный орграф" : "Дерево кратчайших путей") :
                                                   "Исходный орграф"

                            color: Theme.text
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        LabButton {
                            visible: lab.mode === 0
                            text: window.allEdges ? "Скрыть фоновые дуги" : "Все " + lab.edgeCount + " дуг"
                            dark: true
                            compact: true
                            implicitHeight: 34
                            onClicked: window.allEdges = !window.allEdges
                        }
                        LabButton {
                            id: fullscreenAction
                            objectName: "fullscreenAction"
                            text: window.graphFullscreen ? "Выйти · Esc" : ""
                            symbol: window.graphFullscreen ? "restore" : "fullscreen"
                            dark: true
                            compact: true
                            implicitHeight: 34
                            onClicked: window.toggleGraphFullscreen()
                            Accessible.name: window.graphFullscreen ? "Выйти из полноэкранного режима" :
                                                                      "Развернуть граф на весь экран"
                            ToolTip.visible: hovered
                            ToolTip.text: window.graphFullscreen ? "Выйти из полноэкранного режима (Esc / F11)" :
                                                                   "Граф на весь экран (F11)"
                        }
                        LabButton {
                            symbol: "reset"
                            dark: true
                            compact: true
                            implicitHeight: 34
                            onClicked: graph.resetLayout()
                            Accessible.name: "Восстановить расположение вершин"
                            ToolTip.visible: hovered
                            ToolTip.text: "Восстановить расположение"
                        }
                    }

                    GraphView {
                        id: graph
                        engine: lab
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        showAll: window.allEdges
                        showWeights: window.weights
                        Accessible.name:
                            "Интерактивный граф. Нажмите на вершину для выбора цели; перетащите для перемещения."
                        NumberAnimation on phase {
                            from: 0.001
                            to: 0.999
                            duration: 1500
                            loops: Animation.Infinite
                            running: window.playing && window.motion && lab.mode === 0 && window.visibility
                                     !== Window.Minimized && window.visible
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Rectangle {
                            width: 7
                            height: 7
                            radius: 3.5
                            color: Theme.accent
                        }
                        Text {
                            text: lab.mode === 0 ? "Выбранный маршрут" : "Выбранная вершина"
                            color: Theme.secondary
                            font.pixelSize: 10
                        }
                        Rectangle {
                            width: 7
                            height: 7
                            radius: 3.5
                            color: Theme.currentStep
                            Layout.leftMargin: 12
                        }
                        Text {
                            text: "Текущий шаг"
                            color: Theme.secondary
                            font.pixelSize: 10
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        Text {
                            text: "Вершины можно двигать"
                            color: Theme.muted
                            font.pixelSize: 10
                        }
                    }
                }
            }

            ScrollView {
                id: detailsScroll
                padding: 16
                background: Rectangle {
                    color: Theme.surface
                    radius: 16
                    border.color: Theme.divider
                }
                visible: !window.graphFullscreen
                implicitHeight: 480
                Layout.preferredWidth: window.width < 1250 ? 345 : 390
                Layout.maximumWidth: window.width < 1250 ? 345 : 390
                Layout.minimumWidth: window.width < 1250 ? 345 : 390
                Layout.fillWidth: false
                Layout.fillHeight: true
                Layout.minimumHeight: 0
                clip: true
                contentWidth: availableWidth
                contentHeight: detailContent.height
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: contentHeight > availableHeight ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded

                ColumnLayout {
                    id: detailContent
                    width: detailsScroll.availableWidth
                    height: Math.max(detailsScroll.availableHeight, lab.mode === 1 ? 480 : 400)
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: lab.mode === 0 ? "Дистанции от v" + (lab.source + 1) : "Матрица достижимости"
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                            color: Theme.text
                            Layout.fillWidth: true
                        }
                        Text {
                            text: lab.mode === 0 ? "O(V²)" : "O(V³)"
                            font.family: "Consolas"
                            font.pixelSize: 11
                            color: Theme.muted
                        }
                    }

                    Text {
                        text: lab.mode === 0 ? "Нажмите на строку, чтобы подсветить маршрут." :
                                               "Строка — откуда. Столбец — куда."
                        color: Theme.muted
                        font.pixelSize: 11
                    }

                    ListView {
                        id: distanceList
                        objectName: "distanceList"
                        visible: lab.mode === 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 0
                        model: lab.rows
                        reuseItems: true
                        ScrollBar.vertical: ScrollBar {
                            id: distanceScrollBar
                            policy: distanceList.contentHeight > distanceList.height ? ScrollBar.AlwaysOn :
                                                                                       ScrollBar.AsNeeded
                        }

                        delegate: ItemDelegate {
                            id: distanceRow
                            objectName: "distanceRow" + modelData.vertex
                            required property var modelData
                            required property int index
                            width: distanceList.width - (distanceScrollBar.visible ? distanceScrollBar.width + 6 : 0)
                            height: 24
                            padding: 0
                            leftPadding: 8
                            rightPadding: 8
                            topInset: 0
                            bottomInset: 0
                            leftInset: 0
                            rightInset: 0
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            onActiveFocusChanged: {
                                if (activeFocus) {
                                    distanceList.positionViewAtIndex(index, ListView.Contain);
                                }
                            }
                            onClicked: lab.target = modelData.vertex - 1
                            Accessible.name: "Вершина " + modelData.vertex + ", дистанция " + modelData.distance
                            background: Rectangle {
                                anchors.fill: parent
                                anchors.margins: 1
                                radius: 4
                                antialiasing: true
                                border.width: distanceRow.visualFocus ? 1 : 0
                                border.color: Theme.accent
                                color: distanceRow.modelData.vertex - 1 === lab.target ? Theme.routeSelection :
                                                                                         distanceRow.hovered
                                                                                         ? Theme.hover :
                                                                                           distanceRow.modelData.active
                                                                                           ? Theme.selected :
                                                                                             "transparent"
                            }
                            contentItem: RowLayout {
                                spacing: 9
                                Rectangle {
                                    width: 6
                                    height: 6
                                    radius: 3
                                    color: distanceRow.modelData.fixed ? Theme.currentStep : Theme.border
                                }
                                Text {
                                    text: "v" + distanceRow.modelData.vertex
                                    color: Theme.text
                                    font.pixelSize: 11
                                    Layout.preferredWidth: 30
                                }
                                Text {
                                    text: distanceRow.modelData.path
                                    color: Theme.secondary
                                    font.pixelSize: 10
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: distanceRow.modelData.distance
                                    color: Theme.text
                                    font.family: "Consolas"
                                    font.pixelSize: 13
                                    font.bold: true
                                    Layout.preferredWidth: 28
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }
                    }

                    Item {
                        visible: lab.mode === 1
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        property real cellSize: Math.min(width / 11, (height - 68) / 11)

                        Grid {
                            id: reachGrid
                            columns: 11
                            spacing: 2
                            anchors.top: parent.top
                            anchors.horizontalCenter: parent.horizontalCenter
                            property real cellSize: Math.max(22, parent.cellSize - 2)

                            Repeater {
                                id: matrixCells
                                model: 121
                                delegate: Rectangle {
                                    required property int index
                                    readonly property int row: Math.floor(index / 11) - 1
                                    readonly property int col: index % 11 - 1
                                    readonly property bool header: row < 0 || col < 0
                                    readonly property var cell: header ? null : (window.reachability[row * 10 + col]
                                                                                 || window.emptyCell)
                                    activeFocusOnTab: !header
                                    Accessible.role: Accessible.StaticText
                                    Accessible.name: header ? "Заголовок матрицы" : "Из v" + (row + 1) + " в v" + (col
                                                                                                                   + 1) + ": "
                                                              + (cell.value ? "достижима" : "недостижима")
                                    Accessible.description: !header && cell.fresh ? "Связь добавлена на текущем шаге" :
                                                                                    ""
                                    Keys.onShortcutOverride: function (event) {
                                        if (event.key === Qt.Key_Left || event.key === Qt.Key_Right) {
                                            event.accepted = true;
                                        }
                                    }
                                    Keys.onRightPressed: {
                                        if (col < 9) {
                                            matrixCells.itemAt(index + 1).forceActiveFocus();
                                        }
                                    }
                                    Keys.onLeftPressed: {
                                        if (col > 0) {
                                            matrixCells.itemAt(index - 1).forceActiveFocus();
                                        }
                                    }
                                    Keys.onDownPressed: {
                                        if (row < 9) {
                                            matrixCells.itemAt(index + 11).forceActiveFocus();
                                        }
                                    }
                                    Keys.onUpPressed: {
                                        if (row > 0) {
                                            matrixCells.itemAt(index - 11).forceActiveFocus();
                                        }
                                    }
                                    width: reachGrid.cellSize
                                    height: reachGrid.cellSize
                                    radius: header ? 0 : 4
                                    color: header ? "transparent" : cell.fresh ? Theme.currentStep : cell.value ? (
                                                                                                                      cell.original
                                                                                                                      ? Theme.matrixOriginal :
                                                                                                                        Theme.matrixFound) :
                                                                                                                  Theme.matrixEmpty
                                    border.width: activeFocus ? 2 : !header && (row === lab.step - 1 || col
                                                                                === lab.step - 1) ? 1 : 0
                                    border.color: Theme.accent
                                    Behavior on color {
                                        enabled: window.motion
                                        ColorAnimation {
                                            duration: 220
                                        }
                                    }
                                    Text {
                                        anchors.centerIn: parent
                                        text: parent.header ? (parent.row < 0 && parent.col < 0 ? "" : (parent.row < 0
                                                                                                        ? parent.col
                                                                                                          + 1 : parent.row
                                                                                                          + 1)) : parent.cell.value
                                        color: parent.header || !parent.cell.value || (parent.cell.original &&
                                                                                       !parent.cell.fresh) ? Theme.text :
                                                                                                             Theme.accentText
                                        font.family: "Consolas"
                                        font.pixelSize: 11
                                    }
                                    HoverHandler {
                                        id: matrixHover
                                    }
                                    ToolTip.visible: matrixHover.hovered && !header
                                    ToolTip.text: "v" + (row + 1) + " → v" + (col + 1) + ": " + (cell ? (cell.value
                                                                                                         ? "достижима" :
                                                                                                           "недостижима") :
                                                                                                        "")
                                }
                            }
                        }
                        Text {
                            anchors.top: reachGrid.bottom
                            anchors.topMargin: 16
                            width: parent.width
                            text: "Синие — исходные связи. Светлые — найденные. Фиолетовые — добавлены на этом шаге."
                            wrapMode: Text.Wrap
                            color: Theme.secondary
                            font.pixelSize: 11
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.divider
                    }
                    Text {
                        text: lab.mode === 0 ? "К v" + (lab.target + 1) + " · длина " + lab.distanceText :
                                               "Достижимых пар: " + lab.distanceText + " из 100"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: Theme.text
                    }
                    Text {
                        Layout.fillWidth: true
                        text: lab.pathText
                        color: Theme.secondary
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        Layout.minimumHeight: 33
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 9
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Шаг " + lab.step + " / " + (lab.stepCount - 1)
                    color: Theme.text
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                Text {
                    text: lab.message
                    color: Theme.secondary
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                LabButton {
                    text: "К результату"
                    symbol: "next"
                    compact: true
                    implicitHeight: 34
                    onClicked: {
                        pause();
                        lab.finish();
                    }
                }
            }
            Slider {
                id: timeline
                Layout.fillWidth: true
                from: 0
                to: lab.stepCount - 1
                stepSize: 1
                value: lab.step
                onMoved: {
                    pause();
                    lab.step = value;
                }
                Accessible.name: "Шаг алгоритма"
                background: Rectangle {
                    x: timeline.leftPadding
                    y: timeline.topPadding + timeline.availableHeight / 2 - height / 2
                    implicitHeight: 4
                    width: timeline.availableWidth
                    height: 4
                    radius: 2
                    color: Theme.divider
                    Rectangle {
                        width: timeline.visualPosition * parent.width
                        height: parent.height
                        color: Theme.currentStep
                        radius: 2
                    }
                }
                handle: Rectangle {
                    x: timeline.leftPadding + timeline.visualPosition * (timeline.availableWidth - width)
                    y: timeline.topPadding + timeline.availableHeight / 2 - height / 2
                    width: 16
                    height: 16
                    radius: 8
                    color: Theme.text
                    border.width: timeline.activeFocus ? 3 : 2
                    border.color: Theme.currentStep
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                LabButton {
                    symbol: "reset"
                    compact: true
                    onClicked: {
                        pause();
                        lab.reset();
                    }
                    Accessible.name: "Начать заново"
                }
                LabButton {
                    symbol: "back"
                    compact: true
                    enabled: lab.step > 0
                    onClicked: {
                        pause();
                        lab.previous();
                    }
                    Accessible.name: "Предыдущий шаг"
                }
                LabButton {
                    id: playAction
                    text: window.playing ? "Пауза" : "Воспроизвести"
                    symbol: window.playing ? "pause" : "play"
                    primary: true
                    onClicked: {
                        if (lab.step === lab.stepCount - 1) {
                            lab.reset();
                        }
                        window.playing = !window.playing;
                    }
                }
                LabButton {
                    symbol: "next"
                    compact: true
                    enabled: lab.step < lab.stepCount - 1
                    onClicked: {
                        pause();
                        lab.next();
                    }
                    Accessible.name: "Следующий шаг"
                }
                Text {
                    text: "Скорость"
                    color: Theme.secondary
                    font.pixelSize: 11
                    Layout.leftMargin: 12
                }
                Choice {
                    implicitWidth: 82
                    model: ["0.5×", "1×", "2×", "4×"]
                    currentIndex: 1
                    onActivated: window.intervalMs = [1100, 550, 275, 138][currentIndex]
                    Accessible.name: "Скорость воспроизведения"
                }
                Item {
                    Layout.fillWidth: true
                }
                Mark {
                    text: "Анимации"
                    checked: window.motion
                    onToggled: window.motion = checked
                    font.pixelSize: 11
                }
                Mark {
                    visible: lab.mode === 0
                    text: "Веса дуг"
                    checked: window.weights
                    onToggled: window.weights = checked
                    font.pixelSize: 11
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: (window.graphFullscreen ? (lab.mode === 0 ? "Дейкстра · " : "Уоршелл · ") : "Данные из Excel · ")
                      + "вариант " + lab.variant
                color: Theme.muted
                font.pixelSize: 10
            }
            Item {
                Layout.fillWidth: true
            }
            Text {
                text: "Пробел — пуск / пауза     ← → — по шагам     Home / End — начало / итог     " + (
                          window.graphFullscreen ? "Esc / F11 — выйти" : "F11 — на весь экран")
                color: Theme.muted
                font.pixelSize: 10
            }
        }
    }
}
