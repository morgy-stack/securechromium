# Testing

Configure with `BUILD_TESTING=ON` (the default) and run `ctest --preset dev`. The policy suite uses a Qt core application. Browser tests launch the real Chromium integration without contacting a public website. CTest supplies `QT_QPA_PLATFORM=offscreen` and `--disable-gpu` for browser tests only; the Chromium sandbox stays enabled.

On a desktop, no extra display setup should be necessary. On a headless machine, Qt's offscreen platform plugin and the engine's sandbox prerequisites must be available. Restricted containers may prevent user namespaces or the setuid sandbox helper from operating. Use a supported desktop/VM or a CI runner that permits Chromium's sandbox. Never publish an application that works only with `--no-sandbox`.

## Automated coverage

- Table-driven navigation and address parsing: HTTP, credentials, local URLs, javascript/data/blob destinations, malformed input, ports, IPv6 and whitespace.
- Resource scheme decisions, path traversal in suggested filenames, and launch environment guards.
- Actual off-the-record profile, cookie/cache policies and security attributes.
- A real WebEngine page refuses an HTTP navigation; an independent request-interceptor test verifies no connection reaches a loopback HTTP server.
- A loopback TLS server presents a self-signed certificate; the real engine emits a certificate denial and fails the load.
- The executable refuses unsafe engine arguments and environment overrides before engine startup.
- Tab creation and orderly view/page/profile teardown.

These tests do not prove Chromium's sandbox, TLS stack or Qt's cookie implementation correct. Download dialogs, permission-dependent sites, and live redirect chains also need integration/manual coverage.

## Manual release scenarios

1. Open a valid HTTPS site; navigate back/forward, reload, stop and close the last tab. A fresh empty tab should remain.
2. Try an HTTP address, `file:///etc/passwd`, a credential-bearing URL and a JavaScript URL. The UI should reject them without navigation.
3. Visit a controlled HTTPS endpoint with an expired or self-signed certificate. It must fail with no override option. Redirect HTTPS to HTTP and verify rejection.
4. On an HTTPS fixture, request geolocation, camera, notifications, clipboard, a popup and fullscreen. None should be granted. Attempt HTTP subresource loading; inspect the blocked request counter.
5. Download a harmless file: cancel confirmation, cancel path selection, choose a new path, and attempt an existing path. Verify the completed file contents and that the existing file is unchanged. Test loss of write permission, interruption, and reset during transfer.
6. Set first-party cookies/storage, reset the session, and return to the site. The old state must be gone. Confirm a saved download remains.
7. Run with `QTWEBENGINE_DISABLE_SANDBOX=1`, remote-debugging configuration or `--ignore-certificate-errors` in Chromium flags. Startup must fail. Run as root; startup must fail.
8. Verify page rendering and download behavior with the exact release engine, platform plugins, trust store, and sandbox configuration on the target OS.

Record the compiler, OS, Qt/Chromium versions, results, and outstanding limitations in the release notes. Never substitute a passing CI badge for this release exercise.
