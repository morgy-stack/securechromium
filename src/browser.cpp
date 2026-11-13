#include "browser.h"
#include "policy.h"
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QResizeEvent>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWebEngineCertificateError>
#include <QWebEngineCookieStore>
#include <QWebEngineDownloadRequest>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineView>
#include <memory>
namespace secure {
namespace {
class BrowserView final : public QWebEngineView {
  public:
    explicit BrowserView(QWidget *parent) : QWebEngineView(parent), welcome_(new QFrame(this)) {
        welcome_->setObjectName("welcome");
        welcome_->setStyleSheet(
            "QFrame#welcome { background: #101b2b; } QLabel { color: #e6edf5; background: transparent; }");
        auto *layout = new QVBoxLayout(welcome_);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(18);
        auto label = [layout, this](const QString &text, const QString &style) {
            auto *item = new QLabel(text, welcome_);
            item->setAlignment(Qt::AlignCenter);
            item->setStyleSheet(style);
            layout->addWidget(item);
        };
        label("S / C", "color: #70e1c5; font-size: 24px; font-weight: 700;");
        label("SecureChromium", "font-size: 40px; font-weight: 700;");
        label("A fresh session. An explicit security policy.", "color: #b5c4d8; font-size: 17px;");
        label("HTTPS ONLY   /   EPHEMERAL PROFILE   /   PERMISSIONS DENIED",
              "color: #70e1c5; font-size: 12px;");
        label("Enter an HTTPS address above to begin.\nCtrl+L  address    ·    Ctrl+T  new tab    ·    "
              "Ctrl+I  security",
              "color: #93a7c1; font-size: 14px;");
        label("Private sessions do not provide anonymity. Saved downloads remain on disk.",
              "color: #93a7c1; font-size: 12px;");
        connect(this, &QWebEngineView::urlChanged, this, [this](const QUrl &url) {
            welcome_->setVisible(url.isEmpty() || url == QUrl("about:blank"));
            welcome_->raise();
        });
    }

  protected:
    void resizeEvent(QResizeEvent *event) override {
        QWebEngineView::resizeEvent(event);
        welcome_->setGeometry(rect());
        welcome_->raise();
    }

