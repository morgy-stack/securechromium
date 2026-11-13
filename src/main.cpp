#include "browser.h"
#include "policy.h"
#include <QApplication>
#include <cstdio>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

int main(int argc, char **argv) {
    // Validate before QApplication/Chromium can consume their own command-line switches.
    const QString argument = argc == 2 ? QString::fromLocal8Bit(argv[1]) : QString{};
    if (argc == 2 && (argument == "--help" || argument == "--version")) {
        std::puts(argument == "--help" ? "Usage: securechromium [https://host/path]"
                                       : "SecureChromium 1.0.0");
        return 0;
    }
    const QUrl initial = argc == 2 ? secure::address(argument) : QUrl("about:blank");
    if (argc > 2 || !initial.isValid() || initial.isEmpty()) {
        std::fprintf(stderr, "Usage: securechromium [https://host/path]\n");
        return 2;
    }
    QString reason;
    if (!secure::safeRuntime(&reason)) {
        std::fprintf(stderr, "%s\n", qPrintable(reason));
        return 2;
    }
#ifdef Q_OS_UNIX
    if (geteuid() == 0) {
        std::fprintf(stderr, "Run SecureChromium as an unprivileged user.\n");
        return 2;
    }
#endif
    QApplication app(argc, argv);
    app.setApplicationName("SecureChromium");
    app.setOrganizationName("SecureChromium");
    app.setApplicationVersion("1.0.0");
    auto *window = new secure::BrowserWindow(nullptr, initial);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
    return app.exec();
}
