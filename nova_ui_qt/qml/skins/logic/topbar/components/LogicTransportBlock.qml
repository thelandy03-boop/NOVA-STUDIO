import QtQuick
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 250
    implicitHeight: 70

    // Pista de botones de transporte analógicos
    Rectangle {
        anchors.centerIn: parent
        width: 245
        height: 48
        radius: 6
        color: "#32363F"
        border.color: "#1A1C22"
        border.width: 1
        clip: true

        Row {
            anchors.fill: parent

            // 1. Rewind
            Rectangle {
                width: 48
                height: parent.height
                color: maRewind.pressed ? "#252830" : "transparent"

                Image {
                    anchors.centerIn: parent
                    source: Qt.resolvedUrl("../icons/svg/LogicIconRewind.svg")
                    width: 24
                    height: 24
                    opacity: 0.95
                }

                MouseArea {
                    id: maRewind
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof AudioEngine !== "undefined") {
                            AudioEngine.rewind();
                        }
                    }
                }
            }
            Rectangle { width: 1; height: parent.height; color: "#1A1C22" }

            // 2. Play
            Rectangle {
                width: 48
                height: parent.height
                color: (typeof AudioEngine !== "undefined" && AudioEngine.isPlaying) 
                        ? "#454A56" 
                        : (maPlay.pressed ? "#252830" : "transparent")

                Image {
                    anchors.centerIn: parent
                    source: Qt.resolvedUrl("../icons/svg/LogicIconPlay.svg")
                    width: 26
                    height: 26
                    opacity: 0.95
                }

                MouseArea {
                    id: maPlay
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof AudioEngine !== "undefined") {
                            AudioEngine.togglePlay();
                        }
                    }
                }
            }
            Rectangle { width: 1; height: parent.height; color: "#1A1C22" }

            // 3. Stop
            Rectangle {
                width: 48
                height: parent.height
                color: maStop.pressed ? "#252830" : "transparent"

                Image {
                    anchors.centerIn: parent
                    source: Qt.resolvedUrl("../icons/svg/LogicIconStop.svg")
                    width: 22
                    height: 22
                    opacity: 0.95
                }

                MouseArea {
                    id: maStop
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof AudioEngine !== "undefined") {
                            AudioEngine.stop();
                        }
                    }
                }
            }
            Rectangle { width: 1; height: parent.height; color: "#1A1C22" }

            // 4. Record (Conmutador 1º toque = Grabación + Marcha / 2º toque = Parar)
            Rectangle {
                width: 48
                height: parent.height
                color: (typeof AudioEngine !== "undefined" && AudioEngine.isRecording)
                        ? "#5A1A1A" 
                        : (maRecord.pressed ? "#252830" : "transparent")

                Rectangle {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    radius: 10
                    color: (typeof AudioEngine !== "undefined" && AudioEngine.isRecording) 
                            ? "#FF1744" 
                            : "#E53935"
                }

                MouseArea {
                    id: maRecord
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof AudioEngine !== "undefined") {
                            AudioEngine.toggleRecord();
                        }
                    }
                }
            }
            Rectangle { width: 1; height: parent.height; color: "#1A1C22" }

            // 5. Loop
            Rectangle {
                width: 48
                height: parent.height
                color: (typeof AudioEngine !== "undefined" && AudioEngine.loopEnabled)
                        ? "#554A20"
                        : (maLoop.pressed ? "#252830" : "transparent")

                Image {
                    anchors.centerIn: parent
                    source: Qt.resolvedUrl("../icons/svg/IconLoop.svg")
                    width: 24
                    height: 24
                    opacity: 0.95
                }

                MouseArea {
                    id: maLoop
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof AudioEngine !== "undefined") {
                            AudioEngine.toggleLoop();
                        }
                    }
                }
            }
        }
    }
}