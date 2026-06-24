import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LAStudio
import "../base"

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: 58
    color: Theme.background

    property var family: null
    property string statusText: "Ready"
    property color statusColor: Theme.success
    property var currentRuntimeItem: null
    property bool modelLoaded: false
    property bool processing: false
    property real cpuUsage: 0
    property string estimatedRamUsage: ""
    property string estimatedVramUsage: ""
    property bool studioReady: false
    property string studioTitle: ""
    property string backToolTip: qsTr("Back")
    property string runtimeDisplayText: ""
    property bool runtimeClickable: false

    signal backClicked()
    signal runtimeClicked()
    signal reloadRequested()
    signal ejectRequested()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.paddingLarge
        anchors.rightMargin: Theme.paddingLarge
        spacing: Theme.paddingMedium

        ToolIconButton {
            iconName: "chevron-left"
            toolTip: root.backToolTip
            onClicked: root.backClicked()
        }

        Image {
            source: "qrc:/LAStudio/icons/app_icon_32.png"
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            fillMode: Image.PreserveAspectFit
            Layout.alignment: Qt.AlignVCenter
            visible: root.width > 420
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            Text {
                Layout.fillWidth: true
                text: root.studioTitle !== "" ? root.studioTitle : (root.family ? qsTr("%1 Studio").arg(root.family.title) : qsTr("Studio"))
                color: Theme.textPrimary
                font.pixelSize: Theme.fontLarge
                font.bold: true
                elide: Text.ElideRight
            }

            RowLayout {
                spacing: Theme.paddingSmall

                Rectangle {
                    Layout.preferredWidth: 7
                    Layout.preferredHeight: 7
                    radius: 4
                    color: root.statusColor
                }

                Text {
                    text: root.statusText
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                }

                Text {
                    text: root.family ? " / " + root.family.modelId : ""
                    color: Theme.textSecondary
                    opacity: 0.8
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                    Layout.maximumWidth: 220
                }

                Rectangle {
                    radius: 5
                    color: Qt.rgba(0.22, 0.62, 0.39, 0.18)
                    border.color: Qt.rgba(0.22, 0.62, 0.39, 0.45)
                    border.width: 1
                    visible: root.processing
                    implicitHeight: 20
                    implicitWidth: cpuText.implicitWidth + 10

                    Text {
                        id: cpuText
                        anchors.centerIn: parent
                        text: "CPU " + root.cpuUsage.toFixed(1) + "%"
                        color: Theme.textPrimary
                        font.pixelSize: 10
                        font.bold: true
                    }
                }

                Rectangle {
                    radius: 5
                    color: Qt.rgba(0.22, 0.62, 0.39, 0.18)
                    border.color: Qt.rgba(0.22, 0.62, 0.39, 0.45)
                    border.width: 1
                    visible: root.modelLoaded
                    implicitHeight: 20
                    implicitWidth: ramText.implicitWidth + 10

                    Text {
                        id: ramText
                        anchors.centerIn: parent
                        text: "RAM " + root.estimatedRamUsage
                        color: Theme.textPrimary
                        font.pixelSize: 10
                        font.bold: true
                    }
                }

                Rectangle {
                    radius: 5
                    color: Qt.rgba(0.31, 0.48, 0.89, 0.16)
                    border.color: Qt.rgba(0.31, 0.48, 0.89, 0.40)
                    border.width: 1
                    visible: root.modelLoaded
                    implicitHeight: 20
                    implicitWidth: vramText.implicitWidth + 10

                    Text {
                        id: vramText
                        anchors.centerIn: parent
                        text: "VRAM " + root.estimatedVramUsage
                        color: Theme.textPrimary
                        font.pixelSize: 10
                        font.bold: true
                    }
                }
            }
        }

        Rectangle {
            visible: root.width > 600
            Layout.preferredWidth: 320
            Layout.minimumWidth: 120
            Layout.maximumWidth: 420
            Layout.preferredHeight: 38
            radius: 8
            color: Qt.rgba(1, 1, 1, 0.03)
            border.color: Qt.rgba(1, 1, 1, 0.08)
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                LineIcon {
                    name: "cpu"
                    color: Theme.textSecondary
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                }

                Text {
                    Layout.fillWidth: true
                    text: root.currentRuntimeItem ? (root.currentRuntimeItem.name + (root.currentRuntimeItem.version ? "  " + root.currentRuntimeItem.version : "")) : (root.runtimeDisplayText !== "" ? root.runtimeDisplayText : qsTr("No runtime installed"))
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSmall
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.runtimeClickable
                hoverEnabled: root.runtimeClickable
                cursorShape: root.runtimeClickable ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: root.runtimeClicked()
            }
        }

        Rectangle {
            Layout.preferredWidth: 78
            Layout.preferredHeight: 38
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.032)
            border.color: Qt.rgba(1, 1, 1, 0.085)
            border.width: 1

            RowLayout {
                id: modelActions
                anchors.fill: parent
                anchors.margins: 3
                spacing: 0

                HeaderActionButton {
                    iconName: "power"
                    toolTip: qsTr("Unload model from memory")
                    enabled: root.modelLoaded
                    onClicked: root.ejectRequested()
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 18
                    Layout.alignment: Qt.AlignVCenter
                    color: Qt.rgba(1, 1, 1, 0.08)
                }

                HeaderActionButton {
                    iconName: "refresh"
                    toolTip: root.modelLoaded ? qsTr("Reload current model") : qsTr("Load current model")
                    enabled: (root.studioReady || !root.modelLoaded) && !root.processing
                    onClicked: root.reloadRequested()
                }
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.surfaceAlt
    }

    component ToolIconButton: Button {
        id: iconButton
        property string iconName: ""
        property string toolTip: ""

        implicitWidth: 32
        implicitHeight: 32
        flat: true

        AppToolTip {
            text: iconButton.toolTip
            visible: iconButton.hovered && iconButton.toolTip !== ""
        }

        contentItem: LineIcon {
            name: iconButton.iconName
            color: iconButton.hovered ? Theme.accent : Theme.textSecondary
            anchors.centerIn: parent
            width: 18
            height: 18
        }

        background: Rectangle {
            radius: 7
            color: iconButton.checked ? Qt.rgba(0.49, 0.30, 1.0, 0.14) : (iconButton.hovered ? Qt.rgba(1, 1, 1, 0.045) : "transparent")
            border.color: iconButton.checked ? Qt.rgba(0.49, 0.30, 1.0, 0.70) : Qt.rgba(1, 1, 1, 0.08)
            border.width: iconButton.checked || iconButton.hovered ? 1 : 0
        }
        HoverHandler { cursorShape: Qt.PointingHandCursor }
    }

    component HeaderActionButton: Button {
        id: actionButton
        property string iconName: "refresh"
        property string toolTip: ""

        Layout.preferredWidth: 35
        Layout.preferredHeight: 32
        padding: 0
        flat: true

        AppToolTip {
            text: toolTip
            visible: parent.hovered && toolTip !== ""
        }

        contentItem: LineIcon {
            name: actionButton.iconName
            color: !actionButton.enabled ? Qt.rgba(Theme.textSecondary.r, Theme.textSecondary.g, Theme.textSecondary.b, 0.42)
                  : actionButton.hovered ? Theme.accent : Theme.textSecondary
            strokeWidth: actionButton.hovered ? 2.0 : 1.75
            anchors.centerIn: parent
            width: 17
            height: 17
        }

        background: Rectangle {
            radius: 8
            color: actionButton.enabled && actionButton.hovered ? Qt.rgba(0.49, 0.30, 1.0, 0.14) : "transparent"
            border.color: actionButton.enabled && actionButton.hovered ? Qt.rgba(0.49, 0.30, 1.0, 0.45) : "transparent"
            border.width: actionButton.enabled && actionButton.hovered ? 1 : 0
        }

        HoverHandler { cursorShape: actionButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
    }
}
