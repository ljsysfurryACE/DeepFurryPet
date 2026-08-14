#include "deepseekclient.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

DeepSeekClient::DeepSeekClient(QObject* parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_systemPrompt("你是一只可爱的DeepFurry，一只毛茸茸的赛博兽。你性格活泼可爱，偶尔毒舌但心地善良。请用简短可爱的语气回复。")
{
    connect(m_manager, &QNetworkAccessManager::finished, this, &DeepSeekClient::onReplyFinished);
    
    // System message
    QJsonObject sysMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = m_systemPrompt;
    m_messages.append(sysMsg);
}

void DeepSeekClient::setApiKey(const QString& key) {
    m_apiKey = key;
}

void DeepSeekClient::sendMessage(const QString& message) {
    if (m_waiting || m_apiKey.isEmpty()) return;
    m_waiting = true;
    emit thinking();

    // Add invisible prompt prefix
    QString augmentedMsg = "你是一只可爱的DeepFurry。" + message;
    
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = augmentedMsg;
    m_messages.append(userMsg);
    
    QJsonObject body;
    body["model"] = "deepseek-v4-flash";
    body["messages"] = m_messages;
    body["temperature"] = 0.7;
    body["max_tokens"] = 500;
    
    QNetworkRequest request(QUrl("https://api.deepseek.com/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    
    QJsonDocument doc(body);
    m_manager->post(request, doc.toJson());
}

void DeepSeekClient::onReplyFinished(QNetworkReply* reply) {
    m_waiting = false;
    
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();
    
    if (obj.contains("choices") && obj["choices"].isArray()) {
        QJsonArray choices = obj["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject choice = choices[0].toObject();
            QJsonObject message = choice["message"].toObject();
            QString content = message["content"].toString();
            
            // Add to conversation history
            QJsonObject assistantMsg;
            assistantMsg["role"] = "assistant";
            assistantMsg["content"] = content;
            m_messages.append(assistantMsg);
            
            emit responseReady(content);
        }
    } else {
        emit errorOccurred("API 响应格式错误");
    }
    
    reply->deleteLater();
}
