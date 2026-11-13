#pragma once
#include <QString>
#include <QUrl>
namespace secure {
// Pure decisions, shared by the UI, page, and network interceptor.
struct Decision {
    bool allowed;
    QString reason;
};
Decision navigation(const QUrl &url);
Decision request(const QUrl &url);
QUrl address(const QString &input);
QString safeDownloadName(const QString &suggested);
bool safeRuntime(QString *reason);
} // namespace secure
