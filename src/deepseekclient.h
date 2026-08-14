#ifndef DEEPSEEKCLIENT_H
#define DEEPSEEKCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QDateTime>

class DeepSeekClient : public QObject {
    Q_OBJECT
public:
    explicit DeepSeekClient(QObject* parent = nullptr);
    
    void sendMessage(const QString& message);
    void setApiKey(const QString& key);
    bool isWaiting() const { return m_waiting; }

signals:
    void responseReady(const QString& response);
    void errorOccurred(const QString& error);
    void thinking();

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_manager;
    QString m_apiKey;
    QString m_systemPrompt;
    QJsonArray m_messages;
    bool m_waiting = false;
};

#endif
