#ifndef PETWINDOW_H
#define PETWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QMovie>
#include <QTimer>
#include <QTime>
#include <QPoint>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include "deepseekclient.h"

class PetWindow : public QWidget {
    Q_OBJECT
public:
    explicit PetWindow(QWidget* parent = nullptr);
    ~PetWindow();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onActionTimerTimeout();
    void checkTimeOfDay();
    void onDeepSeekResponse(const QString& response);
    void onDeepSeekError(const QString& error);
    void onDeepSeekThinking();

private:
    enum AnimState { Idle, ClickAnim, DoubleClickAnim };

    void setupUI();
    void setupApi();
    void playGif(const QString& gifPath, int durationMs = 0);
    void setIdleGif();
    void toggleChatDialog();
    void setChatMode(bool chatting);
    bool isNightTime() const;

    // 对话记忆持久化
    QString chatHistoryPath() const;
    void saveChatHistory();
    void loadChatHistory();

    QLabel* m_petLabel;
    QMovie* m_currentMovie;

    // Chat dialog
    QWidget* m_chatDialog;
    QTextEdit* m_chatDisplay;
    QLineEdit* m_chatInput;
    QPushButton* m_sendBtn;

    // Animation state
    AnimState m_currentState = Idle;
    QTimer* m_actionTimer;
    QTimer* m_timeCheckTimer;

    // Drag & swipe
    bool m_isDragging = false;
    bool m_dragStartedDownward = false;
    QPoint m_dragStartPos;
    QPoint m_pressStartPos;
    QPoint m_windowPosAtPress;  // 按下时窗口位置 (拖动用)
    QElapsedTimer m_pressTime;

    // Click tracking (单击/双击区分)
    QTimer* m_clickTimer;
    int m_clickCount = 0;

    // 对话标题栏拖拽
    bool eventFilter(QObject* obj, QEvent* event) override;

    // API
    DeepSeekClient* m_deepseek;
    bool m_chatVisible = false;

    // Time
    int m_lastCheckedHour = -1;
    bool m_lastNightState = false;
};

#endif