  private:
    QFrame *welcome_;
};
} // namespace

RequestGuard::RequestGuard(QObject *parent) : QWebEngineUrlRequestInterceptor(parent) {}
void RequestGuard::interceptRequest(QWebEngineUrlRequestInfo &info) {
    if (!request(info.requestUrl()).allowed) {
        info.block(true);
        emit blocked();
    }
}
SecurePage::SecurePage(QWebEngineProfile *profile, QObject *parent) : QWebEnginePage(profile, parent) {
    connect(this, &QWebEnginePage::certificateError, this, [this](QWebEngineCertificateError error) {
        error.rejectCertificate();
        emit denied("Certificate validation failed; connection rejected.");
    });
    connect(this, &QWebEnginePage::featurePermissionRequested, this,
            [this](const QUrl &origin, Feature feature) {
                setFeaturePermission(origin, feature, PermissionDeniedByUser);
                emit denied("Website permission denied by policy.");
            });
    connect(this, &QWebEnginePage::fullScreenRequested, this,
            [](QWebEngineFullScreenRequest request) { request.reject(); });
}
bool SecurePage::acceptNavigationRequest(const QUrl &url, NavigationType, bool) {
    const auto decision = navigation(url);
    if (!decision.allowed)
        emit denied(decision.reason);
    return decision.allowed;
}
QWebEnginePage *SecurePage::createWindow(WebWindowType) {
    emit denied("Website popup blocked. Open a tab with Ctrl+T.");
    return nullptr;
}
QWebEngineProfile *createProfile(QObject *parent) {
    auto *profile = new QWebEngineProfile(parent); // unnamed = off the record
    profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    profile->cookieStore()->setCookieFilter(
        [](const QWebEngineCookieStore::FilterRequest &request) { return !request.thirdParty; });
    auto *settings = profile->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, false);
    settings->setAttribute(QWebEngineSettings::JavascriptCanPaste, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    settings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, false);
    settings->setAttribute(QWebEngineSettings::AllowGeolocationOnInsecureOrigins, false);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, false);
    settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, false);
    settings->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, false);
    settings->setUnknownUrlSchemePolicy(QWebEngineSettings::DisallowUnknownUrlSchemes);
    return profile;
}
BrowserWindow::BrowserWindow(QWidget *parent, const QUrl &initial)
    : QMainWindow(parent), profile_(createProfile(this)), tabs_(new QTabWidget(this)),
      address_(new QLineEdit(this)), protection_(new QLabel(this)), downloads_(new QListWidget(this)) {
    setWindowTitle("SecureChromium — Private session");
    resize(1180, 800);
    setStyleSheet("QMainWindow { background: #101b2b; } QToolBar { background: #18263a; border: 0; padding: "
                  "8px; spacing: 6px; }"
                  "QToolButton { color: #e6edf5; padding: 7px; border-radius: 5px; } QToolButton:hover { "
                  "background: #30445f; }"
                  "QLineEdit { background: #0e1827; color: #e6edf5; border: 1px solid #405570; "
                  "border-radius: 6px; padding: 9px; min-width: 180px; }"
                  "QTabWidget::pane { border: 0; } QTabBar::tab { background: #18263a; color: #b5c4d8; "
                  "padding: 10px 20px; }"
                  "QTabBar::tab:selected { background: #30445f; color: #ffffff; }"
                  "QStatusBar { background: #18263a; color: #b5c4d8; } QMenuBar { background: #101b2b; "
                  "color: #e6edf5; }");
    setCentralWidget(tabs_);
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    address_->setObjectName("address");
    address_->setPlaceholderText("Enter an HTTPS address");
    address_->setClearButtonEnabled(true);
    auto *toolbar = addToolBar("Navigation");
    toolbar->setMovable(false);
    auto action = [this, toolbar](const QString &name, const QKeySequence &key, auto callback) {
        auto *item = toolbar->addAction(name);
        item->setShortcut(key);
        connect(item, &QAction::triggered, this, callback);
        return item;
    };
    action("Back", QKeySequence("Alt+Left"), [this] { currentView()->back(); });
    action("Forward", QKeySequence("Alt+Right"), [this] { currentView()->forward(); });
    action("Reload", QKeySequence::Refresh, [this] { currentView()->reload(); });
    action("Stop", QKeySequence("Escape"), [this] { currentView()->stop(); });
    toolbar->addWidget(address_);
    action("New tab", QKeySequence::AddTab, [this] { newTab(); });
    action("Security", QKeySequence("Ctrl+I"), [this] { showSecurity(); });
    auto *focus = new QAction(this);
    focus->setShortcut(QKeySequence("Ctrl+L"));
    addAction(focus);
    connect(focus, &QAction::triggered, this, [this] {
        address_->setFocus();
        address_->selectAll();
    });
    auto *close = new QAction(this);
    close->setShortcut(QKeySequence::Close);
    addAction(close);
    connect(close, &QAction::triggered, this, [this] { closeTab(tabs_->currentIndex()); });
    auto *session = menuBar()->addMenu("Session");
    connect(session->addAction("Reset private session"), &QAction::triggered, this,
            &BrowserWindow::resetSession);
    connect(session->addAction("Quit"), &QAction::triggered, this, &QWidget::close);
    auto *dock = new QDockWidget("Downloads", this);
    dock->setWidget(downloads_);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    dock->hide();
    menuBar()->addMenu("View")->addAction(dock->toggleViewAction());
    auto *guard = new RequestGuard(profile_);
    profile_->setUrlRequestInterceptor(guard);
    connect(
        guard, &RequestGuard::blocked, this,
        [this] {
            ++blocked_;
            protection_->setText(QString("Private session · %1 requests blocked").arg(blocked_));
        },
        Qt::QueuedConnection);
    connect(
        profile_, &QWebEngineProfile::downloadRequested, this,
        [this, dock](QWebEngineDownloadRequest *download) {
            if (!navigation(download->url()).allowed || !download->page() ||
                download->page() != currentView()->page()) {
                download->cancel();
                return;
            }
            const auto answer =
                QMessageBox::question(this, "Confirm download",
                                      "Save a file from " + download->url().host() +
                                          "?\nDownloaded files remain on disk after the session ends.");
            if (answer != QMessageBox::Yes) {
                download->cancel();
                return;
            }
            const QString path = QFileDialog::getSaveFileName(
                this, "Save download", safeDownloadName(download->suggestedFileName()));
            if (path.isEmpty()) {
                download->cancel();
                return;
            }
            const QFileInfo file(path);
            // Refuse existing destinations and stage in a private directory.
            if (file.exists() || file.isSymLink()) {
                download->cancel();
                QMessageBox::warning(this, "Download cancelled",
                                     "Choose a new file name; overwriting is disabled.");
                return;
            }
            auto staging = std::make_shared<QTemporaryDir>(file.absolutePath() + "/.securechromium-XXXXXX");
            if (!staging->isValid()) {
                download->cancel();
                QMessageBox::warning(this, "Download cancelled",
                                     "Cannot create a private staging directory.");
                return;
            }
            const QString stagedPath = staging->filePath("payload");
            download->setDownloadDirectory(staging->path());
            download->setDownloadFileName("payload");
            auto *item = new QListWidgetItem(file.fileName() + " — starting", downloads_);
            dock->show();
            auto update = [download, item, file] {
                const QString state =
                    download->state() == QWebEngineDownloadRequest::DownloadCompleted   ? "complete"
                    : download->state() == QWebEngineDownloadRequest::DownloadCancelled ? "cancelled"
                    : download->state() == QWebEngineDownloadRequest::DownloadInterrupted
                        ? "interrupted: " + download->interruptReasonString()
                        : "receiving";
                item->setText(
                    QString("%1 — %2 (%3 bytes)").arg(file.fileName(), state).arg(download->receivedBytes()));
            };
            connect(download, &QWebEngineDownloadRequest::receivedBytesChanged, this, update);
            connect(download, &QWebEngineDownloadRequest::stateChanged, this,
                    [this, download, item, file, stagedPath, staging, update] {
                        update();
                        if (download->state() == QWebEngineDownloadRequest::DownloadCompleted) {
                            if (!QFile::rename(stagedPath, file.absoluteFilePath())) {
                                item->setText(file.fileName() + " — save failed; staged file discarded");
                                QMessageBox::warning(this, "Save failed",
                                                     "The destination exists or cannot be written. The "
                                                     "temporary download will be discarded.");
                            }
                        }
                    });
            download->accept();
        });
    connect(address_, &QLineEdit::returnPressed, this, &BrowserWindow::navigate);
    connect(tabs_, &QTabWidget::currentChanged, this, [this] { syncAddress(); });
    connect(tabs_, &QTabWidget::tabCloseRequested, this, &BrowserWindow::closeTab);
    protection_->setText("Private session · HTTPS only");
    statusBar()->addPermanentWidget(protection_);
    newTab(initial);
}
BrowserWindow::~BrowserWindow() {
    // Qt requires all pages to die before their profile.
    while (tabs_->count()) {
        auto *view = tabs_->widget(0);
        tabs_->removeTab(0);
        delete view;
    }
}
QWebEngineView *BrowserWindow::currentView() const {
    return qobject_cast<QWebEngineView *>(tabs_->currentWidget());
}
QWebEngineView *BrowserWindow::newTab(const QUrl &url) {
    auto *view = new BrowserView(tabs_);
    auto *page = new SecurePage(profile_, view);
    view->setPage(page);
    view->setContextMenuPolicy(Qt::NoContextMenu); // no engine-provided external actions
    const int index = tabs_->addTab(view, "New tab");
    tabs_->setCurrentIndex(index);
    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &title) {
        const int tab = tabs_->indexOf(view);
        if (tab >= 0) {
            tabs_->setTabText(tab, title.isEmpty() ? "New tab" : title.left(32));
            tabs_->setTabToolTip(tab, title);
        }
    });
    connect(view, &QWebEngineView::urlChanged, this, [this, view] {
        if (view == currentView())
            syncAddress();
    });
    connect(view, &QWebEngineView::loadProgress, this, [this, view](int progress) {
        if (view == currentView())
            statusBar()->showMessage(QString("Loading %1%").arg(progress));
    });
    connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
        if (view == currentView())
            statusBar()->showMessage(
                ok ? "Ready" : "Page failed to load or was blocked. Check the address and connection.",
                10000);
    });
    connect(page, &SecurePage::denied, this,
            [this](const QString &reason) { statusBar()->showMessage(reason, 15000); });
    connect(page, &QWebEnginePage::renderProcessTerminated, this,
            [this](QWebEnginePage::RenderProcessTerminationStatus, int) {
                statusBar()->showMessage("Renderer stopped. Reload the tab to recover.", 15000);
            });
    if (navigation(url).allowed)
        view->setUrl(url);
    address_->setFocus();
    return view;
}
void BrowserWindow::navigate() {
    const QUrl url = address(address_->text());
    if (url.isEmpty()) {
        statusBar()->showMessage("Enter a valid HTTPS address without embedded credentials.", 10000);
        return;
    }
    currentView()->setUrl(url);
}
void BrowserWindow::syncAddress() {
    if (const auto *view = currentView())
        address_->setText(view->url() == QUrl("about:blank") ? QString{}
                                                             : view->url().toString(QUrl::FullyEncoded));
}
void BrowserWindow::closeTab(int index) {
    if (index < 0)
        return;
    auto *view = tabs_->widget(index);
    tabs_->removeTab(index);
    delete view;
    if (!tabs_->count())
        newTab();
}
void BrowserWindow::resetSession() {
    if (QMessageBox::question(this, "Reset private session",
                              "Close all tabs, cancel active downloads, and discard the browsing profile? "
                              "Saved downloads remain on disk.") != QMessageBox::Yes)
        return;
    // Replacing the whole window also disposes downloads and queued UI callbacks safely.
    auto *replacement = new BrowserWindow;
    replacement->setAttribute(Qt::WA_DeleteOnClose);
    replacement->show();
    close();
}
void BrowserWindow::showSecurity() {
    QMessageBox::information(
        this, "Session security",
        "HTTPS-only navigation and requests\nInvalid certificates rejected\nMemory-only profile and "
        "cache\nDevice permissions and popups denied\nDownloads require confirmation\n\nJavaScript is "
        "enabled. This browser does not provide anonymity, malware scanning, tracker-list filtering, or "
        "protection from engine vulnerabilities. Keep Qt WebEngine updated.");
}
} // namespace secure
