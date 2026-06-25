import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LAStudio
import "../components/base"

Rectangle {
    id: root
    color: Theme.background

    signal pageRequested(string routeId)

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width - Theme.paddingXL * 2
            anchors.margins: Theme.paddingXL
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingXL

            // Header Section (Minimalist & Clean)
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                Layout.topMargin: Theme.paddingLarge

                RowLayout {
                    spacing: Theme.paddingSmall
                    Rectangle {
                        width: 22; height: 22
                        radius: 5
                        color: Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: "LA"
                            color: "#ffffff"
                            font.bold: true
                            font.pixelSize: 10
                        }
                    }
                    Text {
                        text: qsTr("LOCAL AUDIO WORKSTATION")
                        color: Theme.accentLight
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 1.0
                    }
                }

                Text {
                    text: qsTr("Welcome to LA Studio")
                    color: Theme.textPrimary
                    font.pixelSize: 26
                    font.bold: true
                }

                Text {
                    text: qsTr("Private, offline, and high-performance local AI audio processing. All operations run locally on your device.")
                    color: Theme.textSecondary
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            // Divider Line
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.surfaceAlt
                opacity: 0.4
            }

            // Grid of core tools
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.paddingMedium

                Text {
                    text: qsTr("Studios & Tools")
                    color: Theme.textPrimary
                    font.pixelSize: 15
                    font.bold: true
                    Layout.bottomMargin: Theme.paddingSmall
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.width > 1400 ? 5 : (root.width > 1100 ? 3 : (root.width > 700 ? 2 : 1))
                    rowSpacing: Theme.paddingMedium
                    columnSpacing: Theme.paddingMedium

                    FeatureCard {
                        Layout.fillWidth: true
                        title: qsTr("Speech to Text")
                        description: qsTr("Transcribe voice recordings, audio files, or live micro recordings to text offline using Whisper.")
                        iconName: "mic"
                        targetRoute: "studio-stt"
                    }

                    FeatureCard {
                        Layout.fillWidth: true
                        title: qsTr("Text to Speech")
                        description: qsTr("Synthesize natural, high-quality audio speech from text in multiple languages using local models.")
                        iconName: "volume"
                        targetRoute: "studio-tts"
                    }

                    FeatureCard {
                        Layout.fillWidth: true
                        title: qsTr("Voice Cloning")
                        description: qsTr("Generate synthetic speech using custom voice profiles cloned from short reference audio files.")
                        iconName: "spark"
                        targetRoute: "studio-voice-cloning"
                    }

                    FeatureCard {
                        Layout.fillWidth: true
                        title: qsTr("Voice Design")
                        description: qsTr("Generate synthetic voice characteristics and speech from natural language descriptions.")
                        iconName: "waves"
                        targetRoute: "studio-voice-design"
                    }

                    FeatureCard {
                        Layout.fillWidth: true
                        title: qsTr("Model Manager")
                        description: qsTr("Download and organize your local AI weights, check download queue, and set paths.")
                        iconName: "gallery"
                        targetRoute: "models"
                    }
                }
            }

            // Divider Line
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.surfaceAlt
                opacity: 0.4
                Layout.topMargin: Theme.paddingXL
            }

            // Footer row
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.paddingMedium

                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("Models Storage Location")
                        color: Theme.textSecondary
                        font.pixelSize: 11
                        font.bold: true
                    }
                    Text {
                        text: AppController.settings.modelsPath
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        font.family: "monospace"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Button {
                    flat: true
                    text: qsTr("✏ Settings")
                    contentItem: Text {
                        text: parent.text
                        color: parent.hovered ? Theme.accentLight : Theme.textSecondary
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
                        radius: Theme.radiusSmall
                    }
                    onClicked: root.pageRequested("settings")
                }
            }
        }
    }

    // Inline Card Component
    component FeatureCard: Rectangle {
        id: card
        property string title: ""
        property string description: ""
        property string iconName: ""
        property string targetRoute: ""

        Layout.fillWidth: true
        Layout.fillHeight: true
        implicitHeight: Math.max(104, cardRow.implicitHeight + Theme.paddingLarge * 2)
        radius: Theme.radiusMedium
        color: hoverHandler.hovered ? Theme.surfaceAlt : Theme.surface
        border.color: hoverHandler.hovered ? Qt.rgba(0.49, 0.30, 1.0, 0.35) : Qt.rgba(1, 1, 1, 0.04)
        border.width: 1

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }

        RowLayout {
            id: cardRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Theme.paddingLarge
            anchors.rightMargin: Theme.paddingLarge
            spacing: Theme.paddingMedium

            // Small glowing icon container
            Rectangle {
                width: 36
                height: 36
                radius: Theme.radiusSmall
                color: hoverHandler.hovered ? Qt.rgba(0.49, 0.30, 1.0, 0.12) : Qt.rgba(1.0, 1.0, 1.0, 0.02)
                border.color: hoverHandler.hovered ? Qt.rgba(0.49, 0.30, 1.0, 0.25) : "transparent"
                border.width: 1
                Layout.alignment: Qt.AlignVCenter

                Behavior on color { ColorAnimation { duration: 120 } }
                Behavior on border.color { ColorAnimation { duration: 120 } }

                LineIcon {
                    anchors.centerIn: parent
                    name: card.iconName
                    color: hoverHandler.hovered ? Theme.accentLight : Theme.textSecondary
                    width: 18
                    height: 18
                    strokeWidth: 1.7

                    Behavior on color { ColorAnimation { duration: 120 } }
                }
            }

            // Content Text
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 2

                Text {
                    text: card.title
                    color: Theme.textPrimary
                    font.pixelSize: 14
                    font.bold: true
                }

                Text {
                    text: card.description
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            // Right Chevron
            Text {
                text: "›"
                color: hoverHandler.hovered ? Theme.accentLight : Theme.textSecondary
                font.pixelSize: 20
                Layout.alignment: Qt.AlignVCenter
                leftPadding: Theme.paddingSmall
                rightPadding: Theme.paddingSmall

                Behavior on color { ColorAnimation { duration: 120 } }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: false
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                root.pageRequested(card.targetRoute)
            }
        }

        HoverHandler {
            id: hoverHandler
        }
    }
}
