# Recorded validation

Validated on November 13, 2026, on Ubuntu 24.04, GCC 13.3, CMake 3.28.3, Ninja, Qt 6.4.2 (Ubuntu package `6.4.2-final+dfsg-12ubuntu9`). This is compatibility validation, not a claim that this engine is suitable for a new production deployment.

| Check | Result |
|---|---|
| Debug build | Passed |
| Clean Release build | Passed |
| Policy suite | 30 Qt Test passes, including lifecycle entries; no failures/skips |
| Browser integration suite | 7 Qt Test passes, including lifecycle entries; no failures/skips |
| Live loopback HTTP interception | Passed; server received no connection |
| Live loopback self-signed TLS certificate rejection | Passed |
| Chromium sandbox | Enabled; tests run outside the tool's restrictive execution sandbox |
| Real UI screenshot | Captured and visually inspected; README image |

The two CTest suites finished in 2.14 seconds. Chromium's expected certificate error appears in the test output. Offscreen-platform and fontconfig diagnostics were also emitted; no compiler warnings were emitted by the application build. Missing optional XKB development discovery did not prevent the offscreen build or tests.

System-wide dependency installation was unavailable because sudo required a password. Build dependencies were downloaded from Ubuntu's package archives and extracted into a user-owned SDK cache. No system packages were modified. A generated, ignored `build/local-env.sh` provides SDK paths in this workspace:

```sh
cd ~/portfolio/SecureChromium
source build/local-env.sh
./build-release/securechromium
```

For other machines, use the normal installation/build instructions in the README. The SDK environment file is local convenience, not a distributable runtime bundle. The application requires a graphical desktop when launched normally.

Not performed: a public-website compatibility sweep, full manual download/permission scenarios, an independent security audit, clean-machine packaging, Windows/macOS validation, or remote GitHub Actions execution. CI configuration is supplied but has not been run on GitHub. The [release checklist](RELEASING.md) defines remaining deployment validation.
