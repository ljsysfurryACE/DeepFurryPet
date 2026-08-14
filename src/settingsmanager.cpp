#include "settingsmanager.h"

SettingsManager& SettingsManager::instance() {
    static SettingsManager inst;
    return inst;
}

SettingsManager::SettingsManager()
    : m_settings("DeepFurry", "DeepFurryPet") {}

QString SettingsManager::apiKey() const {
    return m_settings.value("api/key").toString();
}

void SettingsManager::setApiKey(const QString& key) {
    m_settings.setValue("api/key", key);
    m_settings.sync();
}

bool SettingsManager::hasApiKey() const {
    return !m_settings.value("api/key").toString().isEmpty();
}

void SettingsManager::clearApiKey() {
    m_settings.remove("api/key");
    m_settings.sync();
}
