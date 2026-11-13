#include "browser.h"
#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QtTest>

// Public, self-signed test-only certificate; never a production identity.
class TlsFixture final : public QTcpServer {
  public:
    QSslCertificate certificate;
    QSslKey key;

  protected:
    void incomingConnection(qintptr descriptor) override {
        auto *socket = new QSslSocket(this);
        socket->setLocalCertificate(certificate);
        socket->setPrivateKey(key);
        connect(socket, &QSslSocket::encrypted, socket, [socket] {
            socket->write("HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK");
            socket->disconnectFromHost();
        });
        connect(socket, &QSslSocket::disconnected, socket, &QObject::deleteLater);
        if (!socket->setSocketDescriptor(descriptor)) {
            delete socket;
            return;
        }
        socket->startServerEncryption();
    }
};

class BrowserTests : public QObject {
    Q_OBJECT
  private slots:
    void profileDefaults() {
        QObject owner;
        auto *profile = secure::createProfile(&owner);
        QVERIFY(profile->isOffTheRecord());
        QCOMPARE(profile->httpCacheType(), QWebEngineProfile::MemoryHttpCache);
        QCOMPARE(profile->persistentCookiesPolicy(), QWebEngineProfile::NoPersistentCookies);
        auto *settings = profile->settings();
        QVERIFY(!settings->testAttribute(QWebEngineSettings::AllowRunningInsecureContent));
        QVERIFY(!settings->testAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls));
        QVERIFY(!settings->testAttribute(QWebEngineSettings::JavascriptCanOpenWindows));
        QVERIFY(!settings->testAttribute(QWebEngineSettings::JavascriptCanAccessClipboard));
        QVERIFY(!settings->testAttribute(QWebEngineSettings::ScreenCaptureEnabled));
        QCOMPARE(settings->unknownUrlSchemePolicy(), QWebEngineSettings::DisallowUnknownUrlSchemes);
    }
    void realPageRejectsInsecureNavigation() {
        QObject owner;
        auto *profile = secure::createProfile(&owner);
        secure::SecurePage page(profile);
        QSignalSpy denied(&page, &secure::SecurePage::denied);
        QSignalSpy loaded(&page, &QWebEnginePage::loadFinished);
        page.setUrl(QUrl("http://127.0.0.1:1/never-connect"));
        QTRY_VERIFY_WITH_TIMEOUT(!denied.isEmpty(), 15000);
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty(), 15000);
        QVERIFY(!loaded.last().first().toBool());
    }
    void interceptorBlocksBeforeConnection() {
        QObject owner;
        auto *profile = secure::createProfile(&owner);
        auto *guard = new secure::RequestGuard(profile);
        profile->setUrlRequestInterceptor(guard);
        QSignalSpy blocked(guard, &secure::RequestGuard::blocked);
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QWebEnginePage page(profile);
        QSignalSpy loaded(&page, &QWebEnginePage::loadFinished);
        page.setUrl(QUrl(QString("http://127.0.0.1:%1/pixel").arg(server.serverPort())));
        QTRY_VERIFY_WITH_TIMEOUT(!blocked.isEmpty(), 15000);
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty(), 15000);
        QVERIFY(!loaded.last().first().toBool());
        QVERIFY(!server.hasPendingConnections());
    }
    void selfSignedCertificateIsRejected() {
        QVERIFY(QSslSocket::supportsSsl());
        QFile certificate(QFINDTESTDATA("fixtures/localhost.crt"));
        QVERIFY(certificate.open(QIODevice::ReadOnly));
        QFile key(QFINDTESTDATA("fixtures/localhost.key"));
        QVERIFY(key.open(QIODevice::ReadOnly));
        TlsFixture server;
        server.certificate = QSslCertificate(certificate.readAll());
        server.key = QSslKey(key.readAll(), QSsl::Rsa);
        QVERIFY(!server.certificate.isNull());
        QVERIFY(!server.key.isNull());
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QObject owner;
        auto *profile = secure::createProfile(&owner);
        secure::SecurePage page(profile);
        QSignalSpy denied(&page, &secure::SecurePage::denied);
        QSignalSpy loaded(&page, &QWebEnginePage::loadFinished);
        page.setUrl(QUrl(QString("https://localhost:%1/").arg(server.serverPort())));
        QTRY_VERIFY_WITH_TIMEOUT(!denied.isEmpty(), 15000);
        QVERIFY(denied.first().first().toString().contains("Certificate"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty(), 15000);
        QVERIFY(!loaded.last().first().toBool());
    }
    void tabsAndDestruction() {
        secure::BrowserWindow window;
        auto *view = window.newTab();
        QVERIFY(qobject_cast<secure::SecurePage *>(view->page()));
        QVERIFY(view->page()->profile()->isOffTheRecord());
        QSignalSpy loaded(view, &QWebEngineView::loadFinished);
        view->setUrl(QUrl("about:blank"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty(), 15000);
        if (!qEnvironmentVariableIsEmpty("SECURECHROMIUM_SCREENSHOT")) {
            window.show();
            QTest::qWait(300);
            QVERIFY(window.grab().save(qEnvironmentVariable("SECURECHROMIUM_SCREENSHOT")));
        }
    }
};
QTEST_MAIN(BrowserTests)
#include "browser_tests.moc"
