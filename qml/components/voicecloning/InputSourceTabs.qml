import QtQuick
import LAStudio
import "../shared"

AudioInputSourcePicker {
    id: root

    property string audioPath: ""

    signal audioLoaded(string path)

    audioLabel: "Reference audio"
    audioHint: "24kHz mono WAV recommended"
    fileDialogTitle: "Select Reference Audio"
    fileNameFilters: ["Audio files (*.wav *.mp3 *.m4a *.flac)"]
    showSystemSource: true
    recording: AppController.recorder.recording
    saving: AppController.recorder.saving
    recordingLevel: AppController.recorder.level

    onAudioSelected: function(path) {
        root.audioPath = path
        root.audioLoaded(path)
    }

    onStartRecordingRequested: function(systemAudio) {
        AppController.recorder.recordSystemAudio = systemAudio
        AppController.recorder.start()
    }

    onStopRecordingRequested: {
        AppController.recorder.stop()
        AppController.recorder.saveLastRecordingToCacheAsync(24000)
    }

    Connections {
        target: AppController.recorder

        function onRecordingSaved(path) {
            if (path !== "") {
                root.audioPath = path
                root.audioLoaded(path)
            }
        }
    }
}
