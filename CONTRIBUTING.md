# Contributing

Use C++20 and Qt 6.4-compatible APIs unless the minimum version is intentionally raised. Build with warnings enabled, run both test executables, and explain user-visible policy changes. Add tests for meaningful security regressions. Keep network policy decisions in `policy.cpp`; never add certificate bypasses or sandbox-disabling defaults.

Security changes should document their threat boundary, compatibility impact, and residual risk. Avoid logging URLs, credentials, browsing history or downloaded content. Follow the existing QObject ownership model and delete pages before their profile.
