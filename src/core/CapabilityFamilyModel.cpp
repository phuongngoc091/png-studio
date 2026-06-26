#include "CapabilityFamilyModel.h"
#include "core/RegistryManager.h"
#include "core/ModelManager.h"
#include "core/RuntimeManager.h"
#include "core/HardwareManager.h"
#include "core/Settings.h"
#include "core/PathUtils.h"
#include "core/Logger.h"
#include "core/StudioCapabilityRegistry.h"
#include "controllers/AppController.h"
#include <algorithm>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QSet>
#include <QLocale>
#include <QVersionNumber>

namespace LAStudio {

namespace {

QVariantList asList(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return {};
    if (value.typeId() == QMetaType::QVariantList || value.typeId() == QMetaType::QStringList)
        return value.toList();
    const QString text = value.toString().trimmed();
    return text.isEmpty() ? QVariantList{} : QVariantList{text};
}

QStringList uniqueStrings(const QVariantList &values)
{
    QStringList out;
    QSet<QString> seen;
    for (const QVariant &value : values) {
        QString text = value.toString().trimmed();
        if (text.isEmpty())
            continue;
        const QString key = text.toLower();
        if (seen.contains(key))
            continue;
        seen.insert(key);
        out.append(text);
    }
    return out;
}

QString displayToken(const QString &value)
{
    const QString text = value.trimmed();
    const QString lower = text.toLower();
    if (lower == QStringLiteral("gguf") ||
        lower == QStringLiteral("ggml") ||
        lower == QStringLiteral("onnx") ||
        lower == QStringLiteral("tts") ||
        lower == QStringLiteral("stt")) {
        return text.toUpper();
    }
    if (lower == QStringLiteral("styletts2"))
        return QStringLiteral("StyleTTS2");
    if (lower == QStringLiteral("voice-cloning"))
        return QStringLiteral("Voice Cloning");
    if (lower == QStringLiteral("tool-use"))
        return QStringLiteral("Tool Use");
    if (text.size() <= 3)
        return text.toUpper();
    return text;
}

QString joinTokens(const QVariantList &values)
{
    QStringList out;
    for (const QString &value : uniqueStrings(values))
        out.append(displayToken(value));
    return out.join(QStringLiteral(", "));
}

bool isVoxCpm2Family(const QVariantMap &family)
{
    const QStringList keys = {
        QStringLiteral("id"),
        QStringLiteral("familyId"),
        QStringLiteral("modelId"),
        QStringLiteral("realId"),
        QStringLiteral("localDir")
    };
    for (const QString &key : keys) {
        if (family.value(key).toString().contains(QStringLiteral("voxcpm2"), Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool isFullPrecisionVoxCpm2Candidate(const QString &filename, double sizeGb)
{
    const QString lower = filename.toLower();
    if (lower.contains(QStringLiteral("f16")) ||
        lower.contains(QStringLiteral("fp16"))) {
        return true;
    }
    return sizeGb >= 3.0;
}

QVariantMap metadataMap(const QVariantMap &family)
{
    QVariantMap metadata = family.value(QStringLiteral("metadataOverrides")).toMap();
    if (!metadata.isEmpty())
        return metadata;
    metadata = family.value(QStringLiteral("metadata")).toMap();
    if (!metadata.isEmpty())
        return metadata;
    return family.value(QStringLiteral("modelYaml")).toMap().value(QStringLiteral("metadataOverrides")).toMap();
}

QVariantMap badge(const QString &label, const QString &value, const QString &tone)
{
    QVariantMap item;
    item.insert(QStringLiteral("label"), label);
    item.insert(QStringLiteral("value"), value);
    item.insert(QStringLiteral("tone"), tone);
    return item;
}

QVariantMap capabilityBadge(const QString &value)
{
    QVariantMap item;
    item.insert(QStringLiteral("value"), displayToken(value));
    item.insert(QStringLiteral("tone"), QStringLiteral("success"));
    return item;
}

QString compactCount(qint64 value)
{
    if (value >= 1000000000)
        return QString::number(value / 1000000000.0, 'f', value >= 10000000000LL ? 0 : 1) + QStringLiteral("B");
    if (value >= 1000000)
        return QString::number(value / 1000000.0, 'f', value >= 10000000 ? 0 : 1) + QStringLiteral("M");
    if (value >= 10000)
        return QString::number(value / 1000.0, 'f', value >= 100000 ? 0 : 1) + QStringLiteral("K");
    return QLocale().toString(value);
}

QVariantMap statBadge(const QString &label, qint64 value, const QString &tone, const QString &source)
{
    QVariantMap item = badge(label, compactCount(value), tone);
    item.insert(QStringLiteral("rawValue"), value);
    item.insert(QStringLiteral("source"), source);
    return item;
}

QVariantList formatValues(const QVariantMap &family, const QVariantMap &metadata)
{
    QVariantList values = asList(metadata.value(QStringLiteral("compatibilityTypes")));
    if (!values.isEmpty())
        return values;

    const QVariantList tags = asList(family.value(QStringLiteral("tags")));
    for (const QVariant &tag : tags) {
        const QString lower = tag.toString().toLower();
        if (lower == QStringLiteral("gguf") ||
            lower == QStringLiteral("ggml") ||
            lower == QStringLiteral("onnx") ||
            lower == QStringLiteral("safetensors")) {
            values.append(tag);
        }
    }
    return values;
}

QVariantList modelInfoBadges(const QVariantMap &family)
{
    const QVariantMap metadata = metadataMap(family);
    QVariantList params = asList(metadata.value(QStringLiteral("paramsStrings")));
    QVariantList architectures = asList(metadata.value(QStringLiteral("architectures")));
    QVariantList domain = asList(metadata.value(QStringLiteral("domain")));

    if (params.isEmpty())
        params = asList(family.value(QStringLiteral("params")));
    if (architectures.isEmpty())
        architectures = asList(family.value(QStringLiteral("architectures")));
    if (domain.isEmpty())
        domain = asList(family.value(QStringLiteral("type")));

    QVariantList out;
    const QString paramsText = joinTokens(params);
    const QString archText = joinTokens(architectures);
    const QString domainText = joinTokens(domain);
    const QString contextText = joinTokens(asList(metadata.value(QStringLiteral("contextLengths"))));
    const QString formatText = joinTokens(formatValues(family, metadata));

    if (!paramsText.isEmpty())
        out.append(badge(QStringLiteral("Params"), paramsText, QStringLiteral("neutral")));
    if (!archText.isEmpty())
        out.append(badge(QStringLiteral("Arch"), archText, QStringLiteral("neutral")));
    if (!domainText.isEmpty())
        out.append(badge(QStringLiteral("Domain"), domainText, QStringLiteral("accent")));
    if (!contextText.isEmpty())
        out.append(badge(QStringLiteral("Context"), contextText, QStringLiteral("info")));
    if (!formatText.isEmpty())
        out.append(badge(QStringLiteral("Format"), formatText, QStringLiteral("format")));
    return out;
}

bool metadataFlag(const QVariantMap &metadata, const QString &key)
{
    const QVariant value = metadata.value(key);
    if (!value.isValid() || value.isNull())
        return false;
    if (value.typeId() == QMetaType::Bool)
        return value.toBool();
    return value.toString().compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
}

QVariantList capabilityBadges(const QVariantMap &family)
{
    const QVariantMap metadata = metadataMap(family);
    QVariantList values = asList(metadata.value(QStringLiteral("capabilities")));
    if (values.isEmpty())
        values = asList(family.value(QStringLiteral("capabilities")));
    if (metadataFlag(metadata, QStringLiteral("vision")))
        values.append(QStringLiteral("Vision"));
    if (metadataFlag(metadata, QStringLiteral("trainedForToolUse")))
        values.append(QStringLiteral("Tool Use"));
    if (metadataFlag(metadata, QStringLiteral("reasoning")))
        values.append(QStringLiteral("Reasoning"));

    QVariantList out;
    for (const QString &value : uniqueStrings(values))
        out.append(capabilityBadge(value));
    return out;
}

QVariantList statsBadges(const QVariantMap &family)
{
    const QVariantMap stats = family.value(QStringLiteral("stats")).toMap();
    QVariantList out;

    qint64 downloads = stats.value(QStringLiteral("displayDownloads")).toLongLong();
    if (downloads <= 0)
        downloads = stats.value(QStringLiteral("localDownloads")).toLongLong();
    if (downloads > 0) {
        out.append(statBadge(
            QStringLiteral("HF Downloads"),
            downloads,
            QStringLiteral("info"),
            QStringLiteral("Original Hugging Face model")));
    }

    const qint64 upstreamLikes = stats.value(QStringLiteral("upstreamLikes")).toLongLong();
    if (upstreamLikes > 0) {
        out.append(statBadge(
            QStringLiteral("HF Stars"),
            upstreamLikes,
            QStringLiteral("accent"),
            QStringLiteral("Original Hugging Face model")));
    }

    return out;
}

QString familyCapability(const QVariantMap &family)
{
    const QVariantList capabilities = family.value(QStringLiteral("capabilities")).toList();
    if (capabilities.contains(QStringLiteral("stt")))
        return QStringLiteral("stt");
    if (capabilities.contains(QStringLiteral("voice-cloning")))
        return QStringLiteral("voice-cloning");
    return QStringLiteral("tts");
}

QString familyIconName(const QVariantMap &family)
{
    const QString capability = familyCapability(family);
    if (capability == QStringLiteral("stt"))
        return QStringLiteral("mic");
    if (capability == QStringLiteral("voice-cloning"))
        return QStringLiteral("users");
    return QStringLiteral("volume");
}

QString modelCardUrl(const QVariantMap &family)
{
    const QString explicitUrl = family.value(QStringLiteral("modelCardUrl")).toString();
    if (!explicitUrl.isEmpty())
        return explicitUrl;
    const QString source = family.value(QStringLiteral("source")).toString();
    if (!source.isEmpty())
        return source;
    const QVariantMap manifest = family.value(QStringLiteral("hubFiles")).toMap().value(QStringLiteral("manifest")).toMap();
    const QString manifestSource = manifest.value(QStringLiteral("source")).toString();
    if (!manifestSource.isEmpty())
        return manifestSource;
    const QString modelId = !family.value(QStringLiteral("modelId")).toString().isEmpty()
        ? family.value(QStringLiteral("modelId")).toString()
        : (!family.value(QStringLiteral("realId")).toString().isEmpty()
            ? family.value(QStringLiteral("realId")).toString()
            : family.value(QStringLiteral("id")).toString());
    return modelId.isEmpty() ? QString() : QStringLiteral("https://huggingface.co/") + modelId;
}

QString readmeContent(const QVariantMap &family)
{
    const QVariantMap hubReadme = family.value(QStringLiteral("hubFiles")).toMap().value(QStringLiteral("readme")).toMap();
    const QString hubContent = hubReadme.value(QStringLiteral("content")).toString();
    if (!hubContent.isEmpty())
        return hubContent;

    const QVariant readme = family.value(QStringLiteral("readme"));
    if (readme.typeId() == QMetaType::QVariantMap)
        return readme.toMap().value(QStringLiteral("content")).toString();
    return readme.toString();
}

QString thumbnailSource(const QVariantMap &family)
{
    const QVariantMap thumbnail = family.value(QStringLiteral("hubFiles")).toMap().value(QStringLiteral("thumbnail")).toMap();
    const QString base64 = thumbnail.value(QStringLiteral("base64")).toString();
    if (base64.isEmpty())
        return {};
    const QString mimeType = thumbnail.value(QStringLiteral("mimeType")).toString().isEmpty()
        ? QStringLiteral("image/png")
        : thumbnail.value(QStringLiteral("mimeType")).toString();
    return QStringLiteral("data:%1;base64,%2").arg(mimeType, base64);
}

QString runtimeDescription(const QVariantMap &runtime)
{
    const QString label = (runtime.value(QStringLiteral("label")).toString() + QStringLiteral(" ") +
                           runtime.value(QStringLiteral("name")).toString()).toLower();
    if (label.contains(QStringLiteral("cuda")))
        return QStringLiteral("NVIDIA CUDA accelerated inference engine");
    if (label.contains(QStringLiteral("vulkan")))
        return QStringLiteral("Vulkan GPU accelerated inference engine");
    return QStringLiteral("CPU-only inference engine");
}

QString runtimeIconName(const QVariantMap &runtime)
{
    return runtime.value(QStringLiteral("label")).toString() == QStringLiteral("CPU")
        ? QStringLiteral("cpu")
        : QStringLiteral("spark");
}

QPair<QString, QString> preferredRuntime(const QVariantList &runtimeOptions)
{
    QString fallbackId;
    QString fallbackVersion;
    for (const QVariant &value : runtimeOptions) {
        const QVariantMap option = value.toMap();
        if (fallbackId.isEmpty()) {
            fallbackId = option.value(QStringLiteral("id")).toString();
            fallbackVersion = option.value(QStringLiteral("version")).toString();
        }
        if (option.value(QStringLiteral("compatible")).toBool() &&
            option.value(QStringLiteral("installed")).toBool()) {
            return {option.value(QStringLiteral("id")).toString(),
                    option.value(QStringLiteral("version")).toString()};
        }
    }
    return {fallbackId, fallbackVersion};
}

QVersionNumber parsedRuntimeVersion(QString version)
{
    version = version.trimmed();
    if (version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        version.remove(0, 1);
    return QVersionNumber::fromString(version);
}

int compareRuntimeVersions(const QString &left, const QString &right)
{
    const QVersionNumber leftVersion = parsedRuntimeVersion(left);
    const QVersionNumber rightVersion = parsedRuntimeVersion(right);
    if (!leftVersion.isNull() && !rightVersion.isNull())
        return QVersionNumber::compare(leftVersion, rightVersion);
    return QString::compare(left, right, Qt::CaseInsensitive);
}

bool runtimeVersionGreater(const QString &left, const QString &right)
{
    if (right.isEmpty())
        return !left.isEmpty();
    return compareRuntimeVersions(left, right) > 0;
}

bool installedRuntimeVersion(const QVariantList &installedVersions, const QString &version)
{
    if (version.isEmpty())
        return !installedVersions.isEmpty();
    for (const QVariant &installed : installedVersions) {
        if (installed.toMap().value(QStringLiteral("version")).toString() == version)
            return true;
    }
    return false;
}

QString statusKind(bool ready, const QString &statusReason)
{
    if (ready)
        return QStringLiteral("ready");
    if (statusReason == QStringLiteral("Incompatible"))
        return QStringLiteral("incompatible");
    return QStringLiteral("setup");
}

bool familySupportsLanguage(const QVariantMap &family, const QString &langFilter)
{
    if (langFilter.isEmpty() || langFilter.compare(QStringLiteral("all"), Qt::CaseInsensitive) == 0) {
        return true;
    }

    const QString targetLang = langFilter.trimmed().toLower();

    // 1. Check supportedLanguages
    const QVariantList supportedLangs = family.value(QStringLiteral("supportedLanguages")).toList();
    for (const QVariant &val : supportedLangs) {
        if (val.typeId() == QMetaType::QVariantMap) {
            const QVariantMap m = val.toMap();
            if (m.value(QStringLiteral("value")).toString().trimmed().toLower() == targetLang) {
                return true;
            }
        } else if (val.toString().trimmed().toLower() == targetLang) {
            return true;
        }
    }

    // 2. Check featuredLanguages
    const QVariantList featuredLangs = family.value(QStringLiteral("featuredLanguages")).toList();
    for (const QVariant &val : featuredLangs) {
        if (val.typeId() == QMetaType::QVariantMap) {
            const QVariantMap m = val.toMap();
            if (m.value(QStringLiteral("value")).toString().trimmed().toLower() == targetLang) {
                return true;
            }
        } else if (val.toString().trimmed().toLower() == targetLang) {
            return true;
        }
    }

    // 3. Check supportedLanguageSetId
    const QString setId = family.value(QStringLiteral("supportedLanguageSetId")).toString();
    if (!setId.isEmpty()) {
        AppController *app = AppController::instance();
        if (app && app->catalog()) {
            const QVariantList setLangs = app->catalog()->languageSet(setId);
            for (const QVariant &val : setLangs) {
                if (val.typeId() == QMetaType::QVariantMap) {
                    const QVariantMap m = val.toMap();
                    if (m.value(QStringLiteral("code")).toString().trimmed().toLower() == targetLang) {
                        return true;
                    }
                } else if (val.toString().trimmed().toLower() == targetLang) {
                    return true;
                }
            }
        }
    }

    return false;
}

void collectLanguagesForFamily(const QVariantMap &family, QList<QPair<QString, QString>> &langs)
{
    // 1. Check supportedLanguages
    const QVariantList supportedLangs = family.value(QStringLiteral("supportedLanguages")).toList();
    for (const QVariant &val : supportedLangs) {
        if (val.typeId() == QMetaType::QVariantMap) {
            const QVariantMap m = val.toMap();
            const QString code = m.value(QStringLiteral("value")).toString().trimmed();
            const QString name = m.value(QStringLiteral("text")).toString().trimmed();
            if (!code.isEmpty()) {
                langs.append({code, name.isEmpty() ? code : name});
            }
        } else {
            const QString code = val.toString().trimmed();
            if (!code.isEmpty()) {
                langs.append({code, code});
            }
        }
    }

    // 2. Check featuredLanguages
    const QVariantList featuredLangs = family.value(QStringLiteral("featuredLanguages")).toList();
    for (const QVariant &val : featuredLangs) {
        if (val.typeId() == QMetaType::QVariantMap) {
            const QVariantMap m = val.toMap();
            const QString code = m.value(QStringLiteral("value")).toString().trimmed();
            const QString name = m.value(QStringLiteral("text")).toString().trimmed();
            if (!code.isEmpty()) {
                langs.append({code, name.isEmpty() ? code : name});
            }
        } else {
            const QString code = val.toString().trimmed();
            if (!code.isEmpty()) {
                langs.append({code, code});
            }
        }
    }

    // 3. Check supportedLanguageSetId
    const QString setId = family.value(QStringLiteral("supportedLanguageSetId")).toString();
    if (!setId.isEmpty()) {
        AppController *app = AppController::instance();
        if (app && app->catalog()) {
            const QVariantList setLangs = app->catalog()->languageSet(setId);
            for (const QVariant &val : setLangs) {
                if (val.typeId() == QMetaType::QVariantMap) {
                    const QVariantMap m = val.toMap();
                    const QString code = m.value(QStringLiteral("code")).toString().trimmed();
                    const QString name = m.value(QStringLiteral("name")).toString().trimmed();
                    if (!code.isEmpty()) {
                        langs.append({code, name.isEmpty() ? code : name});
                    }
                } else {
                    const QString code = val.toString().trimmed();
                    if (!code.isEmpty()) {
                        langs.append({code, code});
                    }
                }
            }
        }
    }
}

}

CapabilityFamilyModel::CapabilityFamilyModel(ModelManager *models, RuntimeManager *runtimes, RegistryManager *registry, Settings *settings, QObject *parent)
    : QAbstractListModel(parent)
    , m_models(models)
    , m_runtimes(runtimes)
    , m_registry(registry)
    , m_settings(settings)
{
    if (m_models) {
        connect(m_models, &ModelManager::registryUpdated, this, &CapabilityFamilyModel::refresh);
    }
    if (m_runtimes) {
        connect(m_runtimes, &RuntimeManager::registryUpdated, this, &CapabilityFamilyModel::refresh);
    }
    if (m_settings) {
        connect(m_settings, &Settings::selectedTtsRuntimeChanged, this, &CapabilityFamilyModel::refresh);
        connect(m_settings, &Settings::selectedTtsRuntimeVersionChanged, this, &CapabilityFamilyModel::refresh);
        connect(m_settings, &Settings::selectedSttRuntimeChanged, this, &CapabilityFamilyModel::refresh);
        connect(m_settings, &Settings::selectedSttRuntimeVersionChanged, this, &CapabilityFamilyModel::refresh);
    }
}

int CapabilityFamilyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QHash<int, QByteArray> CapabilityFamilyModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FamilyIdRole] = "familyId";
    roles[DisplayNameRole] = "displayName";
    roles[SubtitleRole] = "subtitle";
    roles[DescriptionRole] = "description";
    roles[AccentRole] = "accent";
    roles[SupportedRole] = "supported";
    roles[InstalledRole] = "installed";
    roles[SelectedRole] = "selected";
    roles[ReadyRole] = "ready";
    roles[StatusReasonRole] = "statusReason";
    roles[SelectedRuntimeIdRole] = "selectedRuntimeId";
    roles[SelectedRuntimeVersionRole] = "selectedRuntimeVersion";
    roles[RuntimeOptionsRole] = "runtimeOptions";
    roles[MissingRequirementsRole] = "missingRequirements";
    roles[RequiredFilesRole] = "requiredFiles";
    roles[SelectedFilesRole] = "selectedFiles";
    roles[RawMetadataRole] = "rawMetadata";
    roles[ModelCardUrlRole] = "modelCardUrl";
    roles[ReadmeContentRole] = "readmeContent";
    roles[ThumbnailSourceRole] = "thumbnailSource";
    roles[IconNameRole] = "iconName";
    roles[FamilyCapabilityRole] = "familyCapability";
    roles[StatusKindRole] = "statusKind";
    roles[StatusTitleRole] = "statusTitle";
    roles[InfoBadgesRole] = "infoBadges";
    roles[CapabilityBadgesRole] = "capabilityBadges";
    roles[StatsBadgesRole] = "statsBadges";
    roles[PreferredRuntimeIdRole] = "preferredRuntimeId";
    roles[PreferredRuntimeVersionRole] = "preferredRuntimeVersion";
    roles[IsLastudioPickRole] = "isLastudioPick";
    roles[PickLabelRole] = "pickLabel";
    roles[PickReasonRole] = "pickReason";
    return roles;
}

QVariant CapabilityFamilyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size()) return {};

    const FamilyItem &item = m_items.at(index.row());
    switch (role) {
    case FamilyIdRole: return item.id;
    case DisplayNameRole: return item.displayName;
    case SubtitleRole: return item.subtitle;
    case DescriptionRole: return item.description;
    case AccentRole: return item.accent;
    case SupportedRole: return item.supported;
    case InstalledRole: return item.installed;
    case SelectedRole: return item.selected;
    case ReadyRole: return item.ready;
    case StatusReasonRole: return item.statusReason;
    case SelectedRuntimeIdRole: return item.selectedRuntimeId;
    case SelectedRuntimeVersionRole: return item.selectedRuntimeVersion;
    case RuntimeOptionsRole: return item.runtimeOptions;
    case MissingRequirementsRole: return item.missingRequirements;
    case RequiredFilesRole: return item.requiredFiles;
    case SelectedFilesRole: return item.selectedFiles;
    case RawMetadataRole: return item.rawMap;
    case ModelCardUrlRole: return item.modelCardUrl;
    case ReadmeContentRole: return item.readmeContent;
    case ThumbnailSourceRole: return item.thumbnailSource;
    case IconNameRole: return item.iconName;
    case FamilyCapabilityRole: return item.familyCapability;
    case StatusKindRole: return item.statusKind;
    case StatusTitleRole: return item.statusTitle;
    case InfoBadgesRole: return item.infoBadges;
    case CapabilityBadgesRole: return item.capabilityBadges;
    case StatsBadgesRole: return item.statsBadges;
    case PreferredRuntimeIdRole: return item.preferredRuntimeId;
    case PreferredRuntimeVersionRole: return item.preferredRuntimeVersion;
    case IsLastudioPickRole: return item.isLastudioPick;
    case PickLabelRole: return item.pickLabel;
    case PickReasonRole: return item.pickReason;
    }
    return {};
}

void CapabilityFamilyModel::setCapability(const QString &capabilityId)
{
    if (m_capabilityId == capabilityId) return;
    m_capabilityId = capabilityId;
    m_languageFilter = QStringLiteral("all");
    emit languageFilterChanged();
    refresh();
}

void CapabilityFamilyModel::setLanguageFilter(const QString &languageCode)
{
    if (m_languageFilter == languageCode) return;
    m_languageFilter = languageCode;
    emit languageFilterChanged();
    refresh();
}

void CapabilityFamilyModel::setSearchText(const QString &searchText)
{
    if (m_searchText == searchText) return;
    m_searchText = searchText;
    refresh();
}

void CapabilityFamilyModel::setStatusFilter(const QString &filterType)
{
    if (m_statusFilter == filterType) return;
    m_statusFilter = filterType;
    refresh();
}

void CapabilityFamilyModel::setSelectedFamilyId(const QString &familyId)
{
    if (m_selectedFamilyId == familyId) return;
    m_selectedFamilyId = familyId;

    // Update selection state in items
    for (int i = 0; i < m_items.size(); ++i) {
        bool selected = (m_items[i].id == m_selectedFamilyId);
        if (m_items[i].selected != selected) {
            m_items[i].selected = selected;
            emit dataChanged(this->index(i), this->index(i), {SelectedRole});
        }
    }
}

void CapabilityFamilyModel::refresh()
{
    beginResetModel();
    updateItems();
    endResetModel();
    ++m_revision;
    emit revisionChanged();
}

QVariantMap CapabilityFamilyModel::toVariantMap(const FamilyItem &item) const
{
    return {
        {QStringLiteral("familyId"), item.id},
        {QStringLiteral("displayName"), item.displayName},
        {QStringLiteral("subtitle"), item.subtitle},
        {QStringLiteral("description"), item.description},
        {QStringLiteral("accent"), item.accent},
        {QStringLiteral("supported"), item.supported},
        {QStringLiteral("installed"), item.installed},
        {QStringLiteral("ready"), item.ready},
        {QStringLiteral("statusReason"), item.statusReason},
        {QStringLiteral("runtimeOptions"), item.runtimeOptions},
        {QStringLiteral("missingRequirements"), item.missingRequirements},
        {QStringLiteral("requiredFiles"), item.requiredFiles},
        {QStringLiteral("selectedFiles"), item.selectedFiles},
        {QStringLiteral("rawMetadata"), item.rawMap},
        {QStringLiteral("modelCardUrl"), item.modelCardUrl},
        {QStringLiteral("readmeContent"), item.readmeContent},
        {QStringLiteral("thumbnailSource"), item.thumbnailSource},
        {QStringLiteral("iconName"), item.iconName},
        {QStringLiteral("familyCapability"), item.familyCapability},
        {QStringLiteral("statusKind"), item.statusKind},
        {QStringLiteral("statusTitle"), item.statusTitle},
        {QStringLiteral("infoBadges"), item.infoBadges},
        {QStringLiteral("capabilityBadges"), item.capabilityBadges},
        {QStringLiteral("statsBadges"), item.statsBadges},
        {QStringLiteral("preferredRuntimeId"), item.preferredRuntimeId},
        {QStringLiteral("preferredRuntimeVersion"), item.preferredRuntimeVersion},
        {QStringLiteral("selectedRuntimeId"), item.selectedRuntimeId},
        {QStringLiteral("selectedRuntimeVersion"), item.selectedRuntimeVersion},
        {QStringLiteral("isLastudioPick"), item.isLastudioPick},
        {QStringLiteral("pickLabel"), item.pickLabel},
        {QStringLiteral("pickReason"), item.pickReason}
    };
}

QVariantMap CapabilityFamilyModel::itemForFamily(const QString &familyId) const
{
    for (const FamilyItem &item : m_items) {
        if (item.id == familyId) {
            return toVariantMap(item);
        }
    }
    return {};
}

QString CapabilityFamilyModel::firstFamilyId() const
{
    return m_items.isEmpty() ? QString() : m_items.first().id;
}

bool CapabilityFamilyModel::requirementRequiredForCapability(const QVariantMap &req, const QString &capability) const
{
    if (req.isEmpty()) return false;
    if (req.contains(QStringLiteral("required"))) {
        return req.value(QStringLiteral("required")).toBool();
    }
    QVariantList requiredFor = req.value(QStringLiteral("requiredFor")).toList();
    if (!requiredFor.isEmpty()) {
        for (const QVariant &capVal : requiredFor) {
            if (capVal.toString() == capability) return true;
        }
        return false;
    }
    return true;
}

bool CapabilityFamilyModel::hasFamilyFile(const QVariantMap &family, const QString &modelId, const QString &fileName) const
{
    if (family.isEmpty() || fileName.isEmpty() || !m_models) return false;

    if (!modelId.isEmpty() && m_models->hasFile(modelId, fileName)) return true;

    QString familyModelId = family.value(QStringLiteral("modelId")).toString();
    if (!familyModelId.isEmpty() && modelId != familyModelId && m_models->hasFile(familyModelId, fileName)) return true;

    QString localDir = family.value(QStringLiteral("localDir")).toString();
    if (!localDir.isEmpty() && m_models->hasFile(localDir, fileName)) return true;

    if (!localDir.isEmpty()) {
        QString path = QDir(m_models->concreteModelDir(localDir)).absoluteFilePath(fileName);
        if (QFileInfo::exists(QDir::fromNativeSeparators(path))) return true;
    }
    return false;
}

bool CapabilityFamilyModel::isFileInstalled(const QVariantMap &family, const QString &fileName, const QVariantMap &req) const
{
    if (family.isEmpty() || fileName.isEmpty()) return false;
    QString modelId = req.value(QStringLiteral("modelId")).toString();
    if (modelId.isEmpty()) modelId = family.value(QStringLiteral("modelId")).toString();
    return hasFamilyFile(family, modelId, fileName);
}

void CapabilityFamilyModel::updateItems()
{
    m_items.clear();
    if (!m_registry) return;

    QVariantList all;
    if (m_capabilityId == QStringLiteral("all")) {
        // Combined list without duplicates
        QVariantList combined = m_registry->ttsFamilies() + m_registry->sttFamilies();
        QHash<QString, bool> seen;
        for (const QVariant &itemVal : combined) {
            QVariantMap family = itemVal.toMap();
            QString id = family.value(QStringLiteral("id")).toString();
            if (!seen.contains(id)) {
                seen.insert(id, true);
                all.append(itemVal);
            }
        }
    } else {
        const QString domain = StudioCapabilityRegistry::instance()->familyDomain(m_capabilityId);
        all = domain == QStringLiteral("stt")
            ? m_registry->sttFamilies()
            : m_registry->ttsFamilies();
    }

    QList<QPair<QString, QString>> rawLangs;
    for (const QVariant &itemVal : all) {
        QVariantMap family = itemVal.toMap();
        bool supports = false;
        if (m_capabilityId == QStringLiteral("all")) {
            supports = true;
        } else {
            supports = StudioCapabilityRegistry::instance()->familySupportsCapability(family, m_capabilityId);
        }
        if (!supports) continue;
        collectLanguagesForFamily(family, rawLangs);
    }

    QVariantList finalLangs;
    QVariantMap allLangsItem;
    allLangsItem.insert(QStringLiteral("text"), QStringLiteral("All Languages"));
    allLangsItem.insert(QStringLiteral("value"), QStringLiteral("all"));
    finalLangs.append(allLangsItem);

    QHash<QString, QString> uniqueMap;
    for (const auto &pair : rawLangs) {
        QString code = pair.first.trimmed().toLower();
        QString name = pair.second.trimmed();
        if (code.isEmpty()) continue;
        if (!uniqueMap.contains(code) || (uniqueMap.value(code) == code && name != code)) {
            uniqueMap.insert(code, name);
        }
    }

    QList<QVariantMap> sortedLangs;
    for (auto it = uniqueMap.constBegin(); it != uniqueMap.constEnd(); ++it) {
        QVariantMap item;
        item.insert(QStringLiteral("value"), it.key());
        QString name = it.value();
        if (name.contains(QStringLiteral(" ("))) {
            name = name.left(name.indexOf(QStringLiteral(" ("))).trimmed();
        }
        item.insert(QStringLiteral("text"), name);
        sortedLangs.append(item);
    }

    std::sort(sortedLangs.begin(), sortedLangs.end(), [](const QVariantMap &a, const QVariantMap &b) {
        return a.value(QStringLiteral("text")).toString().compare(b.value(QStringLiteral("text")).toString(), Qt::CaseInsensitive) < 0;
    });

    for (const auto &item : sortedLangs) {
        finalLangs.append(item);
    }

    if (m_availableLanguages != finalLangs) {
        m_availableLanguages = finalLangs;
        emit availableLanguagesChanged();
    }

    for (const QVariant &itemVal : all) {
        QVariantMap family = itemVal.toMap();
        QVariantList capabilities = family.value(QStringLiteral("capabilities")).toList();

        bool supports = false;
        if (m_capabilityId == QStringLiteral("all")) {
            supports = true;
        } else {
            supports = StudioCapabilityRegistry::instance()->familySupportsCapability(family, m_capabilityId);
        }

        if (!supports) continue;

        FamilyItem item;
        item.rawMap = family;
        item.id = family.value(QStringLiteral("id")).toString();
        item.displayName = family.value(QStringLiteral("title")).toString();
        item.subtitle = family.value(QStringLiteral("subtitle")).toString();
        item.description = family.value(QStringLiteral("description")).toString();
        item.accent = family.value(QStringLiteral("accent")).toString();
        item.modelCardUrl = modelCardUrl(family);
        item.readmeContent = readmeContent(family);
        item.thumbnailSource = thumbnailSource(family);
        item.iconName = familyIconName(family);
        item.familyCapability = familyCapability(family);
        item.infoBadges = modelInfoBadges(family);
        item.capabilityBadges = capabilityBadges(family);
        item.statsBadges = statsBadges(family);
        item.isLastudioPick = family.value(QStringLiteral("isLastudioPick")).toBool();
        item.pickLabel = family.value(QStringLiteral("pickLabel")).toString();
        if (item.isLastudioPick && item.pickLabel.isEmpty())
            item.pickLabel = QStringLiteral("LA Studio Pick");
        item.pickReason = family.value(QStringLiteral("pickReason")).toString();
        item.supported = true;
        item.selected = (item.id == m_selectedFamilyId);

        if (!m_searchText.trimmed().isEmpty()) {
            const QString searchable = QStringList{
                item.id,
                item.displayName,
                item.subtitle,
                family.value(QStringLiteral("modelId")).toString(),
                family.value(QStringLiteral("tags")).toStringList().join(QLatin1Char(' '))
            }.join(QLatin1Char(' '));
            if (!searchable.contains(m_searchText.trimmed(), Qt::CaseInsensitive)) {
                continue;
            }
        }

        if (!m_languageFilter.isEmpty() && m_languageFilter != QStringLiteral("all")) {
            if (!familySupportsLanguage(family, m_languageFilter)) {
                continue;
            }
        }

        // Calculate missing files
        QVariantList reqFiles = family.value(QStringLiteral("requiredFiles")).toList();
        bool missingAnyFiles = false;
        QVariantList missingReqs;
        QVariantList requiredFileOptions;
        QVariantMap selectedFiles;

        QString activeCap = m_capabilityId == QStringLiteral("all") ? (capabilities.contains(QStringLiteral("stt")) ? QStringLiteral("stt") : QStringLiteral("tts")) : m_capabilityId;
        QString sourceModelId = family.value(QStringLiteral("modelId")).toString();

        for (const QVariant &reqVal : reqFiles) {
            QVariantMap req = reqVal.toMap();
            if (!requirementRequiredForCapability(req, activeCap)) continue;

            QString role = req.value(QStringLiteral("role")).toString();
            QString reqFile = req.value(QStringLiteral("file")).toString();
            QVariantList candidates = req.value(QStringLiteral("candidates")).toList();

            QString selectedFile;
            bool hasUserSel = false;
            if (m_userSelectedFiles.contains(item.id)) {
                const QVariantMap &familySel = m_userSelectedFiles.value(item.id);
                if (familySel.contains(reqFile)) {
                    selectedFile = familySel.value(reqFile).toString();
                    hasUserSel = true;
                }
            }

            bool foundInstalled = false;
            if (hasUserSel) {
                foundInstalled = isFileInstalled(family, selectedFile, req);
            } else {
                selectedFile = reqFile;
                if (!candidates.isEmpty()) {
                    for (const QVariant &cand : candidates) {
                        if (isFileInstalled(family, cand.toString(), req)) {
                            selectedFile = cand.toString();
                            foundInstalled = true;
                            break;
                        }
                    }
                } else {
                    foundInstalled = isFileInstalled(family, selectedFile, req);
                }
            }

            selectedFiles.insert(role, selectedFile);

            QString reqModelId = req.value(QStringLiteral("modelId")).toString();
            if (reqModelId.isEmpty()) {
                reqModelId = sourceModelId;
            }
            int installState = 0; // NotInstalled
            AppController *app = AppController::instance();
            if (app && app->downloadInstall()) {
                installState = app->downloadInstall()->modelFileState(reqModelId, selectedFile);
            }
            foundInstalled = (installState == 3); // Installed

            QVariantMap requiredFile = req;
            requiredFile[QStringLiteral("selectedFile")] = selectedFile;
            requiredFile[QStringLiteral("installed")] = foundInstalled;
            requiredFile[QStringLiteral("installState")] = installState;
            requiredFile[QStringLiteral("statusText")] = foundInstalled ? QStringLiteral("Installed") : QStringLiteral("Not installed");
            requiredFile[QStringLiteral("selectedSize")] = estimateSize(
                selectedFile,
                req.value(QStringLiteral("file")).toString(),
                req.value(QStringLiteral("size")).toString());
            requiredFileOptions.append(requiredFile);

            if (!foundInstalled) {
                missingAnyFiles = true;
                QVariantMap missing;
                missing[QStringLiteral("role")] = role;
                missing[QStringLiteral("displayName")] = req.value(QStringLiteral("name")).toString();
                missing[QStringLiteral("file")] = selectedFile;
                missing[QStringLiteral("purpose")] = req.value(QStringLiteral("purpose")).toString();
                missingReqs.append(missing);
            }
        }
        item.missingRequirements = missingReqs;
        item.requiredFiles = requiredFileOptions;
        item.selectedFiles = selectedFiles;

        // Runtime check
        QVariantList runtimes = family.value(QStringLiteral("runtimes")).toList();
        bool hasCompatibleRuntime = false;
        bool hasInstalledCompatibleRuntime = false;
        QVariantList options;
        QHash<QString, QVariantList> runtimesById;
        QStringList runtimeIdOrder;

        for (const QVariant &rtVal : runtimes) {
            QVariantMap rt = rtVal.toMap();
            QString runtimeId = rt.value(QStringLiteral("id")).toString();
            if (runtimeId.isEmpty())
                continue;
            if (!runtimesById.contains(runtimeId)) {
                runtimeIdOrder.append(runtimeId);
            }
            runtimesById[runtimeId].append(rt);
        }

        const QString savedRuntimeId = activeCap == QStringLiteral("stt")
            ? (m_settings ? m_settings->selectedSttRuntime() : QString())
            : (m_settings ? m_settings->selectedTtsRuntime() : QString());
        const QString savedRuntimeVersion = activeCap == QStringLiteral("stt")
            ? (m_settings ? m_settings->selectedSttRuntimeVersion() : QString())
            : (m_settings ? m_settings->selectedTtsRuntimeVersion() : QString());

        for (const QString &runtimeId : runtimeIdOrder) {
            const QVariantList runtimeEntries = runtimesById.value(runtimeId);
            if (runtimeEntries.isEmpty())
                continue;

            QVariantList installedVers = m_runtimes->runtimeVersions(runtimeId);
            QString latestVersion;
            QVariantMap latestRuntime;
            QVariantList availableVersions;
            QVariantList versionOptions;
            QString selectedInstalledVersion;
            QString highestInstalledVersion;
            QVariantMap selectedInstalledRuntime;
            QVariantMap highestInstalledRuntime;
            bool hasInstalling = false;
            bool hasDownloading = false;
            int latestInstallState = 0;
            bool installed = false;

            for (const QVariant &entryVal : runtimeEntries) {
                QVariantMap rt = entryVal.toMap();
                const QString reqVer = rt.value(QStringLiteral("version")).toString();
                availableVersions.append(reqVer);
                versionOptions.append(rt);

                if (runtimeVersionGreater(reqVer, latestVersion)) {
                    latestVersion = reqVer;
                    latestRuntime = rt;
                }

                // Filesystem fallback:
                // Runtime extraction + manifest write can complete before runtime registry refresh
                // propagates to this model. If runtimeVersions() is temporarily empty but the
                // expected runtime library already exists in backends/, treat it as installed.
                QString libraryPath;
                const QString runtimeLibrary = rt.value(QStringLiteral("library")).toString();
                const QString engineFamily = rt.value(QStringLiteral("engineFamily")).toString();
                if (!runtimeLibrary.isEmpty() && !engineFamily.isEmpty() && runtimeId.startsWith(engineFamily + QStringLiteral("-"))) {
                    const QString variant = runtimeId.mid(engineFamily.length() + 1);
                    const QString versionSuffix = reqVer.isEmpty() ? QString() : (QStringLiteral("-") + reqVer);
                    const QString runtimeDir = PathUtils::backendsDir()
                        + QStringLiteral("/") + engineFamily
                        + QStringLiteral("/") + variant + versionSuffix;
                    libraryPath = runtimeDir + QStringLiteral("/") + runtimeLibrary;
                }

                int versionInstallState = 0; // NotInstalled
                AppController *appInstance = AppController::instance();
                if (appInstance && appInstance->downloadInstall()) {
                    versionInstallState = appInstance->downloadInstall()->runtimeState(
                        runtimeId,
                        reqVer,
                        libraryPath,
                        rt.value(QStringLiteral("asset")).toString());
                }
                if (reqVer == latestVersion)
                    latestInstallState = versionInstallState;
                if (versionInstallState == 2)
                    hasInstalling = true;
                if (versionInstallState == 1)
                    hasDownloading = true;
                if (versionInstallState == 3 || installedRuntimeVersion(installedVers, reqVer)) {
                    installed = true;
                    if (runtimeId == savedRuntimeId && reqVer == savedRuntimeVersion) {
                        selectedInstalledVersion = reqVer;
                        selectedInstalledRuntime = rt;
                    }
                    if (runtimeVersionGreater(reqVer, highestInstalledVersion)) {
                        highestInstalledVersion = reqVer;
                        highestInstalledRuntime = rt;
                    }
                }
            }

            if (latestRuntime.isEmpty())
                latestRuntime = runtimeEntries.first().toMap();

            const QString selectedVersion = !selectedInstalledVersion.isEmpty()
                ? selectedInstalledVersion
                : highestInstalledVersion;
            QVariantMap displayRuntime = !selectedInstalledRuntime.isEmpty()
                ? selectedInstalledRuntime
                : highestInstalledRuntime;

            QString resolvedSelectedVersion = selectedVersion;
            if (!installed && !installedVers.isEmpty()) {
                installed = true;
                for (const QVariant &installedVal : installedVers) {
                    const QString installedVersion = installedVal.toMap().value(QStringLiteral("version")).toString();
                    if (runtimeId == savedRuntimeId && installedVersion == savedRuntimeVersion) {
                        resolvedSelectedVersion = installedVersion;
                        break;
                    }
                    if (runtimeVersionGreater(installedVersion, resolvedSelectedVersion)) {
                        resolvedSelectedVersion = installedVersion;
                    }
                }
            }
            if (displayRuntime.isEmpty() && installed) {
                for (const QVariant &entryVal : runtimeEntries) {
                    const QVariantMap rt = entryVal.toMap();
                    if (rt.value(QStringLiteral("version")).toString() == resolvedSelectedVersion) {
                        displayRuntime = rt;
                        break;
                    }
                }
            }

            int installState = installed ? 3 : latestInstallState;
            if (!installed && installState == 0) {
                if (hasInstalling)
                    installState = 2;
                else if (hasDownloading)
                    installState = 1;
            }

            QVariantMap comp = HardwareManager::instance()->runtimeCompatibility(latestRuntime);
            bool isComp = comp.value(QStringLiteral("compatible")).toBool();

            if (isComp) {
                hasCompatibleRuntime = true;
                if (installed) {
                    hasInstalledCompatibleRuntime = true;
                }
            }

            QVariantMap opt = installed && !displayRuntime.isEmpty() ? displayRuntime : latestRuntime;
            opt[QStringLiteral("version")] = installed ? resolvedSelectedVersion : latestVersion;
            opt[QStringLiteral("latestVersion")] = latestVersion;
            opt[QStringLiteral("defaultVersion")] = latestVersion;
            opt[QStringLiteral("availableVersions")] = availableVersions;
            opt[QStringLiteral("versionOptions")] = versionOptions;
            opt[QStringLiteral("compatible")] = isComp;
            opt[QStringLiteral("installed")] = installed;
            opt[QStringLiteral("installState")] = installState;
            opt[QStringLiteral("compatibilityTitle")] = comp.value(QStringLiteral("title")).toString();
            opt[QStringLiteral("compatibilityDetail")] = comp.value(QStringLiteral("detail")).toString();
            opt[QStringLiteral("description")] = runtimeDescription(opt);
            opt[QStringLiteral("iconName")] = runtimeIconName(opt);
            options.append(opt);
        }
        item.runtimeOptions = options;

        const auto preferred = preferredRuntime(options);
        item.preferredRuntimeId = preferred.first;
        item.preferredRuntimeVersion = preferred.second;
        item.selectedRuntimeId = preferred.first;
        item.selectedRuntimeVersion = preferred.second;

        item.installed = !missingAnyFiles && hasInstalledCompatibleRuntime;

        if (runtimes.isEmpty()) {
            // No runtime required by model family (e.g. built-in only or CPU)
            item.ready = !missingAnyFiles;
            item.statusReason = item.ready ? QStringLiteral("Ready") : QStringLiteral("Setup Required");
        } else if (!hasCompatibleRuntime) {
            item.ready = false;
            item.statusReason = QStringLiteral("Incompatible");
        } else if (missingAnyFiles || !hasInstalledCompatibleRuntime) {
            item.ready = false;
            item.statusReason = QStringLiteral("Setup Required");
        } else {
            item.ready = true;
            item.statusReason = QStringLiteral("Ready");
        }
        item.statusKind = statusKind(item.ready, item.statusReason);
        item.statusTitle = item.statusReason.isEmpty() ? QStringLiteral("Setup Required") : item.statusReason;

        if (m_statusFilter == QStringLiteral("installed") && !item.ready) {
            continue;
        }
        if (m_statusFilter == QStringLiteral("missing") && item.ready) {
            continue;
        }

        m_items.append(item);
    }
}

bool CapabilityFamilyModel::isModelSuitable(const QString &filename, const QVariantMap &family, const QVariantMap &requirement) const
{
    QString defFile = requirement.value(QStringLiteral("file")).toString();
    if (defFile.isEmpty()) defFile = family.value(QStringLiteral("file")).toString();

    QString defSize = requirement.value(QStringLiteral("size")).toString();
    if (defSize.isEmpty()) defSize = family.value(QStringLiteral("size")).toString();

    QString sizeStr = estimateSize(filename, defFile, defSize);
    double sizeGb = 0;
    QRegularExpression regex("^([\\d.]+)\\s*(MB|GB|KB)$", QRegularExpression::CaseInsensitiveOption);
    auto match = regex.match(sizeStr);
    if (match.hasMatch()) {
        double val = match.captured(1).toDouble();
        QString unit = match.captured(2).toUpper();
        if (unit == "MB") sizeGb = val / 1024.0;
        else if (unit == "KB") sizeGb = val / (1024.0 * 1024.0);
        else sizeGb = val;
    }
    if (sizeGb <= 0) return true;

    bool isGpu = false;
    if (m_settings) {
        QString savedId = (m_capabilityId == "stt") ? m_settings->selectedSttRuntime() : m_settings->selectedTtsRuntime();
        QVariantList runtimes = family.value("runtimes").toList();
        QVariantMap runtime;
        for (const QVariant &rtVal : runtimes) {
            QVariantMap rt = rtVal.toMap();
            if (rt.value("id").toString() == savedId) {
                runtime = rt;
                break;
            }
        }
        if (runtime.isEmpty() && !runtimes.isEmpty()) {
            runtime = runtimes.first().toMap();
        }
        if (!runtime.isEmpty()) {
            QString id = runtime.value("id").toString().toLower();
            if (id.contains("cuda") || id.contains("vulkan")) {
                isGpu = true;
            }
        }
    }

    double memoryLimit = isGpu ? HardwareManager::instance()->vramTotal() : HardwareManager::instance()->ramTotal();
    if (isGpu && memoryLimit < 1.0) {
        memoryLimit = HardwareManager::instance()->ramTotal();
    }
    if (memoryLimit <= 0) {
        memoryLimit = HardwareManager::instance()->ramTotal();
    }
    if (memoryLimit <= 0) {
        return true;
    }

    double requiredGb = sizeGb;
    if (isGpu &&
        isVoxCpm2Family(family) &&
        isFullPrecisionVoxCpm2Candidate(filename, sizeGb)) {
        requiredGb = qMax(sizeGb * 1.55, 8.0);
    }

    return requiredGb <= memoryLimit;
}

QString CapabilityFamilyModel::estimateSize(const QString &filename, const QString &defaultFile, const QString &defaultSizeStr) const
{
    if (filename.isEmpty()) return QString();

    if (m_registry) {
        QVariantList categories = m_registry->modelCategories();
        auto findSize = [](const QVariantList &cats, const QString &fn) -> QString {
            for (const QVariant &catVal : cats) {
                QVariantMap cat = catVal.toMap();
                QVariantList items = cat.value("items").toList();
                for (const QVariant &itemVal : items) {
                    QVariantMap item = itemVal.toMap();
                    if (item.contains("components")) {
                        QVariantList comps = item.value("components").toList();
                        for (const QVariant &compVal : comps) {
                            QVariantMap comp = compVal.toMap();
                            QVariantList variants = comp.value("variants").toList();
                            for (const QVariant &varVal : variants) {
                                QVariantMap variant = varVal.toMap();
                                if (variant.value("file").toString() == fn && variant.contains("size")) {
                                    return variant.value("size").toString();
                                }
                            }
                        }
                    }
                    if (item.contains("variants")) {
                        QVariantList variants = item.value("variants").toList();
                        for (const QVariant &varVal : variants) {
                            QVariantMap variant = varVal.toMap();
                            if (variant.value("file").toString() == fn && variant.contains("size")) {
                                    return variant.value("size").toString();
                            }
                        }
                    }
                    if (item.value("file").toString() == fn && item.contains("size")) {
                        return item.value("size").toString();
                    }
                }
            }
            return QString();
        };

        QString registrySize = findSize(categories, filename);
        if (!registrySize.isEmpty()) {
            return registrySize;
        }
    }

    if (defaultSizeStr.isEmpty()) return QStringLiteral("Unknown size");

    QRegularExpression regex("^([\\d.]+)\\s*(MB|GB|KB)$", QRegularExpression::CaseInsensitiveOption);
    auto match = regex.match(defaultSizeStr);
    if (!match.hasMatch()) return defaultSizeStr;

    double val = match.captured(1).toDouble();
    QString unit = match.captured(2).toUpper();
    double bytes = val;
    if (unit == "GB") bytes *= 1024;
    else if (unit == "KB") bytes /= 1024;

    auto getBits = [](const QString &fn) -> double {
        QString lower = fn.toLower();
        if (lower.contains("q4_k_m") || lower.contains("q4_k") || lower.contains("q4_0")) return 4.5;
        if (lower.contains("q8_0") || lower.contains("q8")) return 8.5;
        if (lower.contains("bf16") || lower.contains("f16")) return 16.0;
        if (lower.contains("f32")) return 32.0;
        if (lower.contains("tokenizer")) return 32.0;
        return 16.0;
    };

    double defaultBits = getBits(defaultFile);
    double targetBits = getBits(filename);

    double targetBytes = bytes * (targetBits / defaultBits);
    if (targetBytes >= 1024) {
        return QString::number(targetBytes / 1024.0, 'f', 2) + QStringLiteral(" GB");
    } else {
        return QString::number(qRound(targetBytes)) + QStringLiteral(" MB");
    }
}

void CapabilityFamilyModel::selectFileForRequirement(const QString &familyId, const QString &reqFile, const QString &selectedFile)
{
    Logger::info(QStringLiteral("CapabilityFamilyModel"),
                 QStringLiteral("selectFileForRequirement familyId: %1, reqFile: %2, selectedFile: %3")
                 .arg(familyId, reqFile, selectedFile));

    if (familyId.isEmpty() || reqFile.isEmpty() || selectedFile.isEmpty()) {
        Logger::warning(QStringLiteral("CapabilityFamilyModel"),
                       QStringLiteral("selectFileForRequirement ignored: familyId, reqFile, or selectedFile is empty"));
        return;
    }
    if (m_userSelectedFiles.contains(familyId) && m_userSelectedFiles[familyId].value(reqFile).toString() == selectedFile) {
        return; // No change
    }
    m_userSelectedFiles[familyId][reqFile] = selectedFile;
    refresh();
}

void CapabilityFamilyModel::setInitialSelectedFiles(const QString &familyId, const QVariantMap &initialSelected)
{
    Logger::info(QStringLiteral("CapabilityFamilyModel"),
                 QStringLiteral("setInitialSelectedFiles familyId: %1, count: %2")
                 .arg(familyId).arg(initialSelected.size()));

    if (familyId.isEmpty()) return;

    m_userSelectedFiles.remove(familyId);

    if (!initialSelected.isEmpty()) {
        QVariantMap familyMap = itemForFamily(familyId);
        if (!familyMap.isEmpty()) {
            QVariantList reqFiles = familyMap.value(QStringLiteral("rawMetadata")).toMap().value(QStringLiteral("requiredFiles")).toList();
            for (const QVariant &reqVal : reqFiles) {
                QVariantMap req = reqVal.toMap();
                QString role = req.value(QStringLiteral("role")).toString();
                QString reqFile = req.value(QStringLiteral("file")).toString();
                if (initialSelected.contains(role)) {
                    QString chosenFile = initialSelected.value(role).toString();
                    m_userSelectedFiles[familyId][reqFile] = chosenFile;
                }
            }
        }
    }
    refresh();
}

} // namespace LAStudio
