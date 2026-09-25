import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    implicitWidth: 260
    implicitHeight: 74
    color: "#3E3E42" // Gris industrial análogo (REAPER Base)
    border.color: "#111111"
    border.width: 1

    property string trackName: "Audio Track"
    property string trackNum: "1"

    // ── BISEL 3D (Relieve análogo clásico) ──
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        color: "transparent"
        border.color: "#5C5C60" // Brillo superior/izquierdo
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
            text: root.trackNum
            color: "#9A9EA8"
            font.pixelSize: 11
        }
    }

    // 2. Botón Record Arm (Rojo oscuro inactivo)
    Rectangle {
        x: 32; y: 6
        width: 22; height: 22
        radius: 3
        color: "#353535"
        border.color: "#111111"
        // Relieve interno del botón
        Rectangle { anchors.fill: parent; anchors.margins: 1; color: "transparent"; border.color: "#4A4A4A" }
        
        // Foco rojo analógico
        Rectangle {
            anchors.centerIn: parent
            width: 10; height: 10
            radius: 5
            color: "#8B2222" // Rojo apagado
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
            text: root.trackName
            color: "#E0E0E0"
            font.pixelSize: 11
            font.family: "sans-serif"
        }
    }

    // 4. Botón MUTE (M)
    Rectangle {
        x: 192; y: 6
        width: 28; height: 22
        radius: 2
        color: "#353535"
        border.color: "#111111"
        Rectangle { anchors.fill: parent; anchors.margins: 1; border.color: "#4A4A4A"; color: "transparent" }
        Text { anchors.centerIn: parent; text: "M"; color: "#C65555"; font.pixelSize: 11; font.weight: Font.Bold }
    }

    // 5. Botón SOLO (S)
    Rectangle {
        x: 224; y: 6
        width: 28; height: 22
        radius: 2
        color: "#353535"
        border.color: "#111111"
        Rectangle { anchors.fill: parent; anchors.margins: 1; border.color: "#4A4A4A"; color: "transparent" }
        Text { anchors.centerIn: parent; text: "S"; color: "#C6C655"; font.pixelSize: 11; font.weight: Font.Bold }
    }

    // ── FILA INFERIOR ──

    // 6. Icono de Enrutamiento / Carpeta
    Text {
        x: 10; y: 40
        text: "└"
        color: "#888888"
        font.pixelSize: 14
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

            // Indicador de nivel (Línea blanca)
            Rectangle {
                width: 2; height: 8
                color: "#E2E6EF"
                x: 10; y: 2
                transformOrigin: Item.Bottom
                rotation: -45 // Apuntando a las 10 en punto
            }
        }
    }

    // 8. Perilla (Knob) de Paneo (Centro)
    Item {
        x: 74; y: 38
        width: 24; height: 24
        
        Rectangle {
            anchors.centerIn: parent
            width: 22; height: 22
            radius: 11
            color: "#2C2D31"
            border.color: "#111111"

            // Indicador de nivel (Línea blanca centrada)
            Rectangle {
                width: 2; height: 8
                color: "#E2E6EF"
                x: 10; y: 2
                transformOrigin: Item.Bottom
                rotation: 0 // Apuntando a las 12 (Centro)
            }
        }
    }

    // 9. Botón de Inserciones FX
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