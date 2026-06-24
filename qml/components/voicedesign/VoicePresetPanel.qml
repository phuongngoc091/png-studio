import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LAStudio
import "../base"

Item {
    id: root

    property string familyId: ""
    property var presets: []

    signal presetSelected(string description, string name)

    function refreshPresets() {
        if (familyId !== "") {
            root.presets = AppController.voiceDesignPresets.presetsForFamily(familyId)
        } else {
            root.presets = []
        }
    }

    Component.onCompleted: refreshPresets()
    onFamilyIdChanged: refreshPresets()

    Connections {
        target: AppController.voiceDesignPresets
        function onPresetsChanged(famId) {
            if (famId === root.familyId) {
                root.refreshPresets()
            }
        }
    }

    // Modal/Inline Editor State
    property bool isEditing: false
    property string editPresetId: ""
    property string editName: ""
    property string editDescription: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.paddingMedium
        spacing: Theme.paddingMedium

        // Title and Add Button
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.paddingMedium

            Text {
                text: qsTr("Voice Description Presets")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontMedium
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                id: addBtn
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                flat: true
                visible: !root.isEditing

                contentItem: LineIcon {
                    name: "spark"
                    color: addBtn.hovered ? Theme.accentLight : Theme.textSecondary
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                }
                background: Rectangle {
                    color: addBtn.hovered ? Qt.rgba(0.49, 0.30, 1.0, 0.15) : Qt.rgba(1, 1, 1, 0.03)
                    radius: 8
                    border.color: addBtn.hovered ? Theme.accent : "transparent"
                    border.width: 1
                }
                onClicked: {
                    root.editPresetId = ""
                    root.editName = ""
                    root.editDescription = ""
                    root.isEditing = true
                }
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                AppToolTip { text: qsTr("Create new voice description preset") }
            }
        }

        // Inline Editor Pane
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: editorColumn.implicitHeight + Theme.paddingMedium * 2
            visible: root.isEditing
            color: Qt.rgba(1, 1, 1, 0.02)
            radius: Theme.radiusMedium
            border.color: Qt.rgba(1, 1, 1, 0.08)
            border.width: 1

            ColumnLayout {
                id: editorColumn
                anchors.fill: parent
                anchors.margins: Theme.paddingMedium
                spacing: Theme.paddingSmall

                Text {
                    text: root.editPresetId === "" ? qsTr("New Preset") : qsTr("Edit Preset")
                    color: Theme.accentLight
                    font.pixelSize: Theme.fontSmall
                    font.bold: true
                }

                TextField {
                    id: nameField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Preset Name (e.g. Energetic Narrator)")
                    text: root.editName
                    color: Theme.textPrimary
                    placeholderTextColor: Theme.textSecondary
                    background: Rectangle {
                        color: Qt.rgba(1, 1, 1, 0.03)
                        radius: 6
                        border.color: nameField.activeFocus ? Theme.accent : Qt.rgba(1, 1, 1, 0.08)
                    }
                    padding: Theme.paddingSmall
                    onTextChanged: root.editName = text
                }

                AppTextArea {
                    id: descField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Voice Description (e.g. A clear, friendly male voice speaking with high energy and confidence.)")
                    text: root.editDescription
                    background: Rectangle {
                        color: Qt.rgba(1, 1, 1, 0.03)
                        radius: 6
                        border.color: descField.activeFocus ? Theme.accent : Qt.rgba(1, 1, 1, 0.08)
                    }
                    padding: Theme.paddingSmall
                    onTextChanged: root.editDescription = text
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    spacing: Theme.paddingSmall

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Cancel"
                        flat: true
                        contentItem: Text {
                            text: parent.text
                            color: Theme.textSecondary
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: "transparent"
                        }
                        onClicked: root.isEditing = false
                        HoverHandler { cursorShape: Qt.PointingHandCursor }
                    }

                    PrimaryButton {
                        text: "Save"
                        enabled: root.editName.trim().length > 0 && root.editDescription.trim().length > 0
                        onClicked: {
                            if (root.editPresetId === "") {
                                AppController.voiceDesignPresets.addPreset(root.familyId, root.editName, root.editDescription)
                            } else {
                                AppController.voiceDesignPresets.updatePreset(root.editPresetId, root.editName, root.editDescription)
                            }
                            root.isEditing = false
                        }
                    }
                }
            }
        }

        // Preset List
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            visible: !root.isEditing && root.presets.length > 0

            ColumnLayout {
                width: parent.width - 8
                spacing: Theme.paddingSmall

                Repeater {
                    model: root.presets

                    delegate: Rectangle {
                        required property var modelData
                        required property int index

                        Layout.fillWidth: true
                        implicitHeight: itemLayout.implicitHeight + Theme.paddingSmall * 2
                        radius: Theme.radiusMedium
                        color: itemMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.04) : Qt.rgba(1, 1, 1, 0.015)
                        border.color: itemMouse.containsMouse ? Qt.rgba(0.49, 0.30, 1.0, 0.25) : Qt.rgba(1, 1, 1, 0.04)
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }

                        MouseArea {
                            id: itemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.presetSelected(modelData.description, modelData.name)
                        }

                        RowLayout {
                            id: itemLayout
                            anchors.fill: parent
                            anchors.margins: Theme.paddingMedium
                            spacing: Theme.paddingSmall

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: modelData.name
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSmall + 1
                                    font.bold: true
                                }

                                Text {
                                    text: modelData.description
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSmall
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideRight
                                    maximumLineCount: 2
                                    Layout.fillWidth: true
                                }
                            }

                            // Actions (Edit/Delete)
                            RowLayout {
                                visible: itemMouse.containsMouse
                                spacing: 2

                                Button {
                                    id: editBtn
                                    implicitWidth: 26
                                    implicitHeight: 26
                                    flat: true
                                    background: Rectangle { color: editBtn.hovered ? Qt.rgba(1, 1, 1, 0.06) : "transparent"; radius: 5 }
                                    contentItem: LineIcon {
                                        name: "settings"
                                        color: Theme.textSecondary
                                        anchors.centerIn: parent
                                        width: 14
                                        height: 14
                                    }
                                    onClicked: {
                                        root.editPresetId = modelData.id
                                        root.editName = modelData.name
                                        root.editDescription = modelData.description
                                        root.isEditing = true
                                    }
                                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                                    AppToolTip { text: qsTr("Edit preset") }
                                }

                                Button {
                                    id: deleteBtn
                                    implicitWidth: 26
                                    implicitHeight: 26
                                    flat: true
                                    background: Rectangle { color: deleteBtn.hovered ? Qt.rgba(239, 83, 80, 0.15) : "transparent"; radius: 5 }
                                    contentItem: LineIcon {
                                        name: "trash"
                                        color: deleteBtn.hovered ? Theme.danger : Theme.textSecondary
                                        anchors.centerIn: parent
                                        width: 14
                                        height: 14
                                    }
                                    onClicked: AppController.voiceDesignPresets.deletePreset(modelData.id)
                                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                                    AppToolTip { text: qsTr("Delete preset") }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Empty State
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !root.isEditing && root.presets.length === 0
            spacing: Theme.paddingMedium
            Layout.topMargin: 40

            LineIcon {
                name: "spark"
                color: Theme.textSecondary
                opacity: 0.35
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
            }

            Text {
                text: qsTr("No presets created yet.")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
