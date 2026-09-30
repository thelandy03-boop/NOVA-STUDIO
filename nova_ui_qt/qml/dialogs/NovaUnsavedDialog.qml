import QtQuick
import QtQuick.Layouts

Rectangle {
    id: dialogRoot
    anchors.fill: parent
    color: "#80000000" // Fondo de cristal traslúcido suave
    z: 100000
    visible: false

    signal saveAndExit()
    signal discardAndExit()
    signal cancelClose()

    // Bloqueador de clics accidentales fuera de la tarjeta
    MouseArea {
        anchors.fill: parent
        propagateComposedEvents: false
        hoverEnabled: true
        onClicked: {}
    }

    // Tarjeta Modal Estilo Logic Pro / macOS Alert
    Rectangle {
        id: dialogBox
        anchors.centerIn: parent
        width: 430
        height: 165
        color: "#28292D"
        border.color: "#3F4046"
        border.width: 1
        radius: 10

        // Relieve superior sutil de cristal
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            color: "transparent"
            border.color: "#1AFFFFFF"
            border.width: 1
            radius: 9
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            // Fila de Cabecera (Icono SVG Vectorial + Título)
            RowLayout {
                spacing: 12
                Layout.fillWidth: true

                // Icono SVG Vectorial Nativo de Advertencia
                Image {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    sourceSize.width: 26
                    sourceSize.height: 26
                    smooth: true
                    antialiasing: true
                    source: "data:image/svg+xml;utf8,<svg viewBox='0 0 64 64' xmlns='http://www.w3.org/2000/svg'><path d='M5.9 62c-3.3 0-4.8-2.4-3.3-5.3L29.3 4.2c1.5-2.9 3.9-2.9 5.4 0l26.7 52.5c1.5 2.9 0 5.3-3.3 5.3H5.9z' fill='%23ffce31'/><g fill='%23231f20'><path d='M27.8 23.6l2.8 18.5c.3 1.8 2.6 1.8 2.9 0l2.7-18.5c.5-7.2-8.9-7.2-8.4 0'/><circle cx='32' cy='49.6' r='4.2'/></g></svg>"
                }

                Text {
                    text: "Proyecto Modificado"
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    font.bold: true
                    font.family: "sans-serif"
                    Layout.fillWidth: true
                }
            }

            // Cuerpo del Mensaje
            Text {
                text: "El proyecto '" + AudioEngine.currentProjectName + "' tiene cambios sin guardar.\n¿Deseas guardar los cambios antes de salir?"
                color: "#A2A3A8"
                font.pixelSize: 11
                font.family: "sans-serif"
                lineHeight: 1.35
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Item { Layout.fillHeight: true } // Espaciador vertical dinámico

            // Fila de Botones Alineados
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true } // Empuja los botones a la derecha

                // Botón: Cancelar
                Rectangle {
                    implicitWidth: 80
                    implicitHeight: 28
                    radius: 5
                    color: cancelMouse.containsMouse ? "#424349" : "#34353A"
                    border.color: "#4A4C54"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "Cancelar"
                        color: "#D1D2D6"
                        font.pixelSize: 11
                        font.family: "sans-serif"
                    }

                    MouseArea {
                        id: cancelMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: dialogRoot.cancelClose()
                    }
                }

                // Botón: Salir sin Guardar
                Rectangle {
                    implicitWidth: 120
                    implicitHeight: 28
                    radius: 5
                    color: discardMouse.containsMouse ? "#3A2224" : "#34353A"
                    border.color: discardMouse.containsMouse ? "#FF453A" : "#4A4C54"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "Salir sin Guardar"
                        color: discardMouse.containsMouse ? "#FF453A" : "#D1D2D6"
                        font.pixelSize: 11
                        font.family: "sans-serif"
                    }

                    MouseArea {
                        id: discardMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: dialogRoot.discardAndExit()
                    }
                }

                // Botón Principal: GUARDAR Y SALIR (Logic Accent Blue)
                Rectangle {
                    implicitWidth: 115
                    implicitHeight: 28
                    radius: 5
                    color: saveMouse.containsMouse ? "#0071E3" : "#0A84FF"

                    Text {
                        anchors.centerIn: parent
                        text: "Guardar y Salir"
                        color: "#FFFFFF"
                        font.pixelSize: 11
                        font.bold: true
                        font.family: "sans-serif"
                    }

                    MouseArea {
                        id: saveMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: dialogRoot.saveAndExit()
                    }
                }
            }
        }
    }
}