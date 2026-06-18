import QtQuick
import QtQuick.Controls
import "../components/shared"
import "../components/stt"
import LAStudio

StudioPageFrame {
    id: sttPageFrame
    capabilityId: "stt"

    contentView: Component {
        SttStudioView {
            sttSession: AppController.sttSession

            onBackToGallery: sttPageFrame.openConfiguration(studioController ? studioController.selectedFamilyId : "")
        }
    }
}
