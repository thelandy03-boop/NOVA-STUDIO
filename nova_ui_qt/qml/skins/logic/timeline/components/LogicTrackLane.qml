import QtQuick

Rectangle {
    id: root
    width: parent ? parent.width : 4800
    height: 74 // Coincidencia exacta de 74px con LogicTrackCard
    color: (index % 2 === 0) ? "#1E1F24" : "#191A1E"

    // ── Indicador de Color Lateral (Franja izquierda de pista) ──
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 3
        color: model.trackColor || "#3498db"
    }

    // ── Borde Inferior Sombra/Separador ──
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#111216"
    }

    // ── Nombre de Pista Sutil en Marca de Agua ──
    Text {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: model.trackName
        color: "#2C2F38"
        font.pixelSize: 18
        font.bold: true
    }

    // ── CONTENEDOR DE REGIONES (Área reservada para Bloques de Audio/MIDI) ──
    Item {
        id: regionContainer
        anchors.fill: parent
        // Aquí se instanciarán los bloques de Audio / MIDI en futuras fases
    }
}