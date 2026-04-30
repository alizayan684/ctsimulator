import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property color accent;  property color accent2
    property color card;    property color border
    property color textPri; property color textSec; property color textMuted; property color bg

    property real gantryAngle: 0
    NumberAnimation on gantryAngle {
        from: 0; to: 360; duration: 4000; loops: Animation.Infinite; running: true; easing.type: Easing.Linear
    }

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

            // ── Header ────────────────────────────────────────────────────
            Text { text: "Introduction to CT Imaging"; color: root.textPri; font.pixelSize: 26; font.bold: true }
            Text {
                text: "Computed Tomography uses rotating X-rays to produce cross-sectional images. Each rotation angle captures a 1-D projection — hundreds combined reconstruct a 2-D image."
                color: root.textSec; font.pixelSize: 14; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.4
            }

            // ── Main area ─────────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                height: 430
                spacing: 14

                // Gantry animation
                Rectangle {
                    Layout.fillWidth: true; height: 430
                    color: root.card; radius: 14; border.color: root.border; border.width: 1; clip: true

                    // Card header strip
                    Rectangle {
                        width: parent.width; height: 40; z: 2
                        color: Qt.rgba(56/255,189/255,248/255,0.1)
                        Text { anchors.centerIn: parent; text: "🔄  Live Gantry Animation"; color: root.accent; font.pixelSize: 13; font.bold: true }
                    }

                    Canvas {
                        anchors.fill: parent
                        anchors.topMargin: 40
                        property real angle: root.gantryAngle
                        onAngleChanged: requestPaint()

                        onPaint: {
                            var ctx = getContext("2d")
                            var W = width, H = height, cx = W/2, cy = H/2
                            var R = Math.min(W,H) * 0.40
                            var rad = angle * Math.PI / 180
                            var beamHalf = 22 * Math.PI / 180

                            ctx.clearRect(0,0,W,H)
                            ctx.fillStyle = "#0b1120"; ctx.fillRect(0,0,W,H)

                            // Grid
                            ctx.strokeStyle = "rgba(45,63,87,0.5)"; ctx.lineWidth = 1
                            for (var gx = 0; gx < W; gx += 32) { ctx.beginPath(); ctx.moveTo(gx,0); ctx.lineTo(gx,H); ctx.stroke() }
                            for (var gy = 0; gy < H; gy += 32) { ctx.beginPath(); ctx.moveTo(0,gy); ctx.lineTo(W,gy); ctx.stroke() }

                            // Gantry ring
                            ctx.strokeStyle = "#2d3f57"; ctx.lineWidth = 3
                            ctx.setLineDash([8,6]); ctx.beginPath(); ctx.arc(cx,cy,R,0,2*Math.PI); ctx.stroke(); ctx.setLineDash([])

                            // Patient phantom
                            // Skull
                            ctx.beginPath(); ctx.arc(cx,cy,R*0.34,0,2*Math.PI); ctx.fillStyle="#475569"; ctx.fill()
                            // Brain
                            ctx.beginPath(); ctx.arc(cx,cy,R*0.27,0,2*Math.PI); ctx.fillStyle="#334155"; ctx.fill()
                            // Ventricles
                            ctx.beginPath(); ctx.ellipse(cx-12,cy-7,24,14); ctx.fillStyle="#1e3a5f"; ctx.fill()
                            // Lesion
                            var lg = ctx.createRadialGradient(cx+R*0.12,cy-R*0.08,0,cx+R*0.12,cy-R*0.08,R*0.07)
                            lg.addColorStop(0,"#f472b6"); lg.addColorStop(1,"rgba(244,114,182,0)")
                            ctx.beginPath(); ctx.arc(cx+R*0.12,cy-R*0.08,R*0.07,0,2*Math.PI); ctx.fillStyle=lg; ctx.fill()

                            // Fan beam
                            var srcX = cx + Math.cos(rad)*R, srcY = cy + Math.sin(rad)*R
                            var a1 = rad+Math.PI-beamHalf, a2 = rad+Math.PI+beamHalf
                            ctx.beginPath(); ctx.moveTo(srcX,srcY); ctx.arc(cx,cy,R*1.85,a1,a2); ctx.closePath()
                            var bg2 = ctx.createRadialGradient(srcX,srcY,0,srcX,srcY,R*1.85)
                            bg2.addColorStop(0,"rgba(234,179,8,0.30)"); bg2.addColorStop(1,"rgba(234,179,8,0.0)")
                            ctx.fillStyle = bg2; ctx.fill()

                            // Source dot
                            var sg = ctx.createRadialGradient(srcX,srcY,0,srcX,srcY,10)
                            sg.addColorStop(0,"#fde68a"); sg.addColorStop(1,"#eab308")
                            ctx.beginPath(); ctx.arc(srcX,srcY,10,0,2*Math.PI); ctx.fillStyle=sg; ctx.fill()

                            // Detector arc
                            ctx.strokeStyle="#38bdf8"; ctx.lineWidth=10; ctx.lineCap="round"
                            ctx.beginPath(); ctx.arc(cx,cy,R,rad+Math.PI-beamHalf*1.1,rad+Math.PI+beamHalf*1.1); ctx.stroke()

                            // Labels
                            ctx.font="bold 11px sans-serif"
                            ctx.fillStyle="#fde68a"; ctx.fillText("X-ray Source",srcX+13,srcY-10)
                            var dLx=cx+Math.cos(rad+Math.PI)*R+Math.cos(rad)*24
                            var dLy=cy+Math.sin(rad+Math.PI)*R+Math.sin(rad)*24
                            ctx.fillStyle="#38bdf8"; ctx.fillText("Detector",dLx,dLy)
                        }
                    }
                }

                // Step cards
                ColumnLayout {
                    Layout.preferredWidth: 340; height: 430; spacing: 10

                    Repeater {
                        model: [
                            { step:"Step 1", title:"X-rays Are Fired",          body:"A fan-beam fires through the patient. Dense tissue (bone) absorbs more than soft tissue, producing contrast.",           color:"#eab308", icon:"⚡" },
                            { step:"Step 2", title:"Detector Measures Signal",   body:"Opposite detectors measure how much radiation passed through — this 1D measurement is one projection.",                    color:"#38bdf8", icon:"📡" },
                            { step:"Step 3", title:"Gantry Rotates 360°",        body:"The source–detector pair rotates, sampling projections every fraction of a degree from all sides of the patient.",         color:"#818cf8", icon:"🔄" },
                            { step:"Step 4", title:"Algorithm Reconstructs",     body:"FBP or iterative algorithms mathematically combine hundreds of projections into a cross-sectional 2D image.",             color:"#34d399", icon:"🖥️" }
                        ]
                        delegate: Rectangle {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            color: root.card; radius: 10
                            border.color: modelData.color; border.width: 1

                            RowLayout {
                                anchors.fill: parent; anchors.margins: 12; spacing: 12

                                Rectangle {
                                    width: 42; height: 42; radius: 21
                                    color: Qt.rgba(
                                        parseInt(modelData.color.slice(1,3),16)/255,
                                        parseInt(modelData.color.slice(3,5),16)/255,
                                        parseInt(modelData.color.slice(5,7),16)/255, 0.18)
                                    Text { anchors.centerIn: parent; text: modelData.icon; font.pixelSize: 18 }
                                }
                                ColumnLayout { Layout.fillWidth: true; spacing: 3
                                    Text { text: modelData.step+" — "+modelData.title; color: modelData.color; font.pixelSize: 12; font.bold: true }
                                    Text { text: modelData.body; color: root.textSec; font.pixelSize: 12; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.3 }
                                }
                            }
                        }
                    }
                }
            }

            // ── Key Facts ─────────────────────────────────────────────────
            RowLayout { Layout.fillWidth: true; spacing: 10
                Repeater {
                    model: [
                        { label:"Scan Time",   value:"0.3 – 2 s",    sub:"per rotation",    color:"#38bdf8" },
                        { label:"Resolution",  value:"0.5 – 1 mm",   sub:"isotropic voxel", color:"#818cf8" },
                        { label:"Projections", value:"600 – 3000",    sub:"per revolution",  color:"#34d399" },
                        { label:"HU Range",    value:"-1000 to +3000",sub:"Hounsfield units",color:"#eab308" }
                    ]
                    delegate: Rectangle {
                        Layout.fillWidth: true; height: 72
                        color: root.card; radius: 10; border.color: modelData.color; border.width: 1
                        ColumnLayout { anchors.centerIn: parent; spacing: 2
                            Text { text: modelData.value;  color: modelData.color; font.pixelSize: 16; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                            Text { text: modelData.label;  color: root.textPri;    font.pixelSize: 11; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                            Text { text: modelData.sub;    color: root.textMuted;  font.pixelSize: 10; Layout.alignment: Qt.AlignHCenter }
                        }
                    }
                }
            }

            Item { height: 4 }
        }
    }
}
