import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property color accent;  property color accent2
    property color card;    property color border
    property color textPri; property color textSec; property color textMuted; property color bg

    // ── Phantom: Shepp-Logan with clearly off-center features ──────────────
    readonly property var phantom: [
        { cx:0.50, cy:0.50, rx:0.35, ry:0.28, density: 1.0  },   // skull
        { cx:0.50, cy:0.51, rx:0.26, ry:0.20, density:-0.98 },   // brain interior
        { cx:0.36, cy:0.42, rx:0.11, ry:0.10, density: 0.80 },   // left lobe  → big sinusoid
        { cx:0.64, cy:0.42, rx:0.11, ry:0.10, density: 0.80 },   // right lobe → big sinusoid
        { cx:0.50, cy:0.35, rx:0.05, ry:0.05, density: 0.70 },   // top feature
        { cx:0.50, cy:0.67, rx:0.05, ry:0.05, density: 0.70 }    // bottom feature
    ]

    readonly property int numAngles: 180
    readonly property int numBins:   80

    property int   anglesCollected: 0
    property bool  collecting:      false
    property var   sinogramData:    []

    // ── Compute one projection ─────────────────────────────────────────────
    function computeProjection(angleDeg) {
        var rad = angleDeg * Math.PI / 180
        var cosA = Math.cos(rad), sinA = Math.sin(rad)
        var row = [], steps = 160
        for (var b = 0; b < numBins; b++) {
            var t = (b / (numBins - 1)) - 0.5
            var sum = 0
            var x0 = 0.5 - sinA*t - cosA*0.5
            var y0 = 0.5 + cosA*t - sinA*0.5
            for (var s = 0; s < steps; s++) {
                var px = x0 + cosA*s/steps
                var py = y0 + sinA*s/steps
                for (var k = 0; k < phantom.length; k++) {
                    var dd = (px-phantom[k].cx)*(px-phantom[k].cx)/(phantom[k].rx*phantom[k].rx)
                           + (py-phantom[k].cy)*(py-phantom[k].cy)/(phantom[k].ry*phantom[k].ry)
                    if (dd < 1) sum += phantom[k].density / steps
                }
            }
            row.push(Math.max(0, Math.min(1, sum * 2.8)))
        }
        return row
    }

    Timer {
        id: collectTimer; interval: 20; repeat: true; running: root.collecting
        onTriggered: {
            if (root.anglesCollected >= root.numAngles) { root.collecting = false; return }
            var deg = root.anglesCollected * (180.0 / root.numAngles)
            var row = root.computeProjection(deg)
            var nd = root.sinogramData.slice(); nd.push(row); root.sinogramData = nd
            root.anglesCollected++
            phantomCanvas.requestPaint(); sinoCanvas.requestPaint()
        }
    }

    function startCollection() { root.sinogramData = []; root.anglesCollected = 0; root.collecting = true }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: mainCol.implicitHeight + 56
        clip: true

        ColumnLayout {
            id: mainCol
            x: 28; y: 28
            width: parent.width - 56
            spacing: 14

            // ── Header ──────────────────────────────────────────────────
            Text { text: "How Projections Work"; color: root.textPri; font.pixelSize: 26; font.bold: true }
            Text {
                text: "Each CT projection is the total X-ray attenuation along parallel lines at one angle. Press Start Scan and watch the sinogram build row-by-row in real time. Sinusoidal curves reveal off-center structures."
                color: root.textSec; font.pixelSize: 14; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.4
            }

            // ── Main panels ──────────────────────────────────────────────
            RowLayout { Layout.fillWidth: true; height: 400; spacing: 14

                // Phantom panel
                Rectangle {
                    Layout.fillWidth: true; height: 400
                    color: root.card; radius: 14; border.color: root.border; border.width: 1; clip: true

                    Rectangle {
                        width: parent.width; height: 40; z: 2
                        color: Qt.rgba(56/255,189/255,248/255,0.1)
                        Text { anchors.centerIn: parent; text: "🫀  Shepp-Logan Phantom + Active Ray"; color: root.accent; font.pixelSize: 13; font.bold: true }
                    }

                    Canvas {
                        id: phantomCanvas
                        anchors.fill: parent; anchors.topMargin: 40
                        property int collected: root.anglesCollected
                        onCollectedChanged: requestPaint()

                        onPaint: {
                            var ctx = getContext("2d")
                            var W = width, H = height
                            ctx.clearRect(0,0,W,H)
                            ctx.fillStyle = "#0b1120"; ctx.fillRect(0,0,W,H)

                            // Draw phantom
                            var ph = root.phantom
                            for (var k = ph.length-1; k >= 0; k--) {
                                var e = ph[k]
                                var absD = Math.abs(e.density)
                                var col = e.density > 0
                                    ? "rgba("+Math.round(148+absD*60)+","+Math.round(163+absD*40)+","+Math.round(184)+",1)"
                                    : "rgba(15,23,42,1)"
                                var ew = e.rx * W * 0.95, eh = e.ry * H * 0.95
                                ctx.save()
                                ctx.translate(e.cx*W, e.cy*H)
                                ctx.scale(1, eh/ew)
                                ctx.beginPath(); ctx.arc(0, 0, ew, 0, 2*Math.PI)
                                ctx.fillStyle = col; ctx.fill()
                                ctx.restore()
                            }

                            // Draw current ray bundle
                            if (root.anglesCollected > 0) {
                                var deg = (root.anglesCollected-1) * (180.0 / root.numAngles)
                                var rad = deg * Math.PI / 180
                                var cosA = Math.cos(rad), sinA = Math.sin(rad)
                                var nb = root.numBins

                                ctx.strokeStyle = "rgba(56,189,248,0.18)"; ctx.lineWidth = 1.5
                                for (var b = 0; b < nb; b += 2) {
                                    var t = (b/(nb-1)) - 0.5
                                    var x0 = (0.5 - sinA*t - cosA*0.5)*W, y0 = (0.5 + cosA*t - sinA*0.5)*H
                                    var x1 = (0.5 - sinA*t + cosA*0.5)*W, y1 = (0.5 + cosA*t + sinA*0.5)*H
                                    ctx.beginPath(); ctx.moveTo(x0,y0); ctx.lineTo(x1,y1); ctx.stroke()
                                }

                                // Central ray highlighted
                                var cx0 = (0.5 - cosA*0.5)*W, cy0 = (0.5 - sinA*0.5)*H
                                var cx1 = (0.5 + cosA*0.5)*W, cy1 = (0.5 + sinA*0.5)*H
                                ctx.strokeStyle="#38bdf8"; ctx.lineWidth=2.5
                                ctx.beginPath(); ctx.moveTo(cx0,cy0); ctx.lineTo(cx1,cy1); ctx.stroke()

                                // Angle label
                                ctx.fillStyle="#38bdf8"; ctx.font="bold 12px sans-serif"
                                ctx.fillText("θ = "+deg.toFixed(1)+"°  |  Proj "+root.anglesCollected+"/"+root.numAngles, 10, H-10)
                            }
                        }
                    }
                }

                // Sinogram panel  
                Rectangle {
                    Layout.fillWidth: true; height: 400
                    color: root.card; radius: 14; border.color: root.border; border.width: 1; clip: true

                    Rectangle {
                        width: parent.width; height: 40; z: 2
                        color: Qt.rgba(129/255,140/255,248/255,0.1)
                        Text { anchors.centerIn: parent; text: "📊  Sinogram (Angle × Bin) — Grayscale"; color: root.accent2; font.pixelSize: 13; font.bold: true }
                    }

                    Canvas {
                        id: sinoCanvas
                        anchors.fill: parent; anchors.topMargin: 40
                        property var data: root.sinogramData
                        onDataChanged: requestPaint()

                        onPaint: {
                            var ctx = getContext("2d")
                            var W = width, H = height
                            ctx.fillStyle="#0b1120"; ctx.fillRect(0,0,W,H)

                            var data = root.sinogramData
                            var nA = data.length
                            if (nA === 0) {
                                ctx.fillStyle = root.textMuted; ctx.font = "14px sans-serif"
                                ctx.textAlign = "center"
                                ctx.fillText("Press ▶ Start Scan to generate the sinogram", W/2, H/2)
                                ctx.textAlign = "left"
                                return
                            }

                            // Draw grayscale sinogram — rows = angles, cols = bins
                            var cellH = H / root.numAngles
                            var cellW = W / root.numBins
                            for (var a = 0; a < nA; a++) {
                                for (var b = 0; b < root.numBins; b++) {
                                    // Apply window/level: stretch contrast
                                    var v = data[a][b]
                                    var g = Math.round(Math.max(0, Math.min(1, v)) * 255)
                                    ctx.fillStyle = "rgb("+g+","+g+","+g+")"
                                    ctx.fillRect(b*cellW, a*cellH, cellW+1, cellH+1)
                                }
                            }

                            // Axis labels
                            ctx.fillStyle = "rgba(148,163,184,0.85)"; ctx.font = "bold 10px sans-serif"
                            ctx.textAlign = "center"
                            ctx.fillText("← Detector Bins ("+root.numBins+") →", W*0.5, H-6)
                            ctx.textAlign = "left"
                            ctx.save(); ctx.translate(14, H*0.5); ctx.rotate(-Math.PI/2)
                            ctx.textAlign = "center"; ctx.fillText("Angle  0°→180°", 0, 0); ctx.restore()
                        }
                    }
                }
            }

            // ── Progress bar + controls ──────────────────────────────────
            RowLayout { Layout.fillWidth: true; height: 42; spacing: 12

                Button {
                    implicitHeight: 42; implicitWidth: 150
                    text: root.collecting ? "⏹  Stop" : "▶  Start Scan"
                    onClicked: root.collecting ? (root.collecting = false) : root.startCollection()
                    background: Rectangle { color: root.collecting ? "#ef4444" : root.accent; radius: 8
                        Behavior on color { ColorAnimation { duration: 150 } } }
                    contentItem: Text { text: parent.text; color: "#fff"; font.bold: true; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                Button {
                    implicitHeight: 42; implicitWidth: 100; text: "↺  Reset"
                    onClicked: { root.collecting=false; root.sinogramData=[]; root.anglesCollected=0; sinoCanvas.requestPaint(); phantomCanvas.requestPaint() }
                    background: Rectangle { color: "#1e293b"; radius: 8; border.color: root.border; border.width: 1 }
                    contentItem: Text { text: parent.text; color: root.textSec; font.bold: true; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                Rectangle { Layout.fillWidth: true; height: 8; radius: 4; color: "#1e293b"
                    Rectangle {
                        width: parent.width * (root.anglesCollected / root.numAngles)
                        height: parent.height; radius: 4; color: root.accent
                        Behavior on width { NumberAnimation { duration: 80 } }
                    }
                }

                Text { text: root.anglesCollected+" / "+root.numAngles+" projections"; color: root.textSec; font.pixelSize: 13 }
            }

            // ── Insight tip ──────────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true; height: 50
                color: Qt.rgba(56/255,189/255,248/255,0.06); radius: 10
                border.color: Qt.rgba(56/255,189/255,248/255,0.2); border.width: 1
                RowLayout { anchors.fill: parent; anchors.margins: 12; spacing: 10
                    Text { text: "💡"; font.pixelSize: 18 }
                    Text {
                        text: "Each row in the sinogram = one projection. Off-center features (left/right lobes) trace sinusoidal curves across angles — a fundamental property that FBP exploits during reconstruction."
                        color: root.textSec; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.35
                    }
                }
            }

            Item { height: 4 }
        }
    }
}
