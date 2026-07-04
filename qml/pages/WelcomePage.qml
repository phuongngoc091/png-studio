pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LAStudio
import "../components/base"

Rectangle {
    id: root

    color: Theme.background

    signal pageRequested(string routeId)

    readonly property bool compact: width < 980
    readonly property int pageMargin: compact ? Theme.paddingLarge : 40
    readonly property bool omniVoiceSetupCompleted: AppController.models.hasFile("k2-fsa/OmniVoice", "omnivoice-base-Q8_0.gguf") && AppController.models.hasFile("k2-fsa/OmniVoice", "omnivoice-tokenizer-F32.gguf")
    readonly property var toolCards: [
        {
            title: qsTr("Text to Speech"),
            group: qsTr("Generation"),
            description: qsTr("Generate natural speech from scripts using models installed on this device."),
            iconName: "volume",
            routeId: "studio-tts",
            accent: "#a27eff"
        },
        {
            title: qsTr("Voice Cloning"),
            group: qsTr("Custom Voice"),
            description: qsTr("Create speech from a reference voice while keeping the workflow offline."),
            iconName: "spark",
            routeId: "studio-voice-cloning",
            accent: "#ff8fb3"
        },
        {
            title: qsTr("Voice Design"),
            group: qsTr("Voice Control"),
            description: qsTr("Describe voice characteristics and generate speech with controllable style."),
            iconName: "waves",
            routeId: "studio-voice-design",
            accent: "#7bd88f"
        }
    ]

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.background }
            GradientStop { position: 1.0; color: Qt.darker(Theme.background, 1.16) }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: -160
        anchors.topMargin: -170
        width: 430
        height: 430
        radius: 215
        color: Qt.rgba(0.49, 0.30, 1.0, 0.12)
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: -120
        anchors.topMargin: 80
        width: 360
        height: 360
        radius: 180
        color: Qt.rgba(0.20, 0.55, 1.0, 0.07)
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: root.compact ? Theme.paddingLarge : 34
            anchors.bottomMargin: Theme.paddingXL
            width: parent.width - root.pageMargin * 2
            spacing: Theme.paddingLarge

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.paddingMedium

                Rectangle {
                    Layout.preferredWidth: 42
                    Layout.preferredHeight: 42
                    radius: 10
                    color: Qt.rgba(0.49, 0.30, 1.0, 0.18)
                    border.color: Qt.rgba(0.64, 0.49, 1.0, 0.34)
                    border.width: 1

                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/LAStudio/icons/app_icon_32.png"
                        width: 25
                        height: 25
                        fillMode: Image.PreserveAspectFit
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: qsTr("PNG Studio")
                        color: Theme.textPrimary
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Text {
                        text: qsTr("A customized version of LA Studio")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    RowLayout {
                        spacing: Theme.paddingMedium
                        
                        RowLayout {
                            spacing: 4
                            LineIcon {
                                name: "tiktok"
                                color: Theme.accentLight
                                Layout.preferredWidth: 14
                                Layout.preferredHeight: 14
                            }
                            Text {
                                text: "phuongngoc091"
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }
                        }

                        RowLayout {
                            spacing: 4
                            LineIcon {
                                name: "phone"
                                color: Theme.accentLight
                                Layout.preferredWidth: 14
                                Layout.preferredHeight: 14
                            }
                            Text {
                                text: "0932 468 218"
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }
                        }
                    }
                }

                StatusPill {
                    label: qsTr("Offline ready")
                    iconName: "check"
                    accent: Theme.success
                    visible: !root.compact
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: root.compact ? 310 : 236
                radius: 12
                color: Qt.rgba(1, 1, 1, 0.035)
                border.color: Qt.rgba(1, 1, 1, 0.075)
                border.width: 1
                clip: true

                Rectangle {
                    width: parent.width * 0.58
                    height: parent.height * 1.8
                    radius: width / 2
                    anchors.right: parent.right
                    anchors.rightMargin: -width * 0.28
                    anchors.verticalCenter: parent.verticalCenter
                    color: Qt.rgba(0.49, 0.30, 1.0, 0.10)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: root.compact ? Theme.paddingLarge : Theme.paddingXL
                    spacing: Theme.paddingXL

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: Theme.paddingMedium

                        Text {
                            text: qsTr("HOME")
                            color: Theme.accentLight
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.4
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Create, transcribe, and shape speech locally.")
                            color: Theme.textPrimary
                            font.pixelSize: root.compact ? 30 : 40
                            font.bold: true
                            lineHeight: 1.02
                            wrapMode: Text.WordWrap
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.maximumWidth: 760
                            text: qsTr("A focused desktop workspace for speech-to-text, text-to-speech, voice cloning, and voice design. Pick a feature card below to start.")
                            color: Theme.textSecondary
                            font.pixelSize: 14
                            lineHeight: 1.35
                            wrapMode: Text.WordWrap
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: Theme.paddingSmall

                            InfoChip { label: qsTr("Offline processing"); iconName: "cpu"; accent: Theme.accentLight }
                            InfoChip { label: qsTr("Private by design"); iconName: "folder"; accent: Theme.success }
                            InfoChip { label: qsTr("Four audio studios"); iconName: "spark"; accent: Theme.warning }
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 330
                        Layout.fillHeight: true
                        radius: 10
                        color: Qt.rgba(0, 0, 0, 0.16)
                        border.color: Qt.rgba(1, 1, 1, 0.07)
                        border.width: 1
                        visible: !root.compact

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.paddingLarge
                            spacing: Theme.paddingMedium

                            Text {
                                text: qsTr("Workspace Overview")
                                color: Theme.textPrimary
                                font.pixelSize: 14
                                font.bold: true
                            }

                            OverviewRow { label: qsTr("Core studios"); value: "4"; iconName: "spark"; accent: Theme.accentLight }
                            OverviewRow { label: qsTr("Audio workflows"); value: qsTr("Focused"); iconName: "waves"; accent: Theme.warning }
                            OverviewRow { label: qsTr("Runtime mode"); value: qsTr("Local"); iconName: "cpu"; accent: Theme.success }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: Qt.rgba(1, 1, 1, 0.07)
                            }

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("The home page highlights the main audio capabilities. Configuration stays inside each studio.")
                                color: Theme.textSecondary
                                font.pixelSize: 12
                                lineHeight: 1.25
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.paddingSmall
                visible: root.omniVoiceSetupCompleted

                Text {
                    text: qsTr("Studios & Tools")
                    color: Theme.textPrimary
                    font.pixelSize: 16
                    font.bold: true
                    Layout.fillWidth: true
                }

                Text {
                    text: qsTr("Choose a workflow")
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    visible: !root.compact
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width > 1420 ? 3 : (root.width > 860 ? 2 : 1)
                rowSpacing: Theme.paddingMedium
                columnSpacing: Theme.paddingMedium
                visible: root.omniVoiceSetupCompleted

                Repeater {
                    model: root.toolCards

                    ToolCard {
                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: 142
                        cardNumber: index + 1
                        title: modelData.title
                        group: modelData.group
                        description: modelData.description
                        iconName: modelData.iconName
                        targetRoute: modelData.routeId
                        accent: modelData.accent
                    }
                }
            }

            Rectangle {
                id: setupBox
                visible: !root.omniVoiceSetupCompleted
                Layout.fillWidth: true
                Layout.preferredHeight: root.compact ? 240 : 160
                radius: 12
                color: Qt.rgba(1.0, 0.45, 0.15, 0.06)
                border.color: Qt.rgba(1.0, 0.45, 0.15, 0.28)
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.paddingXL
                    spacing: Theme.paddingXL

                    Rectangle {
                        Layout.preferredWidth: 54
                        Layout.preferredHeight: 54
                        radius: 12
                        color: Qt.rgba(1.0, 0.45, 0.15, 0.13)
                        border.color: Qt.rgba(1.0, 0.45, 0.15, 0.3)
                        border.width: 1
                        Layout.alignment: Qt.AlignVCenter

                        LineIcon {
                            anchors.centerIn: parent
                            name: "download"
                            color: "#ff9800"
                            width: 24
                            height: 24
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: qsTr("OmniVoice Setup Required")
                            color: Theme.textPrimary
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Text {
                            text: qsTr("To begin using PNG Studio, please download and configure the OmniVoice voice engine model files.")
                            color: Theme.textSecondary
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    PrimaryButton {
                        text: qsTr("Set up OmniVoice")
                        iconName: "settings"
                        implicitWidth: 160
                        implicitHeight: 38
                        onClicked: root.pageRequested("models")
                    }
                }
            }
        }
    }

    component ToolCard: Rectangle {
        id: card

        property int cardNumber: 1
        property string title: ""
        property string group: ""
        property string description: ""
        property string iconName: ""
        property string targetRoute: ""
        property color accent: Theme.accentLight

        radius: 10
        color: hoverHandler.hovered ? Qt.rgba(1, 1, 1, 0.055) : Qt.rgba(1, 1, 1, 0.032)
        border.color: hoverHandler.hovered ? Qt.rgba(accent.r, accent.g, accent.b, 0.42) : Qt.rgba(1, 1, 1, 0.07)
        border.width: 1
        clip: true

        Behavior on color { ColorAnimation { duration: 130 } }
        Behavior on border.color { ColorAnimation { duration: 130 } }

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 3
            color: card.accent
            opacity: hoverHandler.hovered ? 1.0 : 0.72
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.paddingLarge
            spacing: Theme.paddingMedium

            Rectangle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                radius: 10
                color: Qt.rgba(card.accent.r, card.accent.g, card.accent.b, 0.12)
                border.color: Qt.rgba(card.accent.r, card.accent.g, card.accent.b, 0.28)
                border.width: 1
                Layout.alignment: Qt.AlignTop

                LineIcon {
                    anchors.centerIn: parent
                    name: card.iconName
                    color: card.accent
                    width: 21
                    height: 21
                    strokeWidth: 1.8
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.paddingSmall

                    Text {
                        text: card.group
                        color: card.accent
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 0.9
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Text {
                        text: ("0" + card.cardNumber).slice(-2)
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: card.title
                    color: Theme.textPrimary
                    font.pixelSize: 17
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: card.description
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    lineHeight: 1.25
                    wrapMode: Text.WordWrap
                    verticalAlignment: Text.AlignTop
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    Text {
                        text: qsTr("Open")
                        color: hoverHandler.hovered ? card.accent : Theme.textSecondary
                        font.pixelSize: 12
                        font.bold: true
                    }

                    LineIcon {
                        name: "chevron-right"
                        color: hoverHandler.hovered ? card.accent : Theme.textSecondary
                        Layout.preferredWidth: 14
                        Layout.preferredHeight: 14
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: false
            cursorShape: Qt.PointingHandCursor
            onClicked: root.pageRequested(card.targetRoute)
        }

        HoverHandler {
            id: hoverHandler
        }
    }

    component InfoChip: Rectangle {
        id: chip

        property string label: ""
        property string iconName: "check"
        property color accent: Theme.accentLight

        width: chipRow.implicitWidth + 22
        height: 30
        radius: 8
        color: Qt.rgba(accent.r, accent.g, accent.b, 0.10)
        border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.26)
        border.width: 1

        RowLayout {
            id: chipRow
            anchors.centerIn: parent
            spacing: 6

            LineIcon {
                name: chip.iconName
                color: chip.accent
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
            }

            Text {
                text: chip.label
                color: Theme.textPrimary
                font.pixelSize: 12
                font.bold: true
            }
        }
    }

    component StatusPill: Rectangle {
        id: statusPill

        property string label: ""
        property string iconName: "check"
        property color accent: Theme.success

        Layout.preferredWidth: statusRow.implicitWidth + 22
        Layout.preferredHeight: 32
        radius: 8
        color: Qt.rgba(accent.r, accent.g, accent.b, 0.10)
        border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.26)
        border.width: 1

        RowLayout {
            id: statusRow
            anchors.centerIn: parent
            spacing: 6

            LineIcon {
                name: statusPill.iconName
                color: statusPill.accent
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
            }

            Text {
                text: statusPill.label
                color: statusPill.accent
                font.pixelSize: 12
                font.bold: true
            }
        }
    }

    component OverviewRow: RowLayout {
        id: overviewRow

        property string label: ""
        property string value: ""
        property string iconName: "check"
        property color accent: Theme.accentLight

        Layout.fillWidth: true
        spacing: Theme.paddingSmall

        Rectangle {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            radius: 8
            color: Qt.rgba(overviewRow.accent.r, overviewRow.accent.g, overviewRow.accent.b, 0.10)

            LineIcon {
                anchors.centerIn: parent
                name: overviewRow.iconName
                color: overviewRow.accent
                width: 15
                height: 15
            }
        }

        Text {
            Layout.fillWidth: true
            text: overviewRow.label
            color: Theme.textSecondary
            font.pixelSize: 12
        }

        Text {
            text: overviewRow.value
            color: Theme.textPrimary
            font.pixelSize: 12
            font.bold: true
        }
    }
}
