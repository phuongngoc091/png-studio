#pragma once
#include <QObject>

namespace LAStudio {

class TestModelsAndRuntimes : public QObject {
    Q_OBJECT

private slots:
    void cleanupTestCase();
    void testModelManagerConcreteModelDir();
    void testCapabilityFamilyModelSuitability();
    void testVoiceDesignFamiliesExposeRuntimeOptions();
    void testQwen3TtsUsesAutomaticFrameLimit();
    void testQwen3TtsDoesNotExposeUnsupportedLengthScale();
    void testLogViewServicePending();
    void testStudioConfigurationResolver();
};

} // namespace LAStudio
