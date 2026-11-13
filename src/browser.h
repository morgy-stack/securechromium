#pragma once
#include <QMainWindow>
#include <QWebEnginePage>
#include <QWebEngineUrlRequestInterceptor>
class QWebEngineProfile;
class QWebEngineView;
class QTabWidget;
class QLineEdit;
class QLabel;
class QListWidget;
namespace secure {
class RequestGuard final : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT
  public:
    explicit RequestGuard(QObject *parent = nullptr);
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;
  signals:
    void blocked();
};
class SecurePage final : public QWebEnginePage {
    Q_OBJECT
  public:
    SecurePage(QWebEngineProfile *profile, QObject *parent = nullptr);

  protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool mainFrame) override;
    QWebEnginePage *createWindow(WebWindowType) override;
  signals:
    void denied(const QString &reason);
};
QWebEngineProfile *createProfile(QObject *parent);
class BrowserWindow final : public QMainWindow {
    Q_OBJECT
  public:
    explicit BrowserWindow(QWidget *parent = nullptr,
                           const QUrl &initial = QUrl(QStringLiteral("about:blank")));
    ~BrowserWindow() override;
    QWebEngineView *newTab(const QUrl &url = QUrl(QStringLiteral("about:blank")));

  private:
    QWebEngineView *currentView() const;
    void navigate();
    void syncAddress();
    void closeTab(int index);
    void resetSession();
    void showSecurity();
    QWebEngineProfile *profile_;
    QTabWidget *tabs_;
    QLineEdit *address_;
    QLabel *protection_;
    QListWidget *downloads_;
    quint64 blocked_ = 0;
};
} // namespace secure
