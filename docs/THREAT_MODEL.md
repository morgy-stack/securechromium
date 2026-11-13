# Threat model

## Boundary

Protect a normal user's browsing session from accidental insecure navigation, unwanted page permissions, persistent browser state, unsolicited windows, and unconfirmed downloads. Assume the OS, Qt libraries, trust store, executable, and user account are trusted. Web pages, redirects, suggested filenames, and downloads are untrusted.

Qt WebEngine and its Chromium renderer provide the underlying same-origin checks, TLS, process separation, and sandbox. This project does not implement cryptography or a new sandbox. A production deployment requires security-maintained engine builds and a supported OS.

| Threat | Control | Residual risk |
|---|---|---|
| Insecure transport | HTTPS navigation and request checks; no invalid-certificate bypass | Trusted malicious CAs, compromised endpoints, engine bugs |
| Local-file/external scheme navigation | Scheme allowlist; local access disabled | Upload picker permits files explicitly chosen by the user |
| Unexpected camera/location/notification access | Page feature permissions denied | Engine-specific APIs must be reviewed on every Qt upgrade |
| Persistent browsing state | Off-the-record profile; memory cache; no persistent cookies | Swap, crash dumps, OS DNS cache, downloads, network observers, server logs |
| Cross-site cookies | Qt cookie filter rejects third-party requests | First-party tracking, fingerprinting and third-party network traffic remain |
| Download filename/path tricks | Suggested-name cleanup, user path choice, private staging, no intentional overwrite | Malicious file contents; hostile writable directories; platform filename rules |
| Popup or fullscreen spoofing | Both features disabled | In-page phishing, deceptive titles and HTTPS phishing sites |
| Unsafe launch environment | Common insecure WebEngine overrides rejected; no root | A user controlling the process or Qt plugins can subvert the application |

## Intentional limitations

- HTTPS policy is applied to Qt navigation and intercepted resource requests. Qt's interceptor is not an OS firewall. WebRTC, DNS, engine internals and WebSocket handling are not comprehensively mediated by this hook; do not claim every outgoing packet is inspected. WebRTC is restricted to public interfaces but not fully disabled.
- JavaScript and first-party storage remain available so ordinary websites work. There is no content blocker or site isolation stronger than the Qt engine configuration.
- Browsing data is ephemeral within Qt's documented off-the-record semantics, not securely erased from RAM or disk. Profiles share state across tabs. No session restore, history, bookmarks, saved passwords, or telemetry are implemented.
- HTTP-only sites, custom protocols, local files, some login/payment popups, downloads from blob URLs, and permission-dependent applications may fail by design.
- The OS trust store determines accepted certificates. No pinning, custom trust store, certificate inspection UI, or certificate revocation guarantees are added.
- The startup environment guard catches common unsafe overrides; it is not a security boundary against a user deliberately changing the executable or process environment. Command-line and environment controls evolve with Chromium.
- Downloads are retained on disk after reset. Files may contain malware. Crash leftovers may include partial staging files. No antivirus service is called.

## Maintenance expectations

Test against the exact deployed Qt build, verify that its Chromium security fixes are maintained, and review permissions, URL schemes, downloads, and WebEngine settings after each upgrade. Do not distribute the development compatibility runtime as though it were security-current. Independently audit before using this browser for sensitive production workloads.
