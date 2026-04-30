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

    readonly property int dimN: 64

    property int   numAngles: 60
    property int   numBins:   32
    property int   numIters:  5
    property real  noiseSig:  0.0
    property real  rmseVal:   1.0
    property string badge:    ""

    property var groundTruth: []
    property var reconImage:  []

    function gauss() { var u=1-Math.random(),v=Math.random(); return Math.sqrt(-2*Math.log(u))*Math.cos(2*Math.PI*v) }

    function computeGT() {
        var N=dimN, img=[]
        for(var yi=0;yi<N;yi++) for(var xi=0;xi<N;xi++){
            var px=xi/N, py=yi/N, v=0
            for(var k=0;k<phantom.length;k++){
                var dd=(px-phantom[k].cx)*(px-phantom[k].cx)/(phantom[k].rx*phantom[k].rx)
                       +(py-phantom[k].cy)*(py-phantom[k].cy)/(phantom[k].ry*phantom[k].ry)
                if(dd<1) v+=phantom[k].density
            }
            img.push(Math.max(0,Math.min(1,v)))
        }
        return img
    }

    function runScan() {
        var N=dimN, img=[]
        for(var i=0;i<N*N;i++) img.push(0)
        var nA=numAngles, steps=110
        for(var a=0;a<nA;a++){
            var deg=a*180.0/nA, rad=deg*Math.PI/180
            var cosA=Math.cos(rad), sinA=Math.sin(rad)
            var row=[]
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
            for(var yi=0;yi<N;yi++) for(var xi=0;xi<N;xi++){
                var t2=(xi/N-0.5)*cosA+(yi/N-0.5)*sinA
                var b2=Math.round((t2+0.5)*(numBins-1))
                if(b2>=0&&b2<numBins) img[yi*N+xi]+=row[b2]
            }
        }
        // SIRT iterations
        var lr=0.016
        for(var it=0;it<numIters;it++){
            var corr=[]
            for(var i2=0;i2<N*N;i2++) corr.push(0)
            for(var a2=0;a2<nA;a2++){
                var deg2=a2*180.0/nA, rad2=deg2*Math.PI/180
                var cosA2=Math.cos(rad2), sinA2=Math.sin(rad2)
                for(var b3=0;b3<numBins;b3++){
                    var t3=(b3/(numBins-1))-0.5
                    var fp=0, hits=[], steps2=60
                    var x02=0.5-sinA2*t3-cosA2*0.5, y02=0.5+cosA2*t3-sinA2*0.5
                    for(var s2=0;s2<steps2;s2++){
                        var px3=x02+cosA2*s2/steps2, py3=y02+sinA2*s2/steps2
                        var xi3=Math.floor(px3*N), yi3=Math.floor(py3*N)
                        if(xi3>=0&&xi3<N&&yi3>=0&&yi3<N){ fp+=img[yi3*N+xi3]/steps2; hits.push(yi3*N+xi3) }
                    }
                    var ref=0, steps3=110, x03=0.5-sinA2*t3-cosA2*0.5, y03=0.5+cosA2*t3-sinA2*0.5
                    for(var s3=0;s3<steps3;s3++){
                        var px4=x03+cosA2*s3/steps3, py4=y03+sinA2*s3/steps3
                        for(var k4=0;k4<phantom.length;k4++){
                            var dd4=(px4-phantom[k4].cx)*(px4-phantom[k4].cx)/(phantom[k4].rx*phantom[k4].rx)
                                   +(py4-phantom[k4].cy)*(py4-phantom[k4].cy)/(phantom[k4].ry*phantom[k4].ry)
                            if(dd4<1) ref+=phantom[k4].density/steps3
                        }
                    }
                    ref=Math.max(0,Math.min(1,ref*2.8))
                    var err=ref-fp
                    for(var h=0;h<hits.length;h++) corr[hits[h]]+=lr*err/Math.max(1,hits.length)
                }
            }
            for(var i3=0;i3<N*N;i3++) img[i3]=Math.max(0,img[i3]+corr[i3])
        }
        var mx=0; for(var j=0;j<img.length;j++) if(img[j]>mx) mx=img[j]
        if(mx>0) for(var k5=0;k5<img.length;k5++) img[k5]/=mx
        reconImage=img
        var s2=0; for(var ii=0;ii<groundTruth.length;ii++){var d=groundTruth[ii]-img[ii];s2+=d*d}
        rmseVal=Math.sqrt(s2/groundTruth.length)
        if(rmseVal<0.06) badge="🏆 Expert — Excellent!"
        else if(rmseVal<0.12) badge="🥇 Great — Very good quality"
        else if(rmseVal<0.20) badge="🥈 Good — Acceptable"
        else if(rmseVal<0.30) badge="🥉 Fair — Artifacts visible"
        else badge="❌ Poor — Try more projections/iterations"
        resultCanvas.requestPaint()
    }

    function drawGray(ctx,img,W,H){
        if(!img||img.length===0) return
        var N=dimN, cw=W/N, ch=H/N
        for(var yi=0;yi<N;yi++) for(var xi=0;xi<N;xi++){
            var v=Math.round(img[yi*N+xi]*255)
            ctx.fillStyle="rgb("+v+","+v+","+v+")"; ctx.fillRect(xi*cw,yi*ch,cw+1,ch+1)
        }
    }

    Component.onCompleted: { groundTruth=computeGT(); runScan() }

    Flickable {
        anchors.fill: parent; contentWidth: width; contentHeight: mainCol.implicitHeight+56; clip: true

        ColumnLayout {
            id: mainCol; x: 28; y: 28; width: parent.width-56; spacing: 14

            Text { text: "Parameter Optimization Challenge"; color: root.textPri; font.pixelSize: 26; font.bold: true }
            Text {
                text: "Find the best scan parameters to minimize RMSE vs ground truth — while balancing dose and scan time. Adjust sliders then hit ▶ Run Scan."
                color: root.textSec; font.pixelSize: 14; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.4
            }

            RowLayout { Layout.fillWidth: true; height: 500; spacing: 14

                // ── Parameter panel ───────────────────────────────────────
                Rectangle {
                    Layout.preferredWidth: 310; height: 500
                    color: root.card; radius: 14; border.color: root.border; border.width: 1

                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 18; spacing: 14

                        Text { text: "⚙️  Scan Parameters"; color: root.accent; font.pixelSize: 14; font.bold: true }

                        // Projections
                        ColumnLayout { spacing: 5; Layout.fillWidth: true
                            RowLayout {
                                Text { text: "Projections:"; color: root.textSec; font.pixelSize: 13 }
                                Item { Layout.fillWidth: true }
                                Text { text: root.numAngles; color: root.accent; font.bold: true; font.pixelSize: 13 }
                            }
                            Slider { id: projS; Layout.fillWidth: true; from: 10; to: 120; stepSize: 5; value: 60
                                onValueChanged: root.numAngles = Math.floor(value)
                                background: Rectangle { x:projS.leftPadding;y:projS.topPadding+projS.availableHeight/2-3;width:projS.availableWidth;height:6;radius:3;color:"#334155";Rectangle{width:projS.visualPosition*parent.width;height:parent.height;radius:3;color:root.accent} }
                                handle: Rectangle { x:projS.leftPadding+projS.visualPosition*(projS.availableWidth-width);y:projS.topPadding+projS.availableHeight/2-9;width:18;height:18;radius:9;color:root.accent;border.color:"#fff";border.width:2 }
                            }
                            Text { text: root.numAngles<=30?"⚠️ Few angles":root.numAngles<=60?"~ Moderate":"✅ Good coverage"; color:root.textMuted; font.pixelSize:11 }
                        }

                        // Bins
                        ColumnLayout { spacing: 5; Layout.fillWidth: true
                            RowLayout {
                                Text { text: "Detector Bins:"; color: root.textSec; font.pixelSize: 13 }
                                Item { Layout.fillWidth: true }
                                Text { text: root.numBins; color: root.accent; font.bold: true; font.pixelSize: 13 }
                            }
                            Slider { id: binS; Layout.fillWidth: true; from: 16; to: 64; stepSize: 4; value: 32
                                onValueChanged: root.numBins = Math.floor(value)
                                background: Rectangle { x:binS.leftPadding;y:binS.topPadding+binS.availableHeight/2-3;width:binS.availableWidth;height:6;radius:3;color:"#334155";Rectangle{width:binS.visualPosition*parent.width;height:parent.height;radius:3;color:root.accent} }
                                handle: Rectangle { x:binS.leftPadding+binS.visualPosition*(binS.availableWidth-width);y:binS.topPadding+binS.availableHeight/2-9;width:18;height:18;radius:9;color:root.accent;border.color:"#fff";border.width:2 }
                            }
                            Text { text: root.numBins<=20?"⚠️ Low resolution":"✅ Adequate"; color:root.textMuted; font.pixelSize:11 }
                        }

                        // SIRT Iterations
                        ColumnLayout { spacing: 5; Layout.fillWidth: true
                            RowLayout {
                                Text { text: "SIRT Iterations:"; color: root.textSec; font.pixelSize: 13 }
                                Item { Layout.fillWidth: true }
                                Text { text: root.numIters; color: root.accent2; font.bold: true; font.pixelSize: 13 }
                            }
                            Slider { id: iterS; Layout.fillWidth: true; from: 0; to: 30; stepSize: 1; value: 5
                                onValueChanged: root.numIters = Math.floor(value)
                                background: Rectangle { x:iterS.leftPadding;y:iterS.topPadding+iterS.availableHeight/2-3;width:iterS.availableWidth;height:6;radius:3;color:"#334155";Rectangle{width:iterS.visualPosition*parent.width;height:parent.height;radius:3;color:root.accent2} }
                                handle: Rectangle { x:iterS.leftPadding+iterS.visualPosition*(iterS.availableWidth-width);y:iterS.topPadding+iterS.availableHeight/2-9;width:18;height:18;radius:9;color:root.accent2;border.color:"#fff";border.width:2 }
                            }
                            Text { text: root.numIters===0?"Pure FBP":"Hybrid FBP+SIRT"; color:root.textMuted; font.pixelSize:11 }
                        }

                        // Noise
                        ColumnLayout { spacing: 5; Layout.fillWidth: true
                            RowLayout {
                                Text { text: "Dose Noise (σ):"; color: root.textSec; font.pixelSize: 13 }
                                Item { Layout.fillWidth: true }
                                Text { text: root.noiseSig.toFixed(2); color: "#ef4444"; font.bold: true; font.pixelSize: 13 }
                            }
                            Slider { id: noiseS; Layout.fillWidth: true; from: 0; to: 0.3; value: 0
                                onValueChanged: root.noiseSig = value
                                background: Rectangle { x:noiseS.leftPadding;y:noiseS.topPadding+noiseS.availableHeight/2-3;width:noiseS.availableWidth;height:6;radius:3;color:"#334155";Rectangle{width:noiseS.visualPosition*parent.width;height:parent.height;radius:3;color:"#ef4444"} }
                                handle: Rectangle { x:noiseS.leftPadding+noiseS.visualPosition*(noiseS.availableWidth-width);y:noiseS.topPadding+noiseS.availableHeight/2-9;width:18;height:18;radius:9;color:"#ef4444";border.color:"#fff";border.width:2 }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        Button {
                            Layout.fillWidth: true; implicitHeight: 46; text: "▶  Run Scan & Reconstruct"
                            onClicked: root.runScan()
                            background: Rectangle { color: parent.pressed ? Qt.darker(root.accent,1.3) : root.accent; radius: 10 }
                            contentItem: Text { text: parent.text; color: "#fff"; font.bold: true; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                }

                // ── Results panel ─────────────────────────────────────────
                ColumnLayout { Layout.fillWidth: true; height: 500; spacing: 14

                    // Image comparison
                    RowLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 14

                        Rectangle {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            color: root.card; radius: 12; border.color: root.border; border.width: 1; clip: true
                            Canvas {
                                anchors.fill: parent; anchors.margins: 6
                                onPaint: { var ctx=getContext("2d"); ctx.fillStyle="#0b1120"; ctx.fillRect(0,0,width,height); root.drawGray(ctx,root.groundTruth,width,height) }
                                Component.onCompleted: requestPaint()
                            }
                            Text { anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottomMargin: 8; text: "Ground Truth"; color: root.textMuted; font.pixelSize: 12 }
                        }

                        Rectangle {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            color: root.card; radius: 12; border.color: root.accent; border.width: 1; clip: true
                            Canvas {
                                id: resultCanvas
                                anchors.fill: parent; anchors.margins: 6
                                onPaint: { var ctx=getContext("2d"); ctx.fillStyle="#0b1120"; ctx.fillRect(0,0,width,height); root.drawGray(ctx,root.reconImage,width,height) }
                            }
                            Text { anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottomMargin: 8; text: "Your Reconstruction"; color: root.accent; font.pixelSize: 12; font.bold: true }
                        }
                    }

                    // Score card
                    Rectangle {
                        Layout.fillWidth: true; height: 160
                        color: root.card; radius: 12; border.color: root.border; border.width: 1

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 16; spacing: 10

                            Text { text: "📈  Results"; color: root.textPri; font.pixelSize: 13; font.bold: true }

                            RowLayout { spacing: 12
                                Text { text: "RMSE:"; color: root.textSec; font.pixelSize: 13 }
                                Text {
                                    text: root.rmseVal.toFixed(4)
                                    color: root.rmseVal<0.06 ? "#34d399" : root.rmseVal<0.20 ? "#fbbf24" : "#ef4444"
                                    font.pixelSize: 22; font.bold: true
                                }
                                Rectangle {
                                    Layout.fillWidth: true; height: 12; radius: 6; color: "#334155"
                                    Rectangle {
                                        property real frac: Math.max(0,Math.min(1,1.0-root.rmseVal/0.5))
                                        width: parent.width*frac; height: parent.height; radius: 6
                                        color: frac>0.75?"#34d399":frac>0.4?"#fbbf24":"#ef4444"
                                        Behavior on width { NumberAnimation { duration: 500 } }
                                    }
                                }
                            }

                            Text { text: root.badge; color: root.textPri; font.pixelSize: 14; font.bold: true }

                            RowLayout { spacing: 24
                                Column { spacing: 3
                                    Text { text: "Scan Time"; color: root.textMuted; font.pixelSize: 11 }
                                    Text { text: (root.numAngles*0.5).toFixed(0)+" ms"; color: root.textSec; font.pixelSize: 13; font.bold: true }
                                }
                                Column { spacing: 3
                                    Text { text: "Radiation Dose"; color: root.textMuted; font.pixelSize: 11 }
                                    Text { text: (root.numAngles*0.18).toFixed(1)+" mGy"; color: root.textSec; font.pixelSize: 13; font.bold: true }
                                }
                                Column { spacing: 3
                                    Text { text: "Data Volume"; color: root.textMuted; font.pixelSize: 11 }
                                    Text { text: (root.numAngles*root.numBins/1024).toFixed(1)+" KB"; color: root.textSec; font.pixelSize: 13; font.bold: true }
                                }
                            }
                        }
                    }
                }
            }

            Item { height: 4 }
        }
    }
}
