import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LAStudio
import "../base"

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: active ? 198 : 0
    visible: active
    opacity: active ? 1 : 0
    radius: Theme.radiusSmall
    color: Qt.rgba(1, 1, 1, 0.035)
    border.color: Qt.rgba(0.49, 0.30, 1.0, 0.22)
    border.width: 1
    clip: true

    property bool outputReady: false
    property var samples: []
    property string durationText: "--"
    property int sampleRate: 0
    property string sampleCountText: ""
    property bool isPlaying: false
    property var family: null
    property bool processing: false
    property int generationProgress: 0
    property bool progressEstimated: true
    property string progressLabel: qsTr("Generating audio")
    readonly property bool active: outputReady || processing
    readonly property real normalizedProgress: Math.max(0, Math.min(100, generationProgress)) / 100.0

    signal playClicked()
    signal saveClicked()

    Behavior on opacity {
        NumberAnimation { duration: 160; easing.type: Easing.OutQuad }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.paddingMedium
        spacing: Theme.paddingSmall

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            spacing: Theme.paddingSmall

            Rectangle {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                radius: 7
                color: Qt.rgba(0.49, 0.30, 1.0, 0.14)

                LineIcon {
                    anchors.centerIn: parent
                    name: "volume"
                    color: root.family ? root.family.accent : Theme.accentLight
                    width: 15
                    height: 15
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Generated Audio")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontMedium
                font.bold: true
            }

            Rectangle {
                implicitWidth: outputMetaRow.implicitWidth + Theme.paddingMedium
                implicitHeight: 26
                radius: 6
                color: Qt.rgba(0, 0, 0, 0.12)
                border.color: Qt.rgba(1, 1, 1, 0.06)
                border.width: 1

                RowLayout {
                    id: outputMetaRow
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        text: root.durationText
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSmall
                        font.bold: true
                    }

                    Text {
                        text: root.sampleRate > 0 ? root.sampleRate + " Hz" : "-- Hz"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                    }
                }
            }
        }

        WaveformView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !root.processing
            samples: root.samples
            framed: true
            placeholderText: ""
            showPlaceholder: false
            barWidth: 3
            barGap: 2
            waveColor: root.family ? root.family.accent : Theme.accent
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.processing
            radius: Theme.radiusSmall
            color: Qt.rgba(0, 0, 0, 0.10)
            border.color: Qt.rgba(1, 1, 1, 0.06)
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.paddingMedium
                spacing: Theme.paddingSmall

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.paddingSmall

                    BusyIndicator {
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        running: root.processing
                        visible: root.processing
                        palette.dark: root.family ? root.family.accent : Theme.accentLight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.progressEstimated ? qsTr("%1 (estimated)").arg(root.progressLabel) : root.progressLabel
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSmall
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Text {
                        text: root.generationProgress + "%"
                        color: root.family ? root.family.accent : Theme.accentLight
                        font.pixelSize: Theme.fontSmall
                        font.bold: true
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 8
                    radius: 4
                    color: Theme.surface

                    Rectangle {
                        height: parent.height
                        radius: 4
                        color: root.family ? root.family.accent : Theme.accent
                        width: parent.width * root.normalizedProgress
                        Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: root.progressEstimated ? qsTr("The selected runtime does not report exact inference progress.") : qsTr("Runtime progress is reported by the selected backend.")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            spacing: Theme.paddingSmall

            Text {
                Layout.fillWidth: true
                text: root.sampleCountText
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }

            PrimaryButton {
                text: root.isPlaying ? "Stop" : "Play"
                iconName: root.isPlaying ? "stop" : "play"
                quiet: !root.isPlaying
                buttonColor: root.isPlaying ? Theme.danger : Theme.surface
                textColor: root.isPlaying ? "#ffffff" : Theme.success
                borderColor: Qt.rgba(0.40, 0.73, 0.42, 0.26)
                implicitWidth: 112
                enabled: !root.processing && root.samples.length > 0
                onClicked: root.playClicked()
            }

            PrimaryButton {
                text: "Save"
                iconName: "save"
                quiet: true
                buttonColor: Theme.surface
                textColor: Theme.textPrimary
                borderColor: Qt.rgba(1, 1, 1, 0.09)
                implicitWidth: 112
                enabled: !root.processing && root.samples.length > 0
                onClicked: root.saveClicked()
            }
        }
    }
}
