#include "policy.h"
#include <QProcess>
#include <QProcessEnvironment>
#include <QtTest>
class PolicyTests : public QObject {
    Q_OBJECT
  private slots:
    void navigation_data() {
        QTest::addColumn<QString>("url");
        QTest::addColumn<bool>("allowed");
        QTest::newRow("https") << "https://example.com/path?q=1" << true;
        QTest::newRow("blank") << "about:blank" << true;
        QTest::newRow("http") << "http://example.com" << false;
        QTest::newRow("file") << "file:///etc/passwd" << false;
        QTest::newRow("script") << "javascript:alert(1)" << false;
        QTest::newRow("data") << "data:text/html,hello" << false;
        QTest::newRow("blob") << "blob:https://example.com/123" << false;
        QTest::newRow("credentials") << "https://user:pass@example.com" << false;
        QTest::newRow("encoded-user") << "https://%61@example.com" << false;
        QTest::newRow("relative") << "/path" << false;
        QTest::newRow("hostless") << "https:///" << false;
        QTest::newRow("blank-fragment") << "about:blank#x" << false;
        QTest::newRow("websocket-insecure") << "ws://example.com" << false;
    }
    void navigation() {
        QFETCH(QString, url);
        QFETCH(bool, allowed);
        QCOMPARE(secure::navigation(QUrl(url)).allowed, allowed);
    }
    void addresses_data() {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");
        QTest::newRow("bare") << "example.com" << "https://example.com";
        QTest::newRow("trim") << "  https://example.com/a  " << "https://example.com/a";
        QTest::newRow("port") << "localhost:8443/path" << "https://localhost:8443/path";
        QTest::newRow("ipv6") << "[::1]:8443" << "https://[::1]:8443";
        QTest::newRow("http") << "http://example.com" << "";
        QTest::newRow("javascript") << "javascript:alert(1)" << "";
        QTest::newRow("file") << "file:///tmp/a" << "";
        QTest::newRow("space") << "example.com/a b" << "";
        QTest::newRow("empty") << "" << "";
        QTest::newRow("credentials") << "user:pass@example.com" << "";
        QTest::newRow("newline") << "example.com/\nx" << "";
    }
    void addresses() {
        QFETCH(QString, input);
        QFETCH(QString, expected);
        QCOMPARE(secure::address(input).toString(), expected);
    }
    void resources() {
        QVERIFY(secure::request(QUrl("data:image/png;base64,AA")).allowed);
        QVERIFY(secure::request(QUrl("blob:https://example.com/id")).allowed);
        QVERIFY(!secure::request(QUrl("file:///tmp/a")).allowed);
        QVERIFY(!secure::request(QUrl("http://example.com/pixel")).allowed);
        QVERIFY(!secure::request(QUrl("ftp://example.com/a")).allowed);
    }
    void filenames() {
        QCOMPARE(secure::safeDownloadName("../../etc/passwd"), QString("passwd"));
        QCOMPARE(secure::safeDownloadName("C:\\users\\bad.exe"), QString("bad.exe"));
        QCOMPARE(secure::safeDownloadName(".."), QString("download"));
        QCOMPARE(secure::safeDownloadName("a?.txt"), QString("a_.txt"));
        QCOMPARE(secure::safeDownloadName(""), QString("download"));
        QVERIFY(secure::safeDownloadName(QString(300, 'a')).size() <= 180);
    }
    void cliRejectsEngineArguments() {
        const QString executable = QCoreApplication::applicationDirPath() + "/securechromium";
        const QList<QStringList> inputs = {{"--no-sandbox"},
                                           {"--webEngineArgs", "--ignore-certificate-errors"},
                                           {"http://example.com"},
                                           {"file:///etc/passwd"}};
        for (const auto &input : inputs) {
            QProcess process;
            process.start(executable, input);
            QVERIFY(process.waitForFinished(10000));
            QCOMPARE(process.exitCode(), 2);
            QVERIFY(process.readAllStandardError().contains("Usage:"));
        }
        QProcess process;
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert("QTWEBENGINE_DISABLE_SANDBOX", "1");
        process.setProcessEnvironment(environment);
        process.start(executable, QStringList{});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 2);
        QVERIFY(process.readAllStandardError().contains("Unsafe"));
    }
    void runtime() {
        const QList<QByteArray> variables = {"QTWEBENGINE_DISABLE_SANDBOX", "QTWEBENGINE_REMOTE_DEBUGGING",
                                             "QTWEBENGINE_CHROMIUM_FLAGS"};
        QList<QByteArray> previous;
        for (const auto &var : variables) {
            previous << qgetenv(var);
            qunsetenv(var);
        }
        QString reason;
        QVERIFY(secure::safeRuntime(&reason));
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--ignore-certificate-errors");
        QVERIFY(!secure::safeRuntime(&reason));
        QVERIFY(!reason.isEmpty());
        qunsetenv("QTWEBENGINE_CHROMIUM_FLAGS");
        qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");
        QVERIFY(!secure::safeRuntime(nullptr));
        qunsetenv("QTWEBENGINE_DISABLE_SANDBOX");
        qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9222");
        QVERIFY(!secure::safeRuntime(nullptr));
        for (qsizetype i = 0; i < variables.size(); ++i) {
            if (previous[i].isNull())
                qunsetenv(variables[i]);
            else
                qputenv(variables[i], previous[i]);
        }
    }
};
QTEST_GUILESS_MAIN(PolicyTests)
#include "policy_tests.moc"
