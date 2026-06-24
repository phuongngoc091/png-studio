import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LAStudio
import "../base"

RowLayout {
    id: root
    spacing: 0
    property int railGap: Theme.paddingMedium

    property var family: null
    property var families: []
    property string capability: "tts"
    property string selectedFamilyId: family ? family.id : ""
    property var studioContext: null
    property bool studioReady: false
    property bool isSettingsOpen: true
    property string studioTitle: ""
    property bool showSwitcher: true
    property string backToolTip: qsTr("Back")
    property bool modalSelectionMode: false
    property string modalSelectionTitle: qsTr("Model + Runtime")
    property string modalSelectionValue: qsTr("Select model and runtime")
    property string modalSelectionDetail: ""
    property var studioController: null
    
    property bool showLeftPanel: false
    property bool isLeftPanelOpen: true
    
    property alias leftPanelContent: leftPanelItem.children
    property alias mainContent: mainContentItem.children
    property alias settingsContent: settingsItem.children

    signal requestBack()
    signal requestConfigurationPicker()
    signal requestReload()
    signal requestEject()
    signal requestModelSwitch(string familyId)
    signal requestRuntimeSwitch(string runtimeId)

    function currentRuntimeItem() {
        var fam = activeFamily()
        if (!fam) return null
        var current = studioContext ? studioContext.runtimeId : ""
        var currentVersion = studioContext ? studioContext.runtimeVersion : ""
        var runtimes = AppController.runtimes.allRuntimes
        for (var i = 0; i < runtimes.length; i++) {
            var r = runtimes[i]
            if (r.id !== current) continue
            if (currentVersion === "" || r.version === currentVersion) {
                for (var f = 0; f < fam.runtimes.length; f++) {
                    if (fam.runtimes[f].id === r.id) {
                        return {
                            "id": r.id,
                            "name": fam.runtimes[f].name,
                            "version": r.version
                        }
                    }
                }
            }
        }
        return null
    }

    function statusText() {
        var raw = studioController ? studioController.statusText : (root.studioReady ? "Ready" : "Setup required")
        if (raw === "Ready") return qsTr("Ready")
        if (raw === "Setup required") return qsTr("Setup required")
        if (raw === "Processing") return qsTr("Processing")
        if (raw === "Loading") return qsTr("Loading")
        if (raw === "Error") return qsTr("Error")
        return raw
    }

    function statusColor() {
        var stText = studioController ? studioController.statusText : (root.studioReady ? "Ready" : "Setup required")
        if (stText === "Ready" || stText === "Processing") return Theme.success
        if (stText === "Loading") return Theme.warning
        if (stText === "Error") return Theme.danger
        return Theme.textSecondary
    }

    function runtimeInstalled(runtimeId) {
        if (!runtimeId) return false
        var registry = AppController.runtimes.allRuntimes
        for (var i = 0; i < registry.length; i++) {
            if (registry[i].id === runtimeId) return true
        }
        return false
    }

    function activeFamily() {
        if (family && family.id) return family
        for (var i = 0; i < families.length; i++) {
            if (families[i].id === selectedFamilyId) return families[i]
        }
        return null
    }

    function installedRuntimes() {
        var fam = activeFamily()
        if (!fam || !fam.runtimes) return []
        var out = []
        var seen = ({})
        for (var i = 0; i < fam.runtimes.length; i++) {
            var runtime = fam.runtimes[i]
            if (!runtime.id || seen[runtime.id] || !runtimeInstalled(runtime.id)) continue
            seen[runtime.id] = true
            var item = Object.assign({}, runtime)
            var versions = AppController.runtimes.runtimeVersions(runtime.id)
            var currentVersion = studioContext ? studioContext.runtimeVersion : ""
            item.version = currentVersion
            if (item.version === "" && versions.length > 0) item.version = versions[0].version || ""
            out.push(item)
        }
        return out
    }

    Rectangle {
        id: leftRail
        Layout.preferredWidth: (root.showLeftPanel && root.isLeftPanelOpen) ? 332 : (root.showLeftPanel ? 46 : 0)
        Layout.fillHeight: true
        color: Theme.surface
        clip: true
        visible: root.showLeftPanel

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.surfaceAlt
        }

        Behavior on Layout.preferredWidth {
            NumberAnimation { duration: 180; easing.type: Easing.InOutQuad }
        }

        Item {
            id: leftPanelItem
            anchors.fill: parent
            anchors.margins: root.isLeftPanelOpen ? Theme.paddingLarge : 0
            visible: root.isLeftPanelOpen
        }

        Button {
            id: openLeftRailButton
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: Theme.paddingLarge
            visible: !root.isLeftPanelOpen
            implicitWidth: 30
            implicitHeight: 34
            flat: true

            AppToolTip {
                text: qsTr("Show history")
                visible: parent.hovered
            }

            contentItem: LineIcon {
                name: "history"
                color: openLeftRailButton.hovered ? Theme.accent : Theme.textSecondary
                anchors.centerIn: parent
                width: 18
                height: 18
            }

            background: Rectangle {
                radius: 7
                color: openLeftRailButton.hovered ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(1, 1, 1, 0.025)
                border.color: openLeftRailButton.hovered ? Qt.rgba(0.49, 0.30, 1.0, 0.55) : Qt.rgba(1, 1, 1, 0.08)
                border.width: 1
            }

            onClicked: root.isLeftPanelOpen = true
            HoverHandler { cursorShape: Qt.PointingHandCursor }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: root.railGap
        Layout.rightMargin: root.railGap
        spacing: 0

        StudioHeaderBar {
            family: root.family
            studioTitle: root.studioTitle
            statusText: root.statusText()
            statusColor: root.statusColor()
            currentRuntimeItem: root.currentRuntimeItem()
            modelLoaded: {
                var st = root.statusText()
                return st === "Ready" || st === "Processing"
            }
            processing: root.statusText() === "Processing"
            cpuUsage: studioController ? studioController.cpuUsage : 0
            estimatedRamUsage: studioController ? studioController.estimatedRamUsage : ""
            estimatedVramUsage: studioController ? studioController.estimatedVramUsage : ""
            studioReady: root.studioReady
            backToolTip: root.backToolTip
            runtimeDisplayText: root.modalSelectionMode ? root.modalSelectionValue : ""
            runtimeClickable: root.modalSelectionMode
            onBackClicked: root.requestBack()
            onRuntimeClicked: root.requestConfigurationPicker()
            onReloadRequested: root.requestReload()
            onEjectRequested: root.requestEject()
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.showSwitcher ? 56 : 0
            visible: root.showSwitcher
            color: Theme.surface

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.surfaceAlt
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.paddingLarge
                anchors.rightMargin: Theme.paddingLarge
                spacing: Theme.paddingMedium

                Button {
                    visible: root.modalSelectionMode
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    flat: true
                    onClicked: root.requestConfigurationPicker()

                    contentItem: RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 2
                        anchors.rightMargin: 2
                        spacing: Theme.paddingMedium

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                text: root.modalSelectionTitle
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSmall
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: root.modalSelectionValue
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontMedium
                                font.bold: true
                                elide: Text.ElideRight
                            }
                        }

                        Text {
                            visible: root.modalSelectionDetail !== ""
                            Layout.maximumWidth: Math.max(120, parent.width * 0.32)
                            text: root.modalSelectionDetail
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSmall
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignRight
                        }

                        LineIcon {
                            name: "chevron-down"
                            color: Theme.textSecondary
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                        }
                    }

                    background: Rectangle {
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.03)
                        border.color: Qt.rgba(1, 1, 1, 0.08)
                        border.width: 1
                    }
                }

                RowLayout {
                    visible: !root.modalSelectionMode
                    Layout.fillWidth: true
                    spacing: Theme.paddingMedium

                    Text {
                        text: "Model"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                    }

                    AppComboBox {
                        id: familySelector
                        Layout.preferredWidth: 240
                        model: root.families
                        textRole: "title"
                        valueRole: "id"
                        currentIndex: {
                            for (var i = 0; i < model.length; i++) {
                                if (model[i].id === root.selectedFamilyId) return i
                            }
                            return -1
                        }
                        onActivated: {
                            if (currentValue !== "" && currentValue !== root.selectedFamilyId)
                                root.requestModelSwitch(currentValue)
                        }
                    }

                    Text {
                        text: qsTr("Runtime")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                    }

                    AppComboBox {
                        id: runtimeSelector
                        Layout.preferredWidth: 320
                        model: root.installedRuntimes()
                        textRole: "name"
                        valueRole: "id"
                        currentIndex: {
                            var selected = root.studioContext ? root.studioContext.runtimeId : ""
                            for (var i = 0; i < model.length; i++) {
                                if (model[i].id === selected) return i
                            }
                            return model.length > 0 ? 0 : -1
                        }
                        onActivated: {
                            if (currentValue !== "")
                                root.requestRuntimeSwitch(currentValue)
                        }
                    }
                }
            }
        }

        Rectangle {
            id: mainContentItem
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Qt.darker(Theme.background, 1.04)
        }
    }

    Rectangle {
        id: settingsRail
        Layout.preferredWidth: root.isSettingsOpen ? 332 : 46
        Layout.fillHeight: true
        color: Theme.surface
        clip: true

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.surfaceAlt
        }

        Behavior on Layout.preferredWidth {
            NumberAnimation { duration: 180; easing.type: Easing.InOutQuad }
        }

        Item {
            id: settingsItem
            anchors.fill: parent
            anchors.margins: root.isSettingsOpen ? Theme.paddingLarge : 0
            visible: root.isSettingsOpen
        }

        Button {
            id: openSettingsRailButton
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: Theme.paddingLarge
            visible: !root.isSettingsOpen
            implicitWidth: 30
            implicitHeight: 34
            flat: true

            AppToolTip {
                text: qsTr("Show settings")
                visible: parent.hovered
            }

            contentItem: LineIcon {
                name: "sliders"
                color: openSettingsRailButton.hovered ? Theme.accent : Theme.textSecondary
                anchors.centerIn: parent
                width: 18
                height: 18
            }

            background: Rectangle {
                radius: 7
                color: openSettingsRailButton.hovered ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(1, 1, 1, 0.025)
                border.color: openSettingsRailButton.hovered ? Qt.rgba(0.49, 0.30, 1.0, 0.55) : Qt.rgba(1, 1, 1, 0.08)
                border.width: 1
            }

            onClicked: root.isSettingsOpen = true
            HoverHandler { cursorShape: Qt.PointingHandCursor }
        }
    }
}
