#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QString>
#include <QSettings>

class SettingsManager {
public:
    static SettingsManager& instance();
    
    QString apiKey() const;
    void setApiKey(const QString& key);
    
    bool hasApiKey() const;
    void clearApiKey();

private:
    SettingsManager();
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    
    QSettings m_settings;
};

#endif
