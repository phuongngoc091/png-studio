import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LAStudio
import "../shared/settings"
import "../base"

ColumnLayout {
    id: root

    property var family: null
    property var dynamicSettings: ({})
    property string selectedLanguage: "en"
    property string styleInstruction: ""
    property bool denoise: true
    property bool preprocessPrompt: true
    property bool randomSeed: true
    property int customSeed: 42
    property string suggestedLanguage: "en"
    property string backendType: ""  // "kokoro", "vibevoice", or "" (omnivoice)

    property var capabilitySchema: []
    property var basicSchema: []
    property var advancedSchema: []
    property var studioConfig: ({})
    property bool advancedOpen: false
    readonly property bool hasLanguageInput: studioConfig && studioConfig.inputs ? studioConfig.inputs.indexOf("language") !== -1 : false
    readonly property bool hasInstructInput: studioConfig && studioConfig.inputs ? studioConfig.inputs.indexOf("instruct") !== -1 : false

    function splitSchema(schema, advanced) {
        var result = []
        for (var i = 0; i < schema.length; ++i) {
            var item = schema[i] || {}
            if (!!item.advanced === advanced) {
                result.push(item)
            }
        }
        return result
    }

    function refreshCapabilityMetadata() {
        root.capabilitySchema = AppController.tts.schemaForCapability("tts")
        root.studioConfig = AppController.tts.studioConfigForCapability("tts")
        root.basicSchema = splitSchema(root.capabilitySchema, false)
        root.advancedSchema = splitSchema(root.capabilitySchema, true)
        var defaults = {}
        for (var i = 0; i < root.capabilitySchema.length; ++i) {
            var param = root.capabilitySchema[i] || {}
            if (param.id && param["default"] !== undefined) {
                defaults[param.id] = param["default"]
            }
        }
        root.dynamicSettings = defaults
        root.styleInstruction = ""
        instructInput.text = ""
    }

    Component.onCompleted: refreshCapabilityMetadata()

    Connections {
        target: AppController.tts
        function onFamilyConfigChanged() { root.refreshCapabilityMetadata() }
        function onSchemaChanged() { root.refreshCapabilityMetadata() }
    }

    readonly property bool isKokoro: backendType === "kokoro"

    signal settingsChanged()
    signal closeRequested()

    function getSynthesisSettings() {
        var settings = {}
        for (var i = 0; i < root.capabilitySchema.length; ++i) {
            var param = root.capabilitySchema[i] || {}
            if (!param.id) continue
            if (root.dynamicSettings[param.id] !== undefined) {
                settings[param.id] = root.dynamicSettings[param.id]
            } else if (param["default"] !== undefined) {
                settings[param.id] = param["default"]
            }
        }
        if (root.hasLanguageInput) {
            settings["lang"] = root.selectedLanguage
        }
        if (root.hasInstructInput && root.styleInstruction.length > 0) {
            settings["instruct"] = root.styleInstruction
        }
        return settings
    }

    function updateDynamicSetting(parameterId, value) {
        var settings = JSON.parse(JSON.stringify(root.dynamicSettings))
        settings[parameterId] = value
        root.dynamicSettings = settings
        root.settingsChanged()
    }

    function getSettingsObject(inputText, referenceText) {
        return VoiceCloningUtils.buildCloneSettings(
            root.selectedLanguage,
            instructInput.text,
            referenceText,
            denoiseToggle.checked,
            preprocessToggle.checked,
            root.dynamicSettings,
            randomSeedToggle.checked,
            parseInt(customSeedInput.text) || 0
        )
    }

    onSuggestedLanguageChanged: {
        if (root.selectedLanguage !== suggestedLanguage) root.selectedLanguage = suggestedLanguage
    }

    spacing: Theme.paddingMedium
    Layout.fillWidth: true
    Layout.fillHeight: true

    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 36
        spacing: Theme.paddingSmall

        LineIcon {
            name: "sliders"
            color: Theme.accent
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
        }

        Text {
            text: "TTS Settings"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontMedium
            font.bold: true
            Layout.fillWidth: true
        }

        Button {
            id: closeBtn
            implicitWidth: 30
            implicitHeight: 30
            flat: true

            AppToolTip {
                text: "Hide settings"
                visible: parent.hovered
            }

            contentItem: LineIcon {
                name: "chevron-right"
                color: closeBtn.hovered ? Theme.accent : Theme.textSecondary
                anchors.centerIn: parent
                width: 16
                height: 16
            }
            background: Rectangle {
                color: closeBtn.hovered ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(1, 1, 1, 0.025)
                border.color: closeBtn.hovered ? Qt.rgba(0.49, 0.30, 1.0, 0.55) : Qt.rgba(1, 1, 1, 0.08)
                border.width: 1
                radius: 7
            }
            onClicked: root.closeRequested()
            HoverHandler { cursorShape: Qt.PointingHandCursor }
        }
    }

    Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1, 1, 1, 0.07) }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width - 16
            spacing: Theme.paddingMedium

            SettingsSection {
                title: "Core"
                visible: true // Always show Core for language selection if supported
                iconName: "file"

                LanguageSelector {
                    id: langSelector
                    Layout.fillWidth: true
                    labelText: "Target Language"
                    family: root.family
                    hasLanguageInput: root.hasLanguageInput
                    useTextFieldFallback: false
                    language: root.selectedLanguage
                    onLanguageChanged: {
                        if (root.selectedLanguage !== language) {
                            root.selectedLanguage = language
                            root.settingsChanged()
                        }
                    }
                }

                FieldLabel { 
                    text: (family && family.id && family.id.indexOf("qwen3") !== -1) ? "Style Instruction" : "Style & Emotion"
                    visible: instructInput.visible
                }

                TextField {
                    id: instructInput
                    Layout.fillWidth: true
                    visible: root.hasInstructInput
                    placeholderText: (family && family.id && family.id.indexOf("qwen3") !== -1) 
                                     ? "e.g. Speak with excitement, whisper, or deep voice..." 
                                     : "happy, whisper, dramatic..."
                    color: Theme.textPrimary
                    placeholderTextColor: Theme.textSecondary
                    selectionColor: Theme.accent
                    selectedTextColor: "#ffffff"
                    background: Rectangle {
                        color: Qt.rgba(1, 1, 1, 0.035)
                        radius: 7
                        border.color: instructInput.activeFocus ? Qt.rgba(0.49, 0.30, 1.0, 0.75) : Qt.rgba(1, 1, 1, 0.08)
                        border.width: 1
                    }
                    padding: Theme.paddingMedium
                    onTextChanged: {
                        root.styleInstruction = text
                        root.settingsChanged()
                    }
                }
            }

            SettingsSection {
                title: "Model Parameters"
                iconName: "sliders"
                visible: root.basicSchema.length > 0

                ModelParameterControls {
                    schema: root.basicSchema
                    dynamicSettings: root.dynamicSettings
                    onParameterChanged: function(parameterId, value) { root.updateDynamicSetting(parameterId, value) }
                }
            }

            CollapsibleSettingsSection {
                title: "Advanced"
                iconName: "sliders"
                expanded: root.advancedOpen
                visible: root.advancedSchema.length > 0
                onToggled: root.advancedOpen = !root.advancedOpen

                ModelParameterControls {
                    schema: root.advancedSchema
                    dynamicSettings: root.dynamicSettings
                    onParameterChanged: function(parameterId, value) { root.updateDynamicSetting(parameterId, value) }
                }
            }

            SettingsSection {
                title: "Audio & System"
                visible: !root.isKokoro
                iconName: "cpu"

                ToggleRow {
                    id: denoiseToggle
                    text: "Denoise"
                    checked: true
                    onCheckedChanged: {
                        root.denoise = checked
                        root.settingsChanged()
                    }
                }

                ToggleRow {
                    id: preprocessToggle
                    text: "Preprocess prompt"
                    checked: true
                    onCheckedChanged: {
                        root.preprocessPrompt = checked
                        root.settingsChanged()
                    }
                }

                ToggleRow {
                    id: randomSeedToggle
                    text: "Random seed"
                    checked: true
                    onCheckedChanged: {
                        root.randomSeed = checked
                        root.settingsChanged()
                    }
                }

                TextField {
                    id: customSeedInput
                    Layout.fillWidth: true
                    visible: !randomSeedToggle.checked
                    text: "42"
                    color: Theme.textPrimary
                    placeholderTextColor: Theme.textSecondary
                    validator: IntValidator { bottom: 0 }
                    background: Rectangle {
                        color: Qt.rgba(1, 1, 1, 0.035)
                        radius: 7
                        border.color: customSeedInput.activeFocus ? Qt.rgba(0.49, 0.30, 1.0, 0.85) : Qt.rgba(1, 1, 1, 0.08)
                        border.width: 1
                    }
                    padding: Theme.paddingMedium
                    onTextChanged: {
                        root.customSeed = parseInt(text) || 0
                        root.settingsChanged()
                    }
                }
            }

            Item { Layout.fillWidth: true; Layout.preferredHeight: Theme.paddingSmall }
        }
    }
}
