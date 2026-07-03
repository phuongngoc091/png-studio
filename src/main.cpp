#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QIcon>
#include <QFont>

#include "lastudio/AppVersion.h"
#include "core/Logger.h"
#include "core/QmlLogger.h"
#include "controllers/AppController.h"
#include "controllers/StudioSessionViewModel.h"
#include "core/Settings.h"
#include "core/HFHubClient.h"
#include "core/DownloadManager.h"
#include "core/ModelManager.h"
#include "core/CatalogManager.h"
#include "core/VoiceCloningUtils.h"
#include "stt/SttEngine.h"
#include "tts/TtsEngine.h"
#include "audio/AudioRecorder.h"
#include "audio/AudioPlayer.h"
#include "audio/WaveformProvider.h"
#include "core/HardwareManager.h"


#include <QtQml/qqml.h>

int main(int argc, char *argv[])
{
    // Suppress noisy DirectWrite font warnings about legacy raster fonts
    qputenv("QT_LOGGING_RULES", "qt.gui.font.directwrite=false");

    QGuiApplication app(argc, argv);
    
    // Set a modern default font to avoid falling back to legacy fonts
    app.setFont(QFont(QStringLiteral("Segoe UI"), 10));

    app.setOrganizationName(QStringLiteral(""));
    app.setApplicationName(QStringLiteral("PNG Studio"));
    app.setApplicationVersion(QString::fromLatin1(LASTUDIO_VERSION));
    // Set application window icon from the embedded .ico resource.
    app.setWindowIcon(QIcon(QStringLiteral(":/LAStudio/icons/app_icon.ico")));

    LAStudio::Logger::init();
    LAStudio::Logger::info("App", "Application starting...");

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/"));
 

    auto *controller = LAStudio::AppController::instance();
    if (!controller) {
        controller = LAStudio::AppController::create(&engine, nullptr);
    }
    controller->localization()->setEngine(&engine);
    engine.addImageProvider(QStringLiteral("waveform"), controller->waveformProvider());

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("LAStudio", "Main");

    return app.exec();
}
