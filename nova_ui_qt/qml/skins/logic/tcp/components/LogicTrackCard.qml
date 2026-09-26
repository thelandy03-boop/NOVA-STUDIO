import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    width: parent ? parent.width : 260
    height: 74
    implicitWidth: 260
    implicitHeight: 74
    color: "#3E3E42"
    border.color: "#111111"
    border.width: 1

    // ── BISEL 3D (Relieve análogo clásico) ──
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        color: "transparent"
        border.color: "#5C5C60"
        border.width: 1
    }

    // ── FILA SUPERIOR ──

    // 1. Número de Pista
    Rectangle {
        x: 6; y: 6
        width: 22; height: 22
        color: "#28282B"
        border.color: "#111111"
        Text {
            anchors.centerIn: parent
            text: (index + 1).toString()
            color: "#9A9EA8"
            font.pixelSize: 11
        }
    }

    // 2. Botón Record Arm
    Rectangle {
        id: btnRecord
        x: 32; y: 6
        width: 22; height: 22
        radius: 3
        color: model.recEnable ? "#5A1A1A" : "#353535"
        border.color: "#111111"
        
        Rectangle { 
            anchors.fill: parent; 
            anchors.margins: 1; 
            color: "transparent"; 
            border.color: model.recEnable ? "#7A2E2E" : "#4A4A4A" 
        }
        
        Rectangle {
            anchors.centerIn: parent
            width: 10; height: 10
            radius: 5
            color: model.recEnable ? "#FF3B30" : "#8B2222"
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                AudioEngine.tracks.setRecEnable(index, !model.recEnable);
            }
        }
    }

    // 3. Pantalla LCD Nombre de Pista
    Rectangle {
        x: 58; y: 6
        width: 130; height: 22
        color: "#1F2024"
        border.color: "#111111"
        clip: true
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left; anchors.leftMargin: 6
            text: model.trackName
            color: "#E0E0E0"
            font.pixelSize: 11
            font.family: "sans-serif"
        }
    }

    // 4. Botón MUTE (M)
    Rectangle {
        id: btnMute
        x: 192; y: 6
        width: 28; height: 22
        radius: 2
        color: model.mute ? "#D9822B" : "#353535"
        border.color: "#111111"
        Rectangle { 
            anchors.fill: parent; 
            anchors.margins: 1; 
            border.color: model.mute ? "#FFB060" : "#4A4A4A"; 
            color: "transparent" 
        }
        Text { 
            anchors.centerIn: parent; 
            text: "M"; 
            color: model.mute ? "#000000" : "#C65555"; 
            font.pixelSize: 11; 
            font.weight: Font.Bold 
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                AudioEngine.tracks.setMute(index, !model.mute);
            }
        }
    }

    // 5. Botón SOLO (S)
    Rectangle {
        id: btnSolo
        x: 224; y: 6
        width: 28; height: 22
        radius: 2
        color: model.solo ? "#2E7D32" : "#353535"
        border.color: "#111111"
        Rectangle { 
            anchors.fill: parent; 
            anchors.margins: 1; 
            border.color: model.solo ? "#4CAF50" : "#4A4A4A"; 
            color: "transparent" 
        }
        Text { 
            anchors.centerIn: parent; 
            text: "S"; 
            color: model.solo ? "#FFFFFF" : "#C6C655"; 
            font.pixelSize: 11; 
            font.weight: Font.Bold 
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                AudioEngine.tracks.setSolo(index, !model.solo);
            }
        }
    }

    // ── FILA INFERIOR (Controles de mezcla analógicos) ──

    // 6. Eliminar Pista
    Text {
        x: 10; y: 40
        text: "✕"
        color: deleteMouseArea.containsMouse ? "#FF3B30" : "#666666"
        font.pixelSize: 11

        MouseArea {
            id: deleteMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                AudioEngine.tracks.removeTrack(index);
            }
        }
    }

    // 7. Perilla (Knob) de Volumen Analógico
    Item {
        x: 40; y: 38
        width: 24; height: 24
        
        Rectangle {
            anchors.centerIn: parent
            width: 22; height: 22
            radius: 11
            color: "#2C2D31"
            border.color: "#111111"

            Rectangle {
                width: 2; height: 8
                color: "#E2E6EF"
                x: 10; y: 2
                transformOrigin: Item.Bottom
                rotation: Math.max(-135, Math.min(135, ((model.gain + 60) / 66.0) * 270.0 - 135.0))
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: -14
            text: model.gain.toFixed(1) + " dB"
            color: "#FFFFFF"
            font.pixelSize: 9
            visible: volDragArea.pressed
        }

        MouseArea {
            id: volDragArea
            anchors.fill: parent
            cursorShape: Qt.SizeVerCursor
            property real lastY: 0
            onPressed: (mouse) => lastY = mouse.y
            onPositionChanged: (mouse) => {
                var deltaY = lastY - mouse.y
                lastY = mouse.y
                var currentGain = model.gain
                var newGain = Math.max(-60, Math.min(6, currentGain + deltaY * 0.5));
                AudioEngine.tracks.setGain(index, newGain);
            }
        }
    }

    // 8. Perilla (Knob) de Paneo
    Item {
        x: 74; y: 38
        width: 24; height: 24
        
        Rectangle {
            anchors.centerIn: parent
            width: 22; height: 22
            radius: 11
            color: "#2C2D31"
            border.color: "#111111"

            Rectangle {
                width: 2; height: 8
                color: "#E2E6EF"
                x: 10; y: 2
                transformOrigin: Item.Bottom
                rotation: model.pan * 135.0
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: -14
            text: model.pan === 0.0 ? "C" : (model.pan < 0.0 ? "L " + Math.abs(Math.round(model.pan * 100)) : "R " + Math.round(model.pan * 100))
            color: "#FFFFFF"
            font.pixelSize: 9
            visible: panDragArea.pressed
        }

        MouseArea {
            id: panDragArea
            anchors.fill: parent
            cursorShape: Qt.SizeHorCursor
            property real lastY: 0
            onPressed: (mouse) => lastY = mouse.y
            onPositionChanged: (mouse) => {
                var deltaY = lastY - mouse.y
                lastY = mouse.y
                var currentPan = model.pan
                var newPan = Math.max(-1.0, Math.min(1.0, currentPan + deltaY * 0.02));
                AudioEngine.tracks.setPan(index, newPan);
            }
        }
    }

    // 9. Botón FX
    Rectangle {
        x: 192; y: 40
        width: 60; height: 20
        radius: 2
        color: "#353535"
        border.color: "#111111"
        Rectangle { anchors.fill: parent; anchors.margins: 1; border.color: "#4A4A4A"; color: "transparent" }
        Text { anchors.centerIn: parent; text: "FX"; color: "#A0A0A0"; font.pixelSize: 10; font.weight: Font.Bold }
    }
}