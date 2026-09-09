#include "petwindow.h"
#include "settingsmanager.h"

#include <QApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QPainter>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

PetWindow::PetWindow(QWidget* parent)
    : QWidget(parent)
    , m_petLabel(new QLabel(this))
    , m_currentMovie(nullptr)
    , m_chatDialog(nullptr)
    , m_actionTimer(new QTimer(this))
    , m_timeCheckTimer(new QTimer(this))
    , m_clickTimer(new QTimer(this))
    , m_deepseek(new DeepSeekClient(this))
{
    setupUI();
    setupApi();
    loadChatHistory();

    // 强制定位（多次尝试）
    QTimer* posTimer = new QTimer(this);
    int* count = new int(0);
    connect(posTimer, &QTimer::timeout, this, [this, posTimer, count]() {
        (*count)++;
        QScreen* screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect geo = screen->availableGeometry();
            int x = 0;
            int y = geo.bottom() - height() - 60;
            this->setGeometry(x, y, width(), height());
            this->move(x, y);
            this->raise();
        }
        if (*count >= 3) {
            posTimer->stop();
            delete count;
        }
    });
    posTimer->start(300);

    // Action timer for timed GIF switching
    m_actionTimer->setSingleShot(true);
    connect(m_actionTimer, &QTimer::timeout, this, &PetWindow::onActionTimerTimeout);

    // Click timer for distinguishing single/double click (300ms)
    m_clickTimer->setSingleShot(true);
    m_clickTimer->setInterval(300);
    connect(m_clickTimer, &QTimer::timeout, this, [this]() {
        // 单击（非双击）→ 播放 2.gif 1.2s
        if (m_clickCount == 1 && m_currentState == Idle && !isNightTime()) {
            playGif(":/assets/2.gif", 1200);
            m_currentState = ClickAnim;
        }
        m_clickCount = 0;
    });

    // Time check every minute
    connect(m_timeCheckTimer, &QTimer::timeout, this, &PetWindow::checkTimeOfDay);
    m_timeCheckTimer->start(60000);

    // Initial time check
    checkTimeOfDay();

    // Set idle
    setIdleGif();
}

PetWindow::~PetWindow() {
    saveChatHistory();
    if (m_currentMovie) {
        m_currentMovie->stop();
    }
}

void PetWindow::setupUI() {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(230, 230);

    m_petLabel->setGeometry(0, 0, 230, 230);
    m_petLabel->setScaledContents(true);
    m_petLabel->setAlignment(Qt::AlignCenter);
    m_currentState = Idle;

    // 对话悬浮窗（独立窗口，可拖动）
    m_chatDialog = new QWidget(nullptr);
    m_chatDialog->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    m_chatDialog->setAttribute(Qt::WA_TranslucentBackground);
    m_chatDialog->setFixedSize(320, 380);
    m_chatDialog->setStyleSheet(
        "background-color: rgba(30, 30, 50, 230);"
        "border: 2px solid #6C3FB5;"
        "border-radius: 10px;"
    );

    auto* chatLayout = new QVBoxLayout(m_chatDialog);
    chatLayout->setContentsMargins(10, 10, 10, 10);
    chatLayout->setSpacing(8);

    auto* titleWidget = new QWidget(m_chatDialog);
    titleWidget->setFixedHeight(30);
    titleWidget->setCursor(Qt::OpenHandCursor);
    titleWidget->setStyleSheet("background: rgba(40, 40, 70, 200); border-radius: 5px;");
    auto* titleLay = new QHBoxLayout(titleWidget);
    titleLay->setContentsMargins(8, 0, 0, 0);
    auto* titleLab = new QLabel("💬 DeepFurry", titleWidget);
    titleLab->setStyleSheet("color: #FF6B35; font-size: 13px; font-weight: bold;");
    titleLay->addWidget(titleLab);
    chatLayout->addWidget(titleWidget);

    m_chatDisplay = new QTextEdit(m_chatDialog);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setStyleSheet(
        "background-color: rgba(20, 20, 40, 200); color: #E0E0E0;"
        "border: 1px solid #6C3FB5; border-radius: 5px;"
        "font-size: 13px; padding: 8px;"
    );
    m_chatDisplay->setMaximumHeight(260);
    chatLayout->addWidget(m_chatDisplay);

    auto* inputLayout = new QHBoxLayout();
    m_chatInput = new QLineEdit(m_chatDialog);
    m_chatInput->setStyleSheet(
        "background-color: rgba(40, 40, 60, 200); color: #E0E0E0;"
        "border: 1px solid #6C3FB5; border-radius: 5px;"
        "padding: 6px; font-size: 13px;"
    );
    m_chatInput->setPlaceholderText("跟DeepFurry说话...");
    inputLayout->addWidget(m_chatInput);

    m_sendBtn = new QPushButton("发送", m_chatDialog);
    m_sendBtn->setStyleSheet(
        "background-color: #6C3FB5; color: white;"
        "border: none; border-radius: 5px; padding: 6px 12px;"
        "font-size: 13px;"
    );
    inputLayout->addWidget(m_sendBtn);
    chatLayout->addLayout(inputLayout);

    connect(m_sendBtn, &QPushButton::clicked, this, [this]() {
        QString msg = m_chatInput->text().trimmed();
        if (!msg.isEmpty()) {
            m_chatDisplay->append("<b style='color:#4CAF50'>你:</b> " + msg);
            m_chatInput->clear();
            m_deepseek->sendMessage(msg);
            saveChatHistory();
        }
    });

    connect(m_chatInput, &QLineEdit::returnPressed, m_sendBtn, &QPushButton::click);

    // 对话框拖拽 + 手势
    m_chatDialog->setMouseTracking(true);
    m_chatDialog->installEventFilter(this);
}

