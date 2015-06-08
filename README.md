# curl-loader
curl-loader (also known as "omes-nik" and "davilka") is an open-source tool written in C-language, simulating application load and application behavior of thousands and tens of thousand HTTP/HTTPS and FTP/FTPS clients, each with its own source IP-address. In contrast to other tools curl-loader is using real C-written client protocol stacks, namely, HTTP and FTP stacks of libcurl and TLS/SSL of openssl, and simulates user behavior with support for login and authentication flavors.

The goal of the project is to deliver a powerful and flexible open-source testing solution as a real alternative to Spirent Avalanche and IXIA IxLoad.

The tool is useful for performance loading of various application services, for testing web and ftp servers and traffic generation. Activities of each virtual client are logged and collected statistics includes information about resolving, connection establishment, sending of requests, receiving responses, headers and data received/sent, errors from network, TLS/SSL and application (HTTP, FTP) level events and errors.
