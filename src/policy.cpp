#include "policy.h"
#include <QFileInfo>
#include <QRegularExpression>
namespace secure {
Decision navigation(const QUrl &url) {
    if (!url.isValid() || url.isRelative())
        return {false, "Invalid absolute URL"};
    if (url == QUrl(QStringLiteral("about:blank")))
        return {true, {}};
    if (url.scheme() != QStringLiteral("https") || url.host().isEmpty())
        return {false, "Only HTTPS destinations are allowed"};
    if (!url.userName().isEmpty() || !url.password().isEmpty())
        return {false, "URLs containing credentials are blocked"};
    return {true, {}};
}
Decision request(const QUrl &url) {
    // In-memory resources are needed by modern pages. They are never top-level destinations.
    if (url.isValid() && (url.scheme() == "data" || url.scheme() == "blob"))
        return {true, {}};
    return navigation(url);
}
QUrl address(const QString &input) {
    const QString text = input.trimmed();
    if (text.isEmpty() || text.contains(QRegularExpression(QStringLiteral("[\\s\\x00-\\x1f]"))))
        return {};
    QString candidate = text;
    if (!text.contains("://") && text != "about:blank") {
        // host:port is an address; javascript:, file:, mailto:, etc. are not.
        static const QRegularExpression explicitScheme(
            QStringLiteral("^[A-Za-z][A-Za-z0-9+.-]*:(?![0-9]+(?:/|$))"));
        if (explicitScheme.match(text).hasMatch())
            return {};
        candidate.prepend(QStringLiteral("https://"));
    }
    const QUrl url(candidate, QUrl::StrictMode);
    return navigation(url).allowed ? url : QUrl{};
}
QString safeDownloadName(const QString &suggested) {
    QString name = suggested;
    name.replace('\\', '/');
    name = name.section('/', -1);
    name.replace(QRegularExpression(QStringLiteral("[\\x00-\\x1f<>:\"/\\\\|?*]")), "_");
    while (name.endsWith('.') || name.endsWith(' '))
        name.chop(1);
    if (name.isEmpty() || name == "." || name == "..")
        name = "download";
    return name.left(180);
}
bool safeRuntime(QString *reason) {
    const QString flags = qEnvironmentVariable("QTWEBENGINE_CHROMIUM_FLAGS");
    if (!qEnvironmentVariableIsEmpty("QTWEBENGINE_DISABLE_SANDBOX") ||
        !qEnvironmentVariableIsEmpty("QTWEBENGINE_REMOTE_DEBUGGING") ||
        flags.contains(QRegularExpression(
            QStringLiteral("--(?:no-sandbox|disable-web-security|ignore-certificate-errors|remote-debugging|"
                           "disable-site-isolation|single-process)")))) {
        if (reason)
            *reason = "Unsafe Qt WebEngine environment override detected. Remove sandbox, certificate, "
                      "web-security, or remote-debugging overrides.";
        return false;
    }
    return true;
}
} // namespace secure