void PetWindow::setupApi() {
    connect(m_deepseek, &DeepSeekClient::responseReady, this, &PetWindow::onDeepSeekResponse);
    connect(m_deepseek, &DeepSeekClient::errorOccurred, this, &PetWindow::onDeepSeekError);
    connect(m_deepseek, &DeepSeekClient::thinking, this, &PetWindow::onDeepSeekThinking);

    // Check if API key exists
    if (!SettingsManager::instance().hasApiKey()) {
        bool ok;
        QString key = QInputDialog::getText(
            this, "DeepSeek API",
            "请输入你的DeepSeek API密钥 (首次使用):",
            QLineEdit::Password, "", &ok
        );
        if (ok && !key.isEmpty()) {
            SettingsManager::instance().setApiKey(key);
            m_deepseek->setApiKey(key);
        }
    } else {
        m_deepseek->setApiKey(SettingsManager::instance().apiKey());
    }
}

void PetWindow::playGif(const QString& gifPath, int durationMs) {
    if (m_currentMovie) {
        m_currentMovie->stop();
        m_petLabel->clear();
        delete m_currentMovie;
        m_currentMovie = nullptr;
    }

    m_currentMovie = new QMovie(gifPath, QByteArray(), this);
    m_currentMovie->setCacheMode(QMovie::CacheAll);
    m_currentMovie->setScaledSize(QSize(230, 230));
    m_petLabel->setMovie(m_currentMovie);
    m_currentMovie->start();

    if (durationMs > 0) {
        m_actionTimer->start(durationMs);
    }
}

void PetWindow::setIdleGif() {
    // 夜间模式: 固定 4.gif
    if (isNightTime()) {
        playGif(":/assets/4.gif");
    } else {
        playGif(":/assets/1.gif");
    }
    m_currentState = Idle;
}

bool PetWindow::isNightTime() const {
    int hour = QTime::currentTime().hour();
    return (hour >= 19 || hour < 6);
}

void PetWindow::setChatMode(bool chatting) {
    // 夜间也用 4.gif, 白天打开对话用 4.gif
    playGif(":/assets/4.gif");
    Q_UNUSED(chatting);
}

void PetWindow::checkTimeOfDay() {
    int hour = QTime::currentTime().hour();
    bool night = (hour >= 19 || hour < 6);
    // 时间段变化时刷新动画
    if (night != m_lastNightState) {
        m_lastNightState = night;
        if (m_currentState == Idle) {
            setIdleGif();
        }
    }
    m_lastCheckedHour = hour;
}

void PetWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->globalPosition().toPoint();
        m_windowPosAtPress = pos();   // 记录按下时窗口位置
        m_isDragging = false;
        m_dragStartedDownward = false;
        // 左键按下: 记录起始位置用于手势判断
        m_pressStartPos = event->globalPosition().toPoint();
        m_pressTime.start();
    }
    if (event->button() == Qt::RightButton) {
        // 右键单击：打开对话
        if (!m_chatVisible) {
            toggleChatDialog();
        } else {
            // 如果对话已打开，双击右键退出
            static QTimer* rightClickTimer = nullptr;
            if (!rightClickTimer) {
                rightClickTimer = new QTimer(this);
                rightClickTimer->setSingleShot(true);
                rightClickTimer->setInterval(300);
            }
            if (rightClickTimer->isActive()) {
                // 双击右键：退出程序
                QApplication::quit();
            } else {
                rightClickTimer->start();
                // 先显示一下提示再关上
                toggleChatDialog();
            }
        }
    }
}

void PetWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 判断: 是否下滑手势 (按住下滑 > 60px 且不是拖窗)
        QPoint releasePos = event->globalPosition().toPoint();
        int dy = releasePos.y() - m_pressStartPos.y();
        int dx = releasePos.x() - m_pressStartPos.x();

        if (!m_isDragging && dy > 60 && abs(dx) < 100) {
            // 下滑 → 呼出 DeepFurry 对话
            if (!m_chatVisible) {
                toggleChatDialog();
            }
        } else if (!m_isDragging && abs(dx) < 8 && abs(dy) < 8) {
            // 视为点击 → 走单击/双击判定
            m_clickCount++;
            if (m_clickCount == 1) {
                m_clickTimer->start();
            } else if (m_clickCount == 2) {
                m_clickTimer->stop();
                m_clickCount = 0;
                // 双击 → 播放 3.gif 7.84s
                if (!isNightTime()) {
                    playGif(":/assets/3.gif", 7840);
                    m_currentState = DoubleClickAnim;
                }
            }
        }
        m_isDragging = false;
        m_dragStartedDownward = false;
    }
}

