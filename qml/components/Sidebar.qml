import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LAStudio
import "base"

Rectangle {
    id: root

    property int currentIndex: 0
    property bool isCollapsed: true
    property bool downloadsActive: false

    signal navigated(string routeId)
    signal downloadsClicked()

    color: Qt.darker(Theme.background, 1.02)

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.paddingMedium
        anchors.bottomMargin: Theme.paddingMedium
        spacing: Theme.paddingSmall

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 38
            Layout.preferredHeight: 38
            radius: 9
            color: Qt.rgba(0.49, 0.30, 1.0, 0.13)
            border.color: Qt.rgba(0.49, 0.30, 1.0, 0.28)
            border.width: 1

            Image {
                anchors.centerIn: parent
                source: "qrc:/LAStudio/icons/app_icon_32.png"
                width: 24
                height: 24
                fillMode: Image.PreserveAspectFit
            }

            AppToolTip {
                text: "LA Studio"
                visible: logoHover.hovered
            }

            HoverHandler {
                id: logoHover
                cursorShape: Qt.PointingHandCursor
            }
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 28
            height: 1
            color: Qt.rgba(1, 1, 1, 0.07)
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.paddingSmall
            spacing: 6

            Repeater {
                model: StudioRouteRegistry.routes.filter(function(r) { return r.id !== "settings"; })

                delegate: Item {
                    required property var modelData
                    required property int index

                    readonly property bool isCurrent: root.currentIndex === StudioRouteRegistry.getIndex(modelData.id)

                    Layout.fillWidth: true
                    Layout.preferredHeight: 46

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        width: 3
                        height: isCurrent ? 24 : 0
                        radius: 2
                        color: Theme.accentLight

                        Behavior on height {
                            NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                        }
                    }

                    Rectangle {
                        id: navButton
                        anchors.centerIn: parent
                        width: 42
                        height: 42
                        radius: 9
                        color: {
                            if (isCurrent) return Qt.rgba(0.49, 0.30, 1.0, 0.16)
                            if (navMouse.containsMouse) return Qt.rgba(1, 1, 1, 0.055)
                            return "transparent"
                        }
                        border.color: isCurrent ? Qt.rgba(0.49, 0.30, 1.0, 0.34) : "transparent"
                        border.width: 1

                        LineIcon {
                            anchors.centerIn: parent
                            name: modelData.iconName
                            color: isCurrent ? Theme.accentLight : (navMouse.containsMouse ? Theme.textPrimary : Theme.textSecondary)
                            width: 21
                            height: 21
                            strokeWidth: 1.8
                        }

                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                    }

                    MouseArea {
                        id: navMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.navigated(modelData.id)
                    }

                    AppToolTip {
                        text: modelData.label
                        visible: navMouse.containsMouse
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        Item {
            id: downloadsItem
            Layout.fillWidth: true
            Layout.preferredHeight: 46

            readonly property bool isActive: root.downloadsActive

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                width: 3
                height: downloadsItem.isActive ? 24 : 0
                radius: 2
                color: Theme.accentLight

                Behavior on height {
                    NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                }
            }

            Rectangle {
                id: downloadsNavButton
                anchors.centerIn: parent
                width: 42
                height: 42
                radius: 9
                color: {
                    if (downloadsItem.isActive) return Qt.rgba(0.49, 0.30, 1.0, 0.16)
                    if (downloadsMouse.containsMouse) return Qt.rgba(1, 1, 1, 0.055)
                    return "transparent"
                }
                border.color: downloadsItem.isActive ? Qt.rgba(0.49, 0.30, 1.0, 0.34) : "transparent"
                border.width: 1

                LineIcon {
                    anchors.centerIn: parent
                    name: "download"
                    color: downloadsItem.isActive ? Theme.accentLight : (downloadsMouse.containsMouse ? Theme.textPrimary : Theme.textSecondary)
                    width: 21
                    height: 21
                    strokeWidth: 1.8
                }

                Behavior on color { ColorAnimation { duration: 120 } }
                Behavior on border.color { ColorAnimation { duration: 120 } }
            }

            MouseArea {
                id: downloadsMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.downloadsClicked()
            }

            AppToolTip {
                text: "Downloads"
                visible: downloadsMouse.containsMouse
            }
        }

        Item {
            id: settingsItem
            Layout.fillWidth: true
            Layout.preferredHeight: 46

            readonly property var modelData: {
                for (var i = 0; i < StudioRouteRegistry.routes.length; i++) {
                    if (StudioRouteRegistry.routes[i].id === "settings") {
                        return StudioRouteRegistry.routes[i];
                    }
                }
                return { id: "settings", label: "Settings", iconName: "settings" };
            }
            readonly property bool isCurrent: root.currentIndex === StudioRouteRegistry.getIndex(modelData.id)

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                width: 3
                height: settingsItem.isCurrent ? 24 : 0
                radius: 2
                color: Theme.accentLight

                Behavior on height {
                    NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                }
            }

            Rectangle {
                id: settingsNavButton
                anchors.centerIn: parent
                width: 42
                height: 42
                radius: 9
                color: {
                    if (settingsItem.isCurrent) return Qt.rgba(0.49, 0.30, 1.0, 0.16)
                    if (settingsMouse.containsMouse) return Qt.rgba(1, 1, 1, 0.055)
                    return "transparent"
                }
                border.color: settingsItem.isCurrent ? Qt.rgba(0.49, 0.30, 1.0, 0.34) : "transparent"
                border.width: 1

                LineIcon {
                    anchors.centerIn: parent
                    name: settingsItem.modelData.iconName
                    color: settingsItem.isCurrent ? Theme.accentLight : (settingsMouse.containsMouse ? Theme.textPrimary : Theme.textSecondary)
                    width: 21
                    height: 21
                    strokeWidth: 1.8
                }

                Behavior on color { ColorAnimation { duration: 120 } }
                Behavior on border.color { ColorAnimation { duration: 120 } }
            }

            MouseArea {
                id: settingsMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.navigated(settingsItem.modelData.id)
            }

            AppToolTip {
                text: settingsItem.modelData.label
                visible: settingsMouse.containsMouse
            }
        }
    }
}
