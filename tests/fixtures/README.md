# TLS fixture

`localhost.crt` is a self-signed localhost certificate and `localhost.key` is its intentionally public, test-only private key. Never use this identity for a real service. The browser test runs a loopback TLS server and verifies rejection without adding the certificate to any trust store. An expired fixture still exercises rejection; refresh it if testing a specific certificate error becomes necessary.
