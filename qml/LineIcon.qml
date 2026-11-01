import QtQuick

Canvas {
    id: icon
    property string kind: "play"
    property color ink: Theme.text
    width: 18
    height: 18

    // Declarative drawing coordinates keep the icon family in one stroke system.
    readonly property var outlines: ({
                                         play: [[[5, 3], [14, 9], [5, 15], [5, 3]]],
                                         next: [[[4, 4], [11, 9], [4, 14]], [[14, 4], [14, 14]]],
                                         back: [[[4, 4], [11, 9], [4, 14]], [[14, 4], [14, 14]]],
                                         export: [[[9, 2], [9, 12]], [[5, 6], [9, 2], [13, 6]], [[3, 11], [3, 16], [15,
                                                                                                                    16], [15, 11]]],
                                         reset: [[[3, 2], [3, 6], [7, 6]]],
                                         fullscreen: [[[3, 7], [3, 3], [7, 3]], [[11, 3], [15, 3], [15, 7]], [[15, 11],
                                                                                                              [15, 15], [11,
                                                                                                                         15]], [[7,
                                                                                                                                 15], [3, 15],
                                                                                                                                [3, 11]]],
                                         restore: [[[3, 7], [7, 7], [7, 3]], [[11, 3], [11, 7], [15, 7]], [[15, 11], [11,
                                                                                                                      11], [11, 15]],
                                             [[7, 15], [7, 11], [3, 11]]],
                                         graph: [[[4, 12], [9, 4], [15, 12], [4, 12]]],
                                         check: [[[3, 9], [7, 13], [15, 5]]],
                                         chevron: [[[5, 7], [9, 11], [13, 7]]]
                                     })

    onKindChanged: requestPaint()
    onInkChanged: requestPaint()

    function drawSegments(context, segments) {
        context.beginPath();

        for (const segment of segments) {
            context.moveTo(segment[0][0], segment[0][1]);

            for (let index = 1; index < segment.length; ++index) {
                context.lineTo(segment[index][0], segment[index][1]);
            }
        }

        context.stroke();
    }

    function drawGraphVertices(context) {
        for (const point of [[4, 12], [9, 4], [15, 12]]) {
            context.beginPath();
            context.arc(point[0], point[1], 2.5, 0, Math.PI * 2);
            context.fill();
        }
    }

    onPaint: {
        const context = getContext("2d");
        context.reset();
        context.strokeStyle = ink;
        context.fillStyle = ink;
        context.lineWidth = 1.6;
        context.lineCap = "round";
        context.lineJoin = "round";

        if (kind === "pause") {
            context.fillRect(5, 4, 2.5, 10);
            context.fillRect(11, 4, 2.5, 10);
            return;
        }

        if (kind === "back") {
            context.translate(18, 0);
            context.scale(-1, 1);
        }

        drawSegments(context, outlines[kind] || []);

        if (kind === "reset") {
            context.beginPath();
            context.arc(9, 9, 6, -2.4, 2.6);
            context.stroke();
        }

        if (kind === "graph") {
            drawGraphVertices(context);
        }
    }
}
