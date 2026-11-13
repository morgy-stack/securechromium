# Release checklist

1. Use a Qt WebEngine provider with current security fixes. Record the exact Qt and embedded Chromium version and provenance. The minimum build version is not a deployment recommendation.
2. Run a clean release build, automated tests and the manual security scenarios. Review warnings and compare all WebEngine security-related API changes.
3. Package with the target Qt deployment tools and verify helper processes, resources, locales, plugins and TLS dependencies on a clean machine. Do not copy the local development SDK cache into a release blindly.
4. Preserve the Chromium sandbox. Verify distribution permissions for helpers using Qt's platform deployment guidance; do not add insecure environment defaults.
5. Include this project's MIT license plus Qt, Chromium and all bundled third-party notices. Check LGPL/GPL/commercial obligations for the actual Qt distribution and linking mode; this repository's license does not relicense dependencies.
6. Generate checksums for artifacts, scan dependencies, and use platform signing/notarization where applicable. Builds here are not claimed to be reproducible or signed.
7. Tag the reviewed commit, publish platform/version requirements and known limitations, and describe tests actually performed. Do not label unaudited software as certified or guaranteed secure.
8. Monitor Qt/Chromium security advisories. There is no automatic updater: users must replace the application/runtime when fixes ship.

GitHub presentation: set the description to “C++20 browser built with Qt WebEngine: ephemeral profiles, HTTPS policy, and explicit download controls.” Suggested topics: `cpp`, `qt6`, `qtwebengine`, `chromium`, `desktop`, `security`, `cmake`. The README includes an actual screenshot captured from the tested application. Add a short demo of rejected HTTP navigation, tab controls, and session reset. Do not present a mockup as a running build.
