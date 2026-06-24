import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LAStudio
import "../../components"
import "../../components/base"

Item {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true

    Connections {
        target: AppController.logs
        function onLogContentChanged() {
            logTextArea.text = AppController.logs.logContent;
            if (autoScrollCheck.checked) {
                // Scroll to bottom
                Qt.callLater(function() {
                    logTextArea.cursorPosition = logTextArea.text.length;
                });
            }
        }
    }

    Component.onCompleted: {
        AppController.logs.requestLoadLogs();
    }

    Timer {
        id: autoRefreshTimer
        interval: 2000
        running: autoRefreshCheck.checked
        repeat: true
        onTriggered: AppController.logs.requestLoadLogs()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.paddingMedium

        // Action Toolbar
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.paddingMedium
            Layout.bottomMargin: 4

            // Custom Styled Auto Refresh CheckBox
            CheckBox {
                id: autoRefreshCheck
                text: qsTr("Auto-Refresh (2s)")
                checked: false
                
                indicator: Rectangle {
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 4
                    color: autoRefreshCheck.checked ? Theme.accent : "transparent"
                    border.color: autoRefreshCheck.checked ? Theme.accent : Qt.rgba(255, 255, 255, 0.2)
                    border.width: 1.5
                    anchors.verticalCenter: parent.verticalCenter
                    
                    Text {
                        anchors.centerIn: parent
                        text: "✓"
                        color: "white"
                        font.pixelSize: 10
                        font.bold: true
                        visible: autoRefreshCheck.checked
                    }
                }

                contentItem: Text {
                    text: autoRefreshCheck.text
                    font.pixelSize: Theme.fontMedium
                    color: Theme.textPrimary
                    leftPadding: autoRefreshCheck.indicator.width + Theme.paddingSmall
                    verticalAlignment: Text.AlignVCenter
                }
                
                HoverHandler { cursorShape: Qt.PointingHandCursor }
            }

            // Custom Styled Auto Scroll CheckBox
            CheckBox {
                id: autoScrollCheck
                text: qsTr("Auto-Scroll to Bottom")
                checked: true

                indicator: Rectangle {
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 4
                    color: autoScrollCheck.checked ? Theme.accent : "transparent"
                    border.color: autoScrollCheck.checked ? Theme.accent : Qt.rgba(255, 255, 255, 0.2)
                    border.width: 1.5
                    anchors.verticalCenter: parent.verticalCenter
                    
                    Text {
                        anchors.centerIn: parent
                        text: "✓"
                        color: "white"
                        font.pixelSize: 10
                        font.bold: true
                        visible: autoScrollCheck.checked
                    }
                }

                contentItem: Text {
                    text: autoScrollCheck.text
                    font.pixelSize: Theme.fontMedium
                    color: Theme.textPrimary
                    leftPadding: autoScrollCheck.indicator.width + Theme.paddingSmall
                    verticalAlignment: Text.AlignVCenter
                }

                HoverHandler { cursorShape: Qt.PointingHandCursor }
            }

            Item { Layout.fillWidth: true }

            // Styled Manual Refresh Button
            PrimaryButton {
                text: qsTr("Refresh")
                iconName: "refresh"
                buttonColor: Theme.surfaceAlt
                implicitWidth: 100
                implicitHeight: 34
                quiet: true
                onClicked: AppController.logs.requestLoadLogs()
            }

            // Styled Copy to Clipboard Button
            PrimaryButton {
                text: qsTr("Copy All")
                iconName: "copy"
                buttonColor: Theme.surfaceAlt
                implicitWidth: 110
                implicitHeight: 34
                quiet: true
                onClicked: {
                    var raw = AppController.logs.readLogFile();
                    AppController.copyToClipboard(raw);
                }
            }

            // Styled Clear Button
            PrimaryButton {
                text: qsTr("Clear Logs")
                iconName: "trash"
                buttonColor: Theme.danger
                implicitWidth: 120
                implicitHeight: 34
                onClicked: {
                    AppController.logs.clearLogFile();
                    logTextArea.text = "";
                    AppController.logs.requestLoadLogs();
                }
            }
        }

        // Logs Console Panel
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Qt.rgba(0, 0, 0, 0.2)
            radius: Theme.radiusMedium
            border.color: Qt.rgba(255, 255, 255, 0.05)
            border.width: 1
            clip: true

            BusyIndicator {
                anchors.centerIn: parent
                running: AppController.logs.loading
                visible: running
                z: 10
            }

            ScrollView {
                id: logScrollView
                anchors.fill: parent
                anchors.margins: Theme.paddingMedium
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AsNeeded

                TextArea {
                    id: logTextArea
                    width: logScrollView.width - Theme.paddingMedium * 2
                    selectByMouse: true
                    readOnly: true
                    textFormat: Text.RichText
                    wrapMode: Text.WrapAnywhere
                    font.family: "Consolas"
                    font.pixelSize: 12
                    color: Theme.textPrimary
                    selectionColor: Theme.accent
                    selectedTextColor: "white"
                    placeholderText: qsTr("No logs captured yet.")
                    placeholderTextColor: Theme.textSecondary
                    
                    background: Rectangle {
                        color: "transparent"
                    }
                }
            }
        }
    }
}
