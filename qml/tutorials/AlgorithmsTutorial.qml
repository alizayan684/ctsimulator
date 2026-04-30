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
        { cx:0.50, cy:0.35, rx:0.05, ry:0.05, density: 0.70 },
        { cx:0.50, cy:0.67, rx:0.05, ry:0.05, density: 0.70 }
    ]

    readonly property int numAngles: 120
    readonly property int numBins:   64
    readonly property int dimN:      64

    property var fbpImage:     []
    property var sirtImage:    []
    property var groundTruth:  []

    function computeProjection(angleDeg, bins) {
        var rad = angleDeg * Math.PI / 180
        var cosA = Math.cos(rad), sinA = Math.sin(rad)
        var row = [], steps = 140
        for (var b = 0; b < bins; b++) {
            var t = (b / (bins-1)) - 0.5
            var sum = 0
            var x0 = 0.5 - sinA*t - cosA*0.5, y0 = 0.5 + cosA*t - sinA*0.5
            for (var s = 0; s < steps; s++) {
                var px = x0+cosA*s/steps, py = y0+sinA*s/steps
                for (var k = 0; k < phantom.length; k++) {
                    var dd=(px-phantom[k].cx)*(px-phantom[k].cx)/(phantom[k].rx*phantom[k].rx)
                           +(py-phantom[k].cy)*(py-phantom[k].cy)/(phantom[k].ry*phantom[k].ry)
                    if (dd<1) sum+=phantom[k].density/steps
                }
            }
            row.push(Math.max(0,Math.min(1,sum*2.8)))
        }
        return row
    }

    function computeGT() {
        var N = dimN, img = []
        for (var yi=0;yi<N;yi++) for(var xi=0;xi<N;xi++){
            var px=xi/N, py=yi/N, v=0
            for (var k=0;k<phantom.length;k++){
                var dd=(px-phantom[k].cx)*(px-phantom[k].cx)/(phantom[k].rx*phantom[k].rx)
                       +(py-phantom[k].cy)*(py-phantom[k].cy)/(phantom[k].ry*phantom[k].ry)
                if (dd<1) v+=phantom[k].density
            }
            img.push(Math.max(0,Math.min(1,v)))
        }
        return img
    }

    function computeFBP() {
        var N = dimN, img = []
        for (var i=0;i<N*N;i++) img.push(0)
        for (var a=0;a<numAngles;a++){
            var deg=a*180.0/numAngles, rad=deg*Math.PI/180
            var cosA=Math.cos(rad), sinA=Math.sin(rad)
            var row=computeProjection(deg, numBins)
            for (var yi=0;yi<N;yi++) for(var xi=0;xi<N;xi++){
                var t=(xi/N-0.5)*cosA+(yi/N-0.5)*sinA
                var b=Math.round((t+0.5)*(numBins-1))
                if (b>=0&&b<numBins) img[yi*N+xi]+=row[b]
            }
        }
        var mx=0; for(var j=0;j<img.length;j++) if(img[j]>mx) mx=img[j]
        if(mx>0) for(var k2=0;k2<img.length;k2++) img[k2]/=mx
        return img
    }

    function computeSIRT(iters) {
        var N = dimN, img = []
        for (var i=0;i<N*N;i++) img.push(0)
        var lr = 0.018
        for (var it=0;it<iters;it++){
            for(var a=0;a<numAngles;a++){
                var deg=a*180.0/numAngles, rad=deg*Math.PI/180
                var cosA=Math.cos(rad), sinA=Math.sin(rad)
                var ref=computeProjection(deg, numBins)
                for(var b=0;b<numBins;b++){
                    var t=(b/(numBins-1))-0.5
                    var steps=60, x0=0.5-sinA*t-cosA*0.5, y0=0.5+cosA*t-sinA*0.5
                    var fp=0, hits=[]
                    for(var s=0;s<steps;s++){
                        var px=x0+cosA*s/steps, py=y0+sinA*s/steps
                        var xi2=Math.floor(px*N), yi2=Math.floor(py*N)
                        if(xi2>=0&&xi2<N&&yi2>=0&&yi2<N){ fp+=img[yi2*N+xi2]/steps; hits.push(yi2*N+xi2) }
                    }
                    var err=(ref[b]-fp)
                    for(var h=0;h<hits.length;h++) img[hits[h]]+=lr*err/Math.max(1,hits.length)
                }
            }
        }
        var mx2=0; for(var j2=0;j2<img.length;j2++) if(img[j2]>mx2) mx2=img[j2]
        if(mx2>0) for(var k3=0;k3<img.length;k3++) img[k3]=Math.max(0,img[k3]/mx2)
        return img
    }

    function rmse(a,b){ if(!a||!b||a.length===0) return 0; var s=0; for(var i=0;i<a.length;i++){var d=a[i]-b[i];s+=d*d} return Math.sqrt(s/a.length) }

    function drawGray(ctx,img,W,H){
        if(!img||img.length===0) return
        var N=dimN, cw=W/N, ch=H/N
        for(var yi=0;yi<N;yi++) for(var xi=0;xi<N;xi++){
            var v=Math.round(img[yi*N+xi]*255); ctx.fillStyle="rgb("+v+","+v+","+v+")"; ctx.fillRect(xi*cw,yi*ch,cw+1,ch+1)
        }
    }

    Component.onCompleted: {
        groundTruth = computeGT()
        fbpImage    = computeFBP()
        sirtImage   = computeSIRT(1)
    }

    Flickable {
        anchors.fill: parent; contentWidth: width; contentHeight: mainCol.implicitHeight+56; clip: true

        ColumnLayout {
            id: mainCol; x: 28; y: 28; width: parent.width-56; spacing: 14

            Text { text: "Reconstruction Algorithms Compared"; color: root.textPri; font.pixelSize: 26; font.bold: true }
            Text {
                text: "Same sinogram — different math. FBP is fast but produces streak artifacts; SIRT converges iteratively to a cleaner result. Use the slider to see SIRT improve over iterations."
                color: root.textSec; font.pixelSize: 14; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.4
            }

            // Three image panels
            RowLayout { Layout.fillWidth: true; height: 360; spacing: 14

                // Ground Truth
                Rectangle {
                    Layout.fillWidth: true; height: 360
                    color: root.card; radius: 14; border.color: root.accent2; border.width: 1; clip: true

                    Rectangle { width: parent.width; height: 40; z:2; color: Qt.rgba(129/255,140/255,248/255,0.12)
                        Text { anchors.centerIn: parent; text: "✅  Ground Truth Phantom"; color: root.accent2; font.pixelSize: 13; font.bold: true } }

                    Canvas {
                        anchors.fill: parent; anchors.topMargin: 40
                        onPaint: { var ctx=getContext("2d"); ctx.fillStyle="#0b1120"; ctx.fillRect(0,0,width,height); root.drawGray(ctx,root.groundTruth,width,height) }
                        Component.onCompleted: requestPaint()
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom; width: parent.width; height: 34; color: Qt.rgba(0,0,0,0.45)
                        Text { anchors.centerIn: parent; text: "Ideal output — the actual phantom"; color: root.textMuted; font.pixelSize: 12 }
                    }
                }

                // FBP
                Rectangle {
                    Layout.fillWidth: true; height: 360
                    color: root.card; radius: 14; border.color: "#fbbf24"; border.width: 1; clip: true

                    Rectangle { width: parent.width; height: 40; z:2; color: Qt.rgba(251/255,191/255,36/255,0.12)
                        Text { anchors.centerIn: parent; text: "⚡  FBP — Filtered Back Projection"; color: "#fbbf24"; font.pixelSize: 13; font.bold: true } }

                    Canvas {
                        anchors.fill: parent; anchors.topMargin: 40
                        onPaint: { var ctx=getContext("2d"); ctx.fillStyle="#0b1120"; ctx.fillRect(0,0,width,height); root.drawGray(ctx,root.fbpImage,width,height) }
                        Component.onCompleted: requestPaint()
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom; width: parent.width; height: 34; color: Qt.rgba(0,0,0,0.45)
                        property string rv: root.groundTruth.length>0&&root.fbpImage.length>0 ? root.rmse(root.groundTruth,root.fbpImage).toFixed(4) : "—"
                        Text { anchors.centerIn: parent; text: "RMSE: "+parent.rv; color: "#fbbf24"; font.pixelSize: 12; font.bold: true }
                    }
                }

                // SIRT
                Rectangle {
                    Layout.fillWidth: true; height: 360
                    color: root.card; radius: 14; border.color: root.accent; border.width: 1; clip: true

                    Rectangle { width: parent.width; height: 40; z:2; color: Qt.rgba(56/255,189/255,248/255,0.12)
                        Text { anchors.centerIn: parent; text: "🔁  SIRT — Iterations: "+iterSlider.value; color: root.accent; font.pixelSize: 13; font.bold: true } }

                    Canvas {
                        id: sirtCanvas
                        anchors.fill: parent; anchors.topMargin: 40
                        onPaint: { var ctx=getContext("2d"); ctx.fillStyle="#0b1120"; ctx.fillRect(0,0,width,height); root.drawGray(ctx,root.sirtImage,width,height) }
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom; width: parent.width; height: 34; color: Qt.rgba(0,0,0,0.45)
                        property string rv: root.groundTruth.length>0&&root.sirtImage.length>0 ? root.rmse(root.groundTruth,root.sirtImage).toFixed(4) : "—"
                        Text { anchors.centerIn: parent; text: "RMSE: "+parent.rv; color: root.accent; font.pixelSize: 12; font.bold: true }
                    }
                }
            }

            // Iteration slider
            Rectangle {
                Layout.fillWidth: true; height: 76
                color: root.card; radius: 12; border.color: root.border; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 14; spacing: 8

                    RowLayout {
                        Text { text: "SIRT Iterations:"; color: root.textSec; font.pixelSize: 13 }
                        Slider {
                            id: iterSlider; Layout.fillWidth: true; from: 1; to: 30; stepSize: 1; value: 1
                            onValueChanged: { root.sirtImage=root.computeSIRT(Math.floor(value)); sirtCanvas.requestPaint() }
                            background: Rectangle { x:iterSlider.leftPadding;y:iterSlider.topPadding+iterSlider.availableHeight/2-height/2;width:iterSlider.availableWidth;height:6;radius:3;color:"#334155";Rectangle{width:iterSlider.visualPosition*parent.width;height:parent.height;radius:3;color:root.accent} }
                            handle: Rectangle { x:iterSlider.leftPadding+iterSlider.visualPosition*(iterSlider.availableWidth-width);y:iterSlider.topPadding+iterSlider.availableHeight/2-height/2;width:18;height:18;radius:9;color:root.accent;border.color:"#fff";border.width:2 }
                        }
                        Text { text: Math.floor(iterSlider.value); color: root.accent; font.pixelSize: 14; font.bold: true; Layout.preferredWidth: 28 }
                    }
                    Text {
                        text: "💡 Each iteration of SIRT corrects errors from the previous step. Watch RMSE fall as the image sharpens — but note diminishing returns after ~15 iterations."
                        color: root.textMuted; font.pixelSize: 12; wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                }
            }

            Item { height: 4 }
        }
    }
}
