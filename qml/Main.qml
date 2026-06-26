import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LAStudio

ApplicationWindow {
    id: root

    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 600
    visibility: ApplicationWindow.Maximized
    flags: Qt.Window | Qt.FramelessWindowHint
    readonly property string appName: Qt.application.name && Qt.application.name.length > 0 ? Qt.application.name : "LA Studio"
    readonly property string appVersion: Qt.application.version
    title: appName + " - " + appVersion
    color: Theme.background

    // Error toast
    Popup {
        id: errorPopup
        anchors.centerIn: parent
        width: 400
        padding: Theme.paddingLarge
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: Theme.surface
            radius: Theme.radiusMedium
            border.color: Theme.danger
            border.width: 2
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.paddingMedium

            Text {
                text: qsTr("Error")
                color: Theme.danger
                font.pixelSize: Theme.fontLarge
                font.bold: true
            }
            Text {
                text: AppController.errorMessage
                color: Theme.textPrimary
                font.pixelSize: Theme.fontMedium
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            PrimaryButton {
                text: qsTr("Dismiss")
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    AppController.clearError()
                    errorPopup.close()
                }
            }
        }
    }

    Connections {
        target: AppController
        function onErrorMessageChanged() {
            if (AppController.errorMessage.length > 0)
                errorPopup.open()
        }
    }

    Connections {
        target: Theme
        function onRequestShowDownloads() {
            downloadsPopup.open()
        }
    }

    DownloadsPopup {
        id: downloadsPopup
        x: sidebar.width + 8
        y: root.height - height - 16
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        AppTitleBar {
            window: root
            appName: root.appName
            appVersion: root.appVersion
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Sidebar {
                id: sidebar
                Layout.fillHeight: true
                Layout.preferredWidth: 64
                currentIndex: stack.currentIndex
                downloadsActive: downloadsPopup.opened
                onNavigated: function(routeId) {
                    stack.currentIndex = StudioRouteRegistry.getIndex(routeId)
                    downloadsPopup.close()
                }
                onDownloadsClicked: {
                    if (downloadsPopup.opened) {
                        downloadsPopup.close()
                    } else {
                        downloadsPopup.open()
                    }
                }
            }

            Rectangle {
                width: 1
                Layout.fillHeight: true
                color: Theme.surfaceAlt
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                StackLayout {
                    id: stack
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                WelcomePage {
                    onPageRequested: function(routeId) {
                        stack.currentIndex = StudioRouteRegistry.getIndex(routeId)
                    }
                }
                Loader {
                    id: sttLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: stack.currentIndex === 1 || pendingFamilyId !== ""
                    property string pendingFamilyId: ""
                    sourceComponent: SttPage {
                        id: sttPage
                    }
                    onLoaded: {
                        if (pendingFamilyId !== "") {
                            item.openConfiguration(pendingFamilyId)
                            pendingFamilyId = ""
                        }
                    }
                    function openConfig(familyId) {
                        if (item) {
                            item.openConfiguration(familyId)
                        } else {
                            pendingFamilyId = familyId
                        }
                    }
                }
                Loader {
                    id: ttsLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: stack.currentIndex === 2 || pendingFamilyId !== ""
                    property string pendingFamilyId: ""
                    sourceComponent: TtsPage {
                        id: ttsPage
                    }
                    onLoaded: {
                        if (pendingFamilyId !== "") {
                            item.openConfiguration(pendingFamilyId)
                            pendingFamilyId = ""
                        }
                    }
                    function openConfig(familyId) {
                        if (item) {
                            item.openConfiguration(familyId)
                        } else {
                            pendingFamilyId = familyId
                        }
                    }
                }
                Loader {
                    id: voiceCloningLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: stack.currentIndex === 3 || pendingFamilyId !== ""
                    property string pendingFamilyId: ""
                    sourceComponent: VoiceCloningPage {
                        id: voiceCloningPage
                    }
                    onLoaded: {
                        if (pendingFamilyId !== "") {
                            item.openConfiguration(pendingFamilyId)
                            pendingFamilyId = ""
                        }
                    }
                    function openConfig(familyId) {
                        if (item) {
                            item.openConfiguration(familyId)
                        } else {
                            pendingFamilyId = familyId
                        }
                    }
                }
                Loader {
                    id: voiceDesignLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: stack.currentIndex === 4 || pendingFamilyId !== ""
                    property string pendingFamilyId: ""
                    sourceComponent: VoiceDesignPage {
                        id: voiceDesignPage
                    }
                    onLoaded: {
                        if (pendingFamilyId !== "") {
                            item.openConfiguration(pendingFamilyId)
                            pendingFamilyId = ""
                        }
                    }
                    function openConfig(familyId) {
                        if (item) {
                            item.openConfiguration(familyId)
                        } else {
                            pendingFamilyId = familyId
                        }
                    }
                }
                ModelsPage {
                    onOpenStudioRequested: function(capability, familyId) {
                        var routeId = StudioRouteRegistry.routeForCapability(capability)
                        stack.currentIndex = StudioRouteRegistry.getIndex(routeId)
                        if (routeId === "studio-stt") {
                            sttLoader.openConfig(familyId)
                        } else if (routeId === "studio-tts") {
                            ttsLoader.openConfig(familyId)
                        } else if (routeId === "studio-voice-cloning") {
                            voiceCloningLoader.openConfig(familyId)
                        } else if (routeId === "studio-voice-design") {
                            voiceDesignLoader.openConfig(familyId)
                        }
                    }
                }
                MyModelsPage {
                    onOpenStudioRequested: function(capability, familyId) {
                        var routeId = StudioRouteRegistry.routeForCapability(capability)
                        stack.currentIndex = StudioRouteRegistry.getIndex(routeId)
                        if (routeId === "studio-stt") {
                            sttLoader.openConfig(familyId)
                        } else if (routeId === "studio-tts") {
                            ttsLoader.openConfig(familyId)
                        } else if (routeId === "studio-voice-cloning") {
                            voiceCloningLoader.openConfig(familyId)
                        } else if (routeId === "studio-voice-design") {
                            voiceDesignLoader.openConfig(familyId)
                        }
                    }
                }
                Loader {
                    id: settingsLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: stack.currentIndex === 7
                    sourceComponent: SettingsPage {}
                }
                }

                BottomLogPanel {
                    id: bottomLogPanel
                    Layout.fillWidth: true
                    Layout.preferredHeight: bottomLogPanel.implicitHeight
                    
                    Behavior on Layout.preferredHeight {
                        enabled: !bottomLogPanel.isResizing
                        NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
                    }
                }
            }
        }
    }
}
