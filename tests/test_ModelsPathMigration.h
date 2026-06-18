#pragma once
#include <QObject>
#include <QTemporaryDir>

namespace LAStudio {

class TestModelsPathMigration : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testModelsPathMigrator();
    void testModelsPathMigrationService();

private:
    QTemporaryDir m_tempDir;
};

} // namespace LAStudio
