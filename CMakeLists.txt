cmake_minimum_required(VERSION 3.21)
project(LAStudio VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(cmake/CompilerOptions.cmake)
message(STATUS "MSVC value: ${MSVC}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

option(ENABLE_SHERPA_ONNX "Enable Sherpa-ONNX support" OFF)
if(ENABLE_SHERPA_ONNX)
    add_compile_definitions(LA_ENABLE_SHERPA_ONNX)
endif()

option(ENABLE_OMNIVOICE "Enable Omnivoice support" ON)
if(ENABLE_OMNIVOICE)
    add_compile_definitions(LA_ENABLE_OMNIVOICE)
endif()

include(cmake/Dependencies.cmake)

# Require per-directory qmldir handling for Qt QML module sources.
qt_policy(SET QTP0004 NEW)

set(LASTUDIO_SOURCES
    src/main.cpp
    src/core/Logger.cpp
    src/core/PathUtils.cpp
    src/core/VoiceCloningUtils.cpp
    src/core/Settings.cpp
    src/core/HFHubClient.cpp
    src/core/DownloadManager.cpp
    src/core/ModelManager.cpp
    src/core/CatalogManager.cpp
    src/core/RegistryManager.cpp
    src/core/RuntimeManager.cpp
    src/core/HardwareManager.cpp
    src/core/QmlLogger.cpp
    src/core/StudioCapabilityRegistry.cpp
    src/core/StudioSelectionRepository.cpp
    src/core/CapabilityFamilyModel.cpp
    src/core/LogViewService.cpp

    src/audio/WavIO.cpp
    src/audio/AudioRecorder.cpp
    src/audio/AudioPlayer.cpp
    src/audio/WaveformProvider.cpp
    src/stt/SttEngine.cpp
    src/stt/SttWorker.cpp
    src/stt/backends/SttBackendFactory.cpp
    src/stt/backends/WhisperSttBackend.cpp
    src/stt/backends/QwenSttBackend.cpp
    src/stt/backends/NemotronSttBackend.cpp
    src/tts/TtsEngine.cpp
    src/tts/TtsWorker.cpp
    src/tts/TtsRequestValidator.cpp
    src/tts/backends/TtsBackendFactory.cpp
    src/tts/backends/KokoroBackend.cpp
    src/tts/backends/Qwen3Backend.cpp
    src/tts/backends/VoxCpm2Backend.cpp
    src/tts/backends/VibevoiceBackend.cpp
    src/tts/backends/SpeechLmBackend.cpp
    src/tts/backends/OmnivoiceBackend.cpp
    src/controllers/AppController.cpp
    src/controllers/AudioPreviewService.cpp
    src/controllers/HistoryService.cpp
    src/controllers/HistoryRepository.cpp
    src/controllers/VoiceDesignPresetService.cpp
    src/controllers/DownloadInstallService.cpp
    src/controllers/ModelsPathMigrator.cpp
    src/controllers/ModelsPathMigrationService.cpp
    src/controllers/FileAccessService.cpp
    src/controllers/StudioActions.cpp
    src/controllers/ModelLifecycleController.cpp
    src/controllers/SttModelSession.cpp
    src/controllers/TtsSharedModelSession.cpp
    src/controllers/ModelSessionRegistry.cpp
    src/controllers/StudioSessionViewModel.cpp
    src/controllers/StudioConfigurationResolver.cpp
    src/controllers/SttSessionController.cpp
    src/controllers/SttAudioDecoder.cpp
    src/core/LocalizationManager.cpp
)

set(LASTUDIO_HEADERS
    src/core/Logger.h
    src/core/PathUtils.h
    src/core/VoiceCloningUtils.h
    src/core/Settings.h
    src/core/HFHubClient.h
    src/core/DownloadManager.h
    src/core/ModelManager.h
    src/core/CatalogManager.h
    src/core/RegistryManager.h
    include/runtimes/CrispKokoroInterface.h
    include/runtimes/CrispVoxCpm2Interface.h
    include/runtimes/CrispVibeVoiceInterface.h
    include/runtimes/SpeechLmTtsInterface.h
    include/runtimes/CrispQwen3SttInterface.h
    include/runtimes/CrispNemotronSttInterface.h
    src/core/RuntimeManager.h
    src/core/HardwareManager.h
    src/core/QmlLogger.h
    src/core/StudioCapabilityDescriptor.h
    src/core/StudioCapabilityRegistry.h
    src/core/StudioSelectionRepository.h
    src/core/CapabilityFamilyModel.h
    src/core/LogViewService.h

    src/audio/WavIO.h
    src/audio/AudioRecorder.h
    src/audio/AudioPlayer.h
    src/audio/WaveformProvider.h
    src/stt/SttEngine.h
    src/stt/SttWorker.h
    src/stt/backends/SttBackend.h
    src/stt/backends/SttBackendFactory.h
    src/stt/backends/WhisperSttBackend.h
    src/stt/backends/QwenSttBackend.h
    src/stt/backends/NemotronSttBackend.h
    src/tts/TtsEngine.h
    src/tts/TtsWorker.h
    src/tts/TtsRequestValidator.h
    src/tts/backends/TtsBackend.h
    src/tts/backends/TtsBackendFactory.h
    src/tts/backends/KokoroBackend.h
    src/tts/backends/Qwen3Backend.h
    src/tts/backends/VoxCpm2Backend.h
    src/tts/backends/VibevoiceBackend.h
    src/tts/backends/SpeechLmBackend.h
    src/tts/backends/OmnivoiceBackend.h
    src/controllers/AppController.h
    src/controllers/AudioPreviewService.h
    src/controllers/HistoryService.h
    src/controllers/HistoryRepository.h
    src/controllers/VoiceDesignPresetService.h
    src/controllers/DownloadInstallService.h
    src/controllers/ModelsPathMigrator.h
    src/controllers/ModelsPathMigrationService.h
    src/controllers/FileAccessService.h
    src/controllers/IStudioAction.h
    src/controllers/StudioActions.h
    src/controllers/IModelSession.h
    src/controllers/ModelLifecycleController.h
    src/controllers/SttModelSession.h
    src/controllers/TtsSharedModelSession.h
    src/controllers/ModelSessionRegistry.h
    src/controllers/StudioSessionViewModel.h
    src/controllers/StudioConfigurationResolver.h
    src/controllers/SttSessionController.h
    src/controllers/SttAudioDecoder.h
    src/core/LocalizationManager.h
)

qt_add_executable(LAStudio ${LASTUDIO_SOURCES} ${LASTUDIO_HEADERS})

qt_add_translations(LAStudio
    TS_FILES
        i18n/lastudio_en.ts
        i18n/lastudio_vi.ts
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/components/shared/StudioRouteRegistry.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(LAStudio
    URI LAStudio
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/pages/HFExplorerPage.qml
        qml/pages/ModelsPage.qml
        qml/pages/MyModelsPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/SttPage.qml
        qml/pages/TtsPage.qml
        qml/pages/VoiceCloningPage.qml
        qml/pages/VoiceDesignPage.qml
        qml/pages/WelcomePage.qml
        qml/pages/settings/GeneralSettingsTab.qml
        qml/pages/settings/RuntimeSettingsTab.qml
        qml/pages/settings/HardwareSettingsTab.qml
        qml/pages/settings/SystemLogsTab.qml

        qml/components/Sidebar.qml
        qml/components/AppTitleBar.qml
        qml/components/AudioDropArea.qml
        qml/components/WaveformView.qml
        qml/components/RuntimeVersionManagerDialog.qml
        qml/components/base/AppComboBox.qml
        qml/components/base/AppContextMenu.qml
        qml/components/base/ConfirmationDialog.qml
        qml/components/base/ModelDependenciesDialog.qml
        qml/components/base/AppTabButton.qml
        qml/components/base/AppTextArea.qml
        qml/components/base/AppToolTip.qml
        qml/components/base/LineIcon.qml
        qml/components/PrimaryButton.qml
        qml/components/SearchBar.qml
        qml/components/ModelCard.qml
        qml/components/ModelListItem.qml
        qml/components/ModelDetailsView.qml
        qml/components/DownloadProgressBar.qml
        qml/components/DownloadsPopup.qml
        qml/components/voicecloning/VoiceSettingsPanel.qml
        qml/components/voicecloning/VoiceCloningStudioView.qml
        qml/components/voicecloning/ReferenceInputBox.qml
        qml/components/voicecloning/InputSourceTabs.qml
        qml/components/voicecloning/AudioDisplayBox.qml
        qml/components/voicecloning/ModelSelectorPopup.qml
        qml/components/voicecloning/VoiceCloningSetupPanel.qml
        qml/components/shared/StudioHeaderBar.qml
        qml/components/shared/GeneratedAudioOutput.qml
        qml/components/shared/CapabilityGallery.qml
        qml/components/shared/StudioContext.qml
        qml/components/shared/StudioShell.qml
        qml/components/shared/StudioPageFrame.qml
        qml/components/shared/StudioRouteRegistry.qml
        qml/components/shared/PromptTagPicker.qml
        qml/components/shared/AudioInputSourcePicker.qml
        qml/components/shared/settings/CollapsibleSettingsSection.qml
        qml/components/shared/settings/LanguageSelector.qml
        qml/components/shared/settings/ModelParameterControls.qml
        qml/components/shared/settings/SettingsSection.qml
        qml/components/shared/settings/ParameterSlider.qml
        qml/components/shared/settings/ToggleRow.qml
        qml/components/shared/settings/FieldLabel.qml
        qml/components/shared/settings/SettingsCard.qml
        qml/components/shared/settings/SettingRow.qml
        qml/components/tts/TtsHistoryPanel.qml
        qml/components/tts/TtsSettingsPanel.qml
        qml/components/tts/TtsStudioView.qml
        qml/components/voicedesign/VoiceDesignHistoryPanel.qml
        qml/components/voicedesign/VoiceDesignSettingsPanel.qml
        qml/components/voicedesign/VoiceDesignStudioView.qml
        qml/components/voicedesign/VoicePresetPanel.qml
        qml/components/stt/SttHistoryPanel.qml
        qml/components/stt/SttInputSection.qml
        qml/components/stt/SttTranscriptionView.qml
        qml/components/stt/SttStudioView.qml
        qml/components/stt/SttSettingsPanel.qml
    RESOURCES
        data/catalog.json
        data/registry_schema.sql
        data/language-sets/omnivoice-v1.json
        data/language-sets/whisper-v1.json
        data/language-sets/nemotron-3.5-v1.json
        data/language-sets/default.json
        icons/app_icon.ico
        icons/app_icon_16.png
        icons/app_icon_32.png
        icons/app_icon_48.png
        icons/app_icon_128.png
        icons/app_icon_256.png
    RESOURCE_PREFIX /
)

target_include_directories(LAStudio PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/audio
    ${CMAKE_SOURCE_DIR}/src/stt
    ${CMAKE_SOURCE_DIR}/src/tts
    ${CMAKE_SOURCE_DIR}/src/controllers
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/whisper/include
)


if(ENABLE_OMNIVOICE)
    target_include_directories(LAStudio PRIVATE
        ${CMAKE_SOURCE_DIR}/omnivoice.cpp/src
    )
endif()

target_link_libraries(LAStudio PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Network
    Qt6::Concurrent
    Qt6::Sql
    CURL::libcurl
)

# Unit tests build.
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(WIN32)
    target_compile_definitions(LAStudio PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(LAStudio PRIVATE dxgi)
endif()



# Omnivoice is loaded dynamically at runtime via OmnivoiceInterface

set_target_properties(LAStudio PROPERTIES
    OUTPUT_NAME "LA Studio"
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)

if(WIN32)
    # If an .rc resource exists (and an .ico) then add it so the built .exe has an embedded icon
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/app_icon.rc")
        message(STATUS "Found resources/app_icon.rc — adding to LAStudio sources for Windows exe icon")
        target_sources(LAStudio PRIVATE "${CMAKE_SOURCE_DIR}/resources/app_icon.rc")
    else()
        message(STATUS "No resources/app_icon.rc found — skipping .exe icon embedding")
    endif()
endif()

# --- Installation Rules ---
include(GNUInstallDirs)

# Install the executable
install(TARGETS LAStudio
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install resources (if any outside of QML resources)
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    install(DIRECTORY resources DESTINATION .)
endif()

# Note: Runtime DLLs (Whisper, etc.) and windeployqt will be handled 
# by the CI/CD script or a post-install step.

