import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import LAStudio
import "../../components"
import "../../components/base"
import "../../components/shared/settings"

ScrollView {
    id: root

    clip: true
    contentWidth: availableWidth
    ScrollBar.vertical.policy: ScrollBar.AsNeeded

    property bool wideLayout: availableWidth >= 980
    property bool openDownloadsPaneOnStart: false
    readonly property int contentMaxWidth: 1120
    readonly property int rowLabelWidth: 210

    ColumnLayout {
        width: Math.min(root.contentMaxWidth, Math.max(0, root.availableWidth - Theme.paddingMedium * 2))
        anchors.left: parent.left
        anchors.leftMargin: Theme.paddingMedium
        spacing: Theme.paddingLarge

        Item { Layout.preferredHeight: 2 }

        GridLayout {
            Layout.fillWidth: true
            columns: root.wideLayout ? 2 : 1
            columnSpacing: Theme.paddingLarge
            rowSpacing: Theme.paddingLarge

            SectionPanel {
                title: "App Update"
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                SettingsRow {
                    label: "Installed"
                    valueText: "LA Studio v0.1.0"
                }

                ThinDivider {}

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.paddingMedium

                    Text {
                        text: "Updates channel"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSmall
                        font.bold: true
                        Layout.preferredWidth: root.rowLabelWidth
                        Layout.maximumWidth: root.rowLabelWidth
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.paddingSmall

                        PrimaryButton {
                            text: "Check for updates"
                            buttonColor: Theme.surfaceAlt
                            implicitWidth: 132
                            implicitHeight: 32
                            quiet: true
                        }

                        AppComboBox {
                            id: channelCombo
                            model: ["Stable", "Beta"]
                            currentIndex: 0
                            implicitWidth: 110
                            implicitHeight: 32
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }

            SectionPanel {
                title: "User Interface"
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.paddingMedium

                    ColumnLayout {
                        Layout.preferredWidth: root.rowLabelWidth
                        Layout.maximumWidth: root.rowLabelWidth
                        spacing: 3

                        Text {
                            text: "App Language"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSmall
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        Text {
                            text: "Still in development"
                            color: Theme.textSecondary
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }
                    }

                    AppComboBox {
                        id: langCombo
                        model: [
                            { text: "English", value: "en" },
                            { text: "Tiếng Việt (still in development)", value: "vi" }
                        ]
                        textRole: "text"
                        currentIndex: {
                            for (var i = 0; i < model.length; i++) {
                                if (model[i].value === AppController.settings.language) {
                                    return i
                                }
                            }
                            return 0
                        }
                        Layout.fillWidth: true
                        Layout.maximumWidth: 260
                        implicitHeight: 32
                        onActivated: (index) => {
                            AppController.settings.language = model[index].value
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            SectionPanel {
                title: "General Settings"
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.paddingMedium

                    Text {
                        text: "Open downloads pane when starting a new model download"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    ToggleRow {
                        text: ""
                        checked: root.openDownloadsPaneOnStart
                        Layout.fillWidth: false
                        Layout.preferredWidth: 42
                        onToggled: root.openDownloadsPaneOnStart = checked
                    }
                }
            }

            SectionPanel {
                title: "Models Directory"
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.paddingSmall

                    Text {
                        text: "Model downloads and indexing location"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSmall
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.paddingSmall

                        PathPill {
                            path: AppController.settings.modelsPath
                            Layout.fillWidth: true
                        }

                        PrimaryButton {
                            iconName: "more-horizontal"
                            buttonColor: Theme.surfaceAlt
                            implicitWidth: 36
                            implicitHeight: 32
                            quiet: true
                            enabled: !AppController.modelsMigration.running
                            onClicked: folderDialog.open()
                        }
                    }

                    Text {
                        visible: AppController.modelsMigration.running || AppController.modelsMigration.message !== ""
                        text: AppController.modelsMigration.message
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Text {
                        visible: AppController.modelsMigration.error !== ""
                        text: AppController.modelsMigration.error
                        color: Theme.danger
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }

        SectionPanel {
            title: "App Info"
            Layout.fillWidth: true

            InfoActionRow {
                label: "Open app logs"
                actionText: "Open"
                onActivated: Qt.openUrlExternally("file:///" + AppController.logsDir)
            }

            ThinDivider {}

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.paddingMedium

                Text {
                    text: "App home directory"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSmall
                    Layout.preferredWidth: root.rowLabelWidth
                    Layout.maximumWidth: root.rowLabelWidth
                }

                PathPill {
                    path: AppController.dataDir
                    Layout.fillWidth: true
                }

                PrimaryButton {
                    iconName: "copy"
                    buttonColor: Theme.surfaceAlt
                    implicitWidth: 32
                    implicitHeight: 30
                    quiet: true
                    onClicked: AppController.copyToClipboard(AppController.dataDir)
                }
            }

            ThinDivider {}

            InfoActionRow {
                label: "Report bug or send feedback"
                actionText: "Open in browser"
                actionIcon: "external-link"
                actionWidth: 146
                onActivated: Qt.openUrlExternally("https://github.com/dduongtrandai/LA-Studio-Dev/issues")
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.paddingMedium
            Layout.bottomMargin: Theme.paddingLarge
            spacing: Theme.paddingSmall

            Canvas {
                id: alienIcon
                Layout.alignment: Qt.AlignHCenter
                width: 33
                height: 24
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.fillStyle = "#a27eff"

                    var pixels = [
                        [0,0,1,0,0,0,0,0,1,0,0],
                        [0,0,0,1,0,0,0,1,0,0,0],
                        [0,0,1,1,1,1,1,1,1,0,0],
                        [0,1,1,0,1,1,1,0,1,1,0],
                        [1,1,1,1,1,1,1,1,1,1,1],
                        [1,0,1,1,1,1,1,1,1,0,1],
                        [1,0,1,0,0,0,0,0,1,0,1],
                        [0,0,0,1,1,0,1,1,0,0,0]
                    ]

                    var pixelWidth = width / 11
                    var pixelHeight = height / 8

                    for (var r = 0; r < 8; r++) {
                        for (var c = 0; c < 11; c++) {
                            if (pixels[r][c] === 1) {
                                ctx.fillRect(c * pixelWidth, r * pixelHeight, pixelWidth + 0.1, pixelHeight + 0.1)
                            }
                        }
                    }
                }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Made by LA Studio Team"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
                opacity: 0.6
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Select Models Directory"
        onAccepted: {
            AppController.modelsMigration.changeDirectory(selectedFolder.toString())
        }
    }

    component SectionPanel: Rectangle {
        property string title: ""
        default property alias content: panelContent.data

        implicitHeight: panelLayout.implicitHeight + Theme.paddingLarge * 2
        radius: Theme.radiusMedium
        color: Qt.rgba(255, 255, 255, 0.018)
        border.color: Qt.rgba(255, 255, 255, 0.055)
        border.width: 1

        ColumnLayout {
            id: panelLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.paddingLarge
            spacing: Theme.paddingMedium

            Text {
                text: title
                color: Theme.textPrimary
                font.pixelSize: Theme.fontMedium
                font.bold: true
                Layout.fillWidth: true
            }

            ColumnLayout {
                id: panelContent
                Layout.fillWidth: true
                spacing: Theme.paddingMedium
            }
        }
    }

    component SettingsRow: RowLayout {
        property string label: ""
        property string valueText: ""

        Layout.fillWidth: true
        spacing: Theme.paddingMedium

        Text {
            text: label
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSmall
            font.bold: true
            Layout.preferredWidth: root.rowLabelWidth
            Layout.maximumWidth: root.rowLabelWidth
        }

        Text {
            text: valueText
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    component InfoActionRow: RowLayout {
        id: actionRow

        property string label: ""
        property string actionText: ""
        property string actionIcon: ""
        property int actionWidth: 70
        signal activated()

        Layout.fillWidth: true
        spacing: Theme.paddingMedium

        Text {
            text: label
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSmall
            Layout.fillWidth: true
        }

        PrimaryButton {
            text: actionText
            iconName: actionIcon
            buttonColor: Theme.surfaceAlt
            implicitWidth: actionWidth
            implicitHeight: 30
            quiet: true
            onClicked: actionRow.activated()
        }
    }

    component PathPill: Rectangle {
        property string path: ""

        implicitHeight: 30
        radius: Theme.radiusSmall
        color: Qt.rgba(255, 255, 255, 0.04)
        border.color: Qt.rgba(255, 255, 255, 0.08)
        border.width: 1

        Text {
            anchors.fill: parent
            anchors.leftMargin: Theme.paddingMedium
            anchors.rightMargin: Theme.paddingMedium
            text: path
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
            font.family: "Consolas"
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideMiddle
        }
    }

    component ThinDivider: Rectangle {
        Layout.fillWidth: true
        height: 1
        color: Qt.rgba(255, 255, 255, 0.07)
    }
}
