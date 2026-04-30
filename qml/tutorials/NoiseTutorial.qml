import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property color accent;  property color accent2
    property color card;    property color border
    property color textPri; property color textSec; property color textMuted; property color bg

    readonly property var phantom: [
        { cx:0.50, cy:0.50, rx:0.35, ry:0.28, density: 1.0  },
        { cx:0.50, cy:0.51, rx:0.26, ry:0.20, density:-0.98 },
        { cx:0.36, cy:0.42, rx:0.11, ry:0.10, density: 0.80 },
        { cx:0.64, cy:0.42, rx:0.11, ry:0.10, density: 0.80 },
        { cx:0.50, cy:0.35, rx:0.05, ry:0.05, density: 0.70 }
    ]

    readonly property int numAngles: 100
    readonly property int numBins:   64
    readonly property int dimN:      64

    property real noiseSigma:  0.0
    property real limitedAngle: 180.0
    property var cleanImage:   []
    property var noisyImage:   []

    function gauss() { var u=1-Math.random(),v=Math.random(); return Math.sqrt(-2*Math.log(u))*Math.cos(2*Math.PI*v) }

    function backproject(angCount, noiseSig) {
        var N = dimN, img = []
        for (var i=0;i<N*N;i++) img.push(0)
        var steps=130
        for (var a=0;a<angCount;a++){
            var deg=a*180.0/numAngles, rad=deg*Math.PI/180
            var cosA=Math.cos(rad), sinA=Math.sin(rad)
            for(var b=0;b<numBins;b++){
                var t=(b/(numBins-1))-0.5
                var sum=0, x0=0.5-sinA*t-cosA*0.5, y0=0.5+cosA*t-sinA*0.5
                for(var s=0;s<steps;s++){
                    var px=x0+cosA*s/steps, py=y0+sinA*s/steps
                    for(var k=0;k<phantom.length;k++){
                        var dd=(px-phantom[k].cx)*(px-phantom[k].cx)/(phantom[k].rx*phantom[k].rx)
                               +(py-phantom[k].cy)*(py-phantom[k].cy)/(phantom[k].ry*phantom[k].ry)
                        if(dd<1) sum+=phantom[k].density/steps
                    }
                }
                var val=Math.max(0,Math.min(1,sum*2.8))
                if(noiseSig>0) val=Math.max(0,Math.min(1,val+gauss()*noiseSig))
                var bp2=Math.round((t+0.5)*(numBins-1))
                // backproject this sample
                for(var yi=0;yi<N;yi++) for(var xi=0;xi<N;xi++){
                    var tt=(xi/N-0.5)*cosA+(yi/N-0.5)*sinA
                    var b2=Math.round((tt+0.5)*(numBins-1))
                    if(b2===b) img[yi*N+xi]+=val
                }
            }
        }
        var mx=0; for(var j=0;j<img.length;j++) if(img[j]>mx) mx=img[j]
        if(mx>0) for(var k2=0;k2<img.length;k2++) img[k2]/=mx
        return img
    }

    // Faster: per-angle full row backprojection
    function backprojectFast(angCount, noiseSig) {
        var N = dimN, img = []
        for (var i=0;i<N*N;i++) img.push(0)
        for (var a=0;a<angCount;a++){
            var deg=a*180.0/numAngles, rad=deg*Math.PI/180
            var cosA=Math.cos(rad), sinA=Math.sin(rad)
            // Build projection row
            var row=[]
            var steps=120
            for(var b=0;b<numBins;b++){
                var t=(b/(numBins-1))-0.5
                var sum=0, x0=0.5-sinA*t-cosA*0.5, y0=0.5+cosA*t-sinA*0.5
                for(var s=0;s<steps;s++){
                    var px=x0+cosA*s/steps, py=y0+sinA*s/steps
                    for(var k=0;k<phantom.length;k++){
                        var dd=(px-phantom[k].cx)*(px-phantom[k].cx)/(phantom[k].rx*phantom[k].rx)
                               +(py-phantom[k].cy)*(py-phantom[k].cy)/(phantom[k].ry*phantom[k].ry)
                        if(dd<1) sum+=phantom[k].density/steps
                    }
                }
                var val=Math.max(0,Math.min(1,sum*2.8))
                if(noiseSig>0) val=Math.max(0,Math.min(1,val+gauss()*noiseSig))
                row.push(val)
            }
            // Backproject row into image
            for(var yi=0;yi<N;yi++) for(var xi=0;xi<N;xi++){
                var t2=(xi/N-0.5)*cosA+(yi/N-0.5)*sinA
                var b2=Math.round((t2+0.5)*(numBins-1))
                if(b2>=0&&b2<numBins) img[yi*N+xi]+=row[b2]
            }
        }
        var mx=0; for(var j=0;j<img.length;j++) if(img[j]>mx) mx=img[j]
        if(mx>0) for(var k2=0;k2<img.length;k2++) img[k2]/=mx
        return img
    }

    function rmse(a,b){
        if(!a||!b||a.length===0) return 0
        var s=0; for(var i=0;i<a.length;i++){var d=a[i]-b[i];s+=d*d}
        return Math.sqrt(s/a.length)
    }

    function drawGray(ctx,img,W,H){
        if(!img||img.length===0) return
        var N=dimN, cw=W/N, ch=H/N
        for(var yi=0;yi<N;yi++) for(var xi=0;xi<N;xi++){
            var v=Math.round(img[yi*N+xi]*255)
            ctx.fillStyle="rgb("+v+","+v+","+v+")"; ctx.fillRect(xi*cw,yi*ch,cw+1,ch+1)
        }
    }

    function recompute() {
        var angCount = Math.round(numAngles * (limitedAngle / 180.0))
        if (angCount < 1) angCount = 1
        noisyImage = backprojectFast(angCount, noiseSigma)
        noisyCanvas.requestPaint()
    }

    Component.onCompleted: {
        cleanImage = backprojectFast(numAngles, 0)
        recompute()
    }

    Flickable {
        anchors.fill: parent; contentWidth: width; contentHeight: mainCol.implicitHeight+56; clip: true

        ColumnLayout {
            id: mainCol; x: 28; y: 28; width: parent.width-56; spacing: 14

            Text { text: "Noise & Artifacts in CT"; color: root.textPri; font.pixelSize: 26; font.bold: true }
            Text {
                text: "Two major quality-degrading factors: statistical noise from low X-ray dose, and streak artifacts from insufficient scan angle. Drag the sliders and compare against the clean reference."
                color: root.textSec; font.pixelSize: 14; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.4
            }

            // Two image panels
            RowLayout { Layout.fillWidth: true; height: 340; spacing: 14

                // Clean reference
                Rectangle {
                    Layout.fillWidth: true; height: 340
                    color: root.card; radius: 14; border.color: "#34d399"; border.width: 1; clip: true

                    Rectangle { width: parent.width; height: 40; z:2; color: Qt.rgba(52/255,211/255,153/255,0.12)
                        Text { anchors.centerIn: parent; text: "✅  Clean Reference (180°, no noise)"; color: "#34d399"; font.pixelSize: 13; font.bold: true } }

                    Canvas {
                        anchors.fill: parent; anchors.topMargin: 40
                        onPaint: { var ctx=getContext("2d"); ctx.fillStyle="#0b1120"; ctx.fillRect(0,0,width,height); root.drawGray(ctx,root.cleanImage,width,height) }
                        Component.onCompleted: requestPaint()
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom; width: parent.width; height: 34; color: Qt.rgba(0,0,0,0.5)
                        Text { anchors.centerIn: parent; text: "RMSE: 0.000 (reference)"; color: "#34d399"; font.pixelSize: 12; font.bold: true }
                    }
                }

                // Degraded image
                Rectangle {
                    Layout.fillWidth: true; height: 340
                    color: root.card; radius: 14; border.color: "#ef4444"; border.width: 1; clip: true

                    Rectangle { width: parent.width; height: 40; z:2; color: Qt.rgba(239/255,68/255,68/255,0.12)
                        Text { anchors.centerIn: parent; text: "⚠️  Degraded Scan (your parameters)"; color: "#ef4444"; font.pixelSize: 13; font.bold: true } }

                    Canvas {
                        id: noisyCanvas
                        anchors.fill: parent; anchors.topMargin: 40
                        onPaint: { var ctx=getContext("2d"); ctx.fillStyle="#0b1120"; ctx.fillRect(0,0,width,height); root.drawGray(ctx,root.noisyImage,width,height) }
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom; width: parent.width; height: 34; color: Qt.rgba(0,0,0,0.5)
                        property string rv: root.cleanImage.length>0&&root.noisyImage.length>0
                                            ? root.rmse(root.cleanImage,root.noisyImage).toFixed(4) : "—"
                        Text { anchors.centerIn: parent; text: "RMSE vs Reference: "+parent.rv; color: "#ef4444"; font.pixelSize: 12; font.bold: true }
                    }
                }
            }

            // ── Controls panel ───────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                color: root.card; radius: 14; border.color: root.border; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 18; spacing: 16

                    Text { text: "🎛️  Artifact Controls"; color: root.accent; font.pixelSize: 14; font.bold: true }

                    // Dose Noise
                    ColumnLayout { Layout.fillWidth: true; spacing: 6
                        RowLayout {
                            Text { text: "☢️  Dose Noise (σ):"; color: root.textSec; font.pixelSize: 13 }
                            Text { text: root.noiseSigma.toFixed(2); color: "#ef4444"; font.pixelSize: 13; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 160; height: 26; radius: 6
                                color: root.noiseSigma < 0.05 ? Qt.rgba(52/255,211/255,153/255,0.15)
                                                              : root.noiseSigma < 0.2  ? Qt.rgba(251/255,191/255,36/255,0.15)
                                                                                       : Qt.rgba(239/255,68/255,68/255,0.15)
                                border.color: root.noiseSigma < 0.05 ? "#34d399" : root.noiseSigma < 0.2 ? "#fbbf24" : "#ef4444"
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: root.noiseSigma < 0.05 ? "Low noise" : root.noiseSigma < 0.2 ? "Moderate noise" : "High noise"
                                    color: root.noiseSigma < 0.05 ? "#34d399" : root.noiseSigma < 0.2 ? "#fbbf24" : "#ef4444"
                                    font.pixelSize: 11; font.bold: true
                                }
                            }
                        }
                        Slider {
                            id: noiseSlider; Layout.fillWidth: true; from: 0; to: 0.5; value: 0
                            onValueChanged: { root.noiseSigma = value; root.recompute() }
                            background: Rectangle { x:noiseSlider.leftPadding;y:noiseSlider.topPadding+noiseSlider.availableHeight/2-height/2;width:noiseSlider.availableWidth;height:6;radius:3;color:"#334155";Rectangle{width:noiseSlider.visualPosition*parent.width;height:parent.height;radius:3;color:"#ef4444"} }
                            handle: Rectangle { x:noiseSlider.leftPadding+noiseSlider.visualPosition*(noiseSlider.availableWidth-width);y:noiseSlider.topPadding+noiseSlider.availableHeight/2-height/2;width:18;height:18;radius:9;color:"#ef4444";border.color:"#fff";border.width:2 }
                        }
                        Text { text: "Physics: Lower X-ray dose → fewer photons → higher quantum noise in projection data."; color: root.textMuted; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    }

                    // Scan Angle
                    ColumnLayout { Layout.fillWidth: true; spacing: 6
                        RowLayout {
                            Text { text: "🔄  Scan Angle:"; color: root.textSec; font.pixelSize: 13 }
                            Text { text: Math.round(root.limitedAngle)+"°"; color: root.accent; font.pixelSize: 13; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 160; height: 26; radius: 6
                                color: root.limitedAngle>=150 ? Qt.rgba(52/255,211/255,153/255,0.15)
                                                              : root.limitedAngle>=90 ? Qt.rgba(251/255,191/255,36/255,0.15)
                                                                                     : Qt.rgba(239/255,68/255,68/255,0.15)
                                border.color: root.limitedAngle>=150 ? "#34d399" : root.limitedAngle>=90 ? "#fbbf24" : "#ef4444"
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: root.limitedAngle>=150 ? "Full scan" : root.limitedAngle>=90 ? "Limited angle" : "Severe artifact"
                                    color: root.limitedAngle>=150 ? "#34d399" : root.limitedAngle>=90 ? "#fbbf24" : "#ef4444"
                                    font.pixelSize: 11; font.bold: true
                                }
                            }
                        }
                        Slider {
                            id: angleSlider; Layout.fillWidth: true; from: 30; to: 180; value: 180
                            onValueChanged: { root.limitedAngle = value; root.recompute() }
                            background: Rectangle { x:angleSlider.leftPadding;y:angleSlider.topPadding+angleSlider.availableHeight/2-height/2;width:angleSlider.availableWidth;height:6;radius:3;color:"#334155";Rectangle{width:angleSlider.visualPosition*parent.width;height:parent.height;radius:3;color:root.accent} }
                            handle: Rectangle { x:angleSlider.leftPadding+angleSlider.visualPosition*(angleSlider.availableWidth-width);y:angleSlider.topPadding+angleSlider.availableHeight/2-height/2;width:18;height:18;radius:9;color:root.accent;border.color:"#fff";border.width:2 }
                        }
                        Text { text: "Physics: Missing projection angles → directional streak artifacts. Reduce below 90° for dramatic example."; color: root.textMuted; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    }
                }
            }

            Item { height: 4 }
        }
    }
}