void PetWindow::mouseDoubleClickEvent(QMouseEvent* event) {
    // 双击已由 clickTimer + mouseRelease 处理, 这里留空避免重复
    Q_UNUSED(event);
}

void PetWindow::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        QPoint cur = event->globalPosition().toPoint();
        int dy = cur.y() - m_pressStartPos.y();
        // 如果移动距离超过阈值 → 拖窗 (非手势)
        if (abs(cur.x() - m_pressStartPos.x()) > 8 || dy < -8) {
            m_isDragging = true;
            // 修复: 用按下时窗口位置 + 全局位移 (原公式两同源坐标相减=0, 拖不动)
            move(m_windowPosAtPress + cur - m_pressStartPos);
        }
    }
}

void PetWindow::onActionTimerTimeout() {
    if (m_currentState == ClickAnim || m_currentState == DoubleClickAnim) {
        setIdleGif();
    }
}

void PetWindow::toggleChatDialog() {
    m_chatVisible = !m_chatVisible;
    m_chatDialog->setVisible(m_chatVisible);

    setChatMode(m_chatVisible);

    if (m_chatVisible) {
        // 放在桌宠窗口旁边
        QPoint petPos = this->pos();
        m_chatDialog->move(petPos.x() + this->width() + 10, petPos.y());
        if (!SettingsManager::instance().hasApiKey()) {
            m_chatDisplay->append("<i style='color:#FF9800'>请在设置中输入DeepSeek API密钥后使用</i>");
        }
    }
}

void PetWindow::onDeepSeekResponse(const QString& response) {
    m_chatDisplay->append("<b style='color:#6C3FB5'>DeepFurry:</b> " + response);
    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    if (sb) sb->setValue(sb->maximum());
    saveChatHistory();
}

void PetWindow::onDeepSeekError(const QString& error) {
    m_chatDisplay->append("<b style='color:#F44336'>错误:</b> " + error);
}

void PetWindow::onDeepSeekThinking() {
    m_chatDisplay->append("<i style='color:#888'>DeepFurry正在思考...</i>");
}

void PetWindow::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
}

void PetWindow::closeEvent(QCloseEvent* event) {
    saveChatHistory();
    if (m_currentMovie) { m_currentMovie->stop(); }
    if (m_deepseek) { delete m_deepseek; m_deepseek = nullptr; }
    QApplication::quit();
}

// ===== 对话记忆持久化 =====
QString PetWindow::chatHistoryPath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) dir = QDir::homePath() + "/.deepfurry";
    QDir().mkpath(dir);
    return dir + "/chat_history.json";
}

void PetWindow::saveChatHistory() {
    // 从聊天显示区提取纯文本并保存
    QString plain = m_chatDisplay->toPlainText();
    QFile file(chatHistoryPath());
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject root;
        root["history"] = plain;
        root["saved_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void PetWindow::loadChatHistory() {
    QFile file(chatHistoryPath());
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QString history = doc.object()["history"].toString();
            if (!history.isEmpty()) {
                m_chatDisplay->setPlainText(history);
            }
        }
    }
}

// 事件过滤器：处理对话窗口拖拽 + 上滑关闭
bool PetWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_chatDialog) {
        static bool dragging = false;
        static QPoint dragStart, dragWidgetPos;
        static QPoint swipeStart;
        static bool swipeTracking = false;

        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                QMouseEvent* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton) {
                    if (me->position().y() < 35) {
                        dragging = true;
                        dragStart = me->globalPosition().toPoint();
                        dragWidgetPos = m_chatDialog->pos();
                    } else {
                        // 记录滑动起点 (用于上滑关闭)
                        swipeStart = me->globalPosition().toPoint();
                        swipeTracking = true;
                    }
                    return true;
                }
                break;
            }
            case QEvent::MouseMove: {
                if (dragging) {
                    QMouseEvent* me = static_cast<QMouseEvent*>(event);
                    m_chatDialog->move(dragWidgetPos + me->globalPosition().toPoint() - dragStart);
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                QMouseEvent* me = static_cast<QMouseEvent*>(event);
                if (dragging) {
                    dragging = false;
                    return true;
                }
                if (swipeTracking && me->button() == Qt::LeftButton) {
                    // 上滑关闭对话 (dy < -60)
                    int dy = me->globalPosition().toPoint().y() - swipeStart.y();
                    if (dy < -60) {
                        if (m_chatVisible) {
                            toggleChatDialog();
                        }
                    }
                    swipeTracking = false;
                    return true;
                }
                swipeTracking = false;
                break;
            }
            default: break;
        }
    }
    return QWidget::eventFilter(obj, event);
}
