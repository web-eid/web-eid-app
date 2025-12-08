# web-eid-app

![European Regional Development Fund](https://github.com/open-eid/DigiDoc4-Client/blob/master/client/images/EL_Regionaalarengu_Fond.png)

The Web eID application performs cryptographic digital signing and
authentication operations with electronic ID smart cards for the Web eID
browser extension (it is the [native messaging host](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Native_messaging)
for the extension). Also works standalone without the extension in command-line
mode.

More information about the Web eID project is available on the project [website](https://web-eid.eu/).

Overview of the Web eID system architecture is available in the [system architecture document](https://github.com/web-eid/web-eid-system-architecture-doc).

Command-line mode is described first as it is easily accessible for testing. Input-output mode that the application uses for communication with the browser extension is described [below](#input-output-mode).

## Command-line mode

Command-line mode is useful both for testing and for using the application
outside of the Web eID browser extension context.

### Usage

Usage:

    web-eid [options] command arguments

Options:

```
  -h, --help               Displays help.
  -c, --command-line-mode  Command-line mode, read commands from command line
                           arguments instead of standard input.
```

Arguments:

```
  command                  The command to execute in command-line mode, any of
                           'get-signing-certificate', 'authenticate', 'sign'.
  arguments                Arguments to the given command as a JSON-encoded
                           string.
```

### Authenticate

The authentication command creates the [Web eID authentication token](https://github.com/web-eid/web-eid-system-architecture-doc#web-eid-authentication-token-specification)
and signs it with the authentication key.

The authentication command requires the challenge nonce and origin URL as
JSON-encoded command-line arguments:

    web-eid -c authenticate '{"challengeNonce": "12345678901234567890123456789012345678901234", "origin": "https://ria.ee"}'

The result will be written to standard output as a JSON-encoded message that
either contains the authentication token or an error code. Successful output
example:

```json
{
    "unverifiedCertificate": "MIIEAzCCA2WgAwIBAgIQHWbVWxCkcYxbzz9nBzGrDzAKBggqhkjOPQQDBDBgMQswCQYDVQQGEwJFRTEbMBkGA1UECgwSU0sgSUQgU29sdXRpb25zIEFTMRcwFQYDVQRhDA5OVFJFRS0xMDc0NzAxMzEbMBkGA1UEAwwSVEVTVCBvZiBFU1RFSUQyMDE4MB4XDTE4MTAyMzE1MzM1OVoXDTIzMTAyMjIxNTk1OVowfzELMAkGA1UEBhMCRUUxKjAoBgNVBAMMIUrDlUVPUkcsSkFBSy1LUklTVEpBTiwzODAwMTA4NTcxODEQMA4GA1UEBAwHSsOVRU9SRzEWMBQGA1UEKgwNSkFBSy1LUklTVEpBTjEaMBgGA1UEBRMRUE5PRUUtMzgwMDEwODU3MTgwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAQ/u+9IncarVpgrACN6aRgUiT9lWC9H7llnxoEXe8xoCI982Md8YuJsVfRdeG5jwVfXe0N6KkHLFRARspst8qnACULkqFNat/Kj+XRwJ2UANeJ3Gl5XBr+tnLNuDf/UiR6jggHDMIIBvzAJBgNVHRMEAjAAMA4GA1UdDwEB/wQEAwIDiDBHBgNVHSAEQDA+MDIGCysGAQQBg5EhAQIBMCMwIQYIKwYBBQUHAgEWFWh0dHBzOi8vd3d3LnNrLmVlL0NQUzAIBgYEAI96AQIwHwYDVR0RBBgwFoEUMzgwMDEwODU3MThAZWVzdGkuZWUwHQYDVR0OBBYEFOTddHnA9rJtbLwhBNyn0xZTQGCMMGEGCCsGAQUFBwEDBFUwUzBRBgYEAI5GAQUwRzBFFj9odHRwczovL3NrLmVlL2VuL3JlcG9zaXRvcnkvY29uZGl0aW9ucy1mb3ItdXNlLW9mLWNlcnRpZmljYXRlcy8TAkVOMCAGA1UdJQEB/wQWMBQGCCsGAQUFBwMCBggrBgEFBQcDBDAfBgNVHSMEGDAWgBTAhJkpxE6fOwI09pnhClYACCk+ezBzBggrBgEFBQcBAQRnMGUwLAYIKwYBBQUHMAGGIGh0dHA6Ly9haWEuZGVtby5zay5lZS9lc3RlaWQyMDE4MDUGCCsGAQUFBzAChilodHRwOi8vYy5zay5lZS9UZXN0X29mX0VTVEVJRDIwMTguZGVyLmNydDAKBggqhkjOPQQDBAOBiwAwgYcCQgHYElkX4vn821JR41akI/lpexCnJFUf4GiOMbTfzAxpZma333R8LNrmI4zbzDp03hvMTzH49g1jcbGnaCcbboS8DAJBObenUp++L5VqldHwKAps61nM4V+TiLqD0jILnTzl+pV+LexNL3uGzUfvvDNLHnF9t6ygi8+Bsjsu3iHHyM1haKM=",
    "algorithm": "ES384",
    "signature": "j8KBTYCXZ8OLuL6eoitRlSmiqw6oIsIJmDm6SttGYvEaJUkBS5kLeCeaokQm5u5viLEJy9iUDONEVlcnLgHIlOZUoEozPNw+AzjI9n7n/D25koYrzmGvMsHX1AKbwqAc",
    "format": "web-eid:1.0",
    "appVersion": "https://web-eid.eu/web-eid-app/releases/2.0.0+0"
}
```

Error example:

    {"error": {"code": "ERR_WEBEID_NATIVE_FATAL", "message": "Invalid origin"}}

The full specification of the format is available in the [Web eID system
architecture document](https://github.com/web-eid/web-eid-system-architecture-doc#web-eid-authentication-token-specification).

### Get signing certificate

The get signing certificate command retrieves the digital signing certificate.

For security reasons, the authentication certificate can only be accessed through the `"authenticate"` command and there is no support for signing of raw hash values with the authentication key.

Pass the origin URL as JSON-encoded command-line argument to the `get-signing-certificate`
command to retrieve the digital signing certificate:

    web-eid -c get-signing-certificate '{"origin": "https://ria.ee"}'

The result will be written to standard output as a JSON-encoded message that
either contains the requested Base64-encoded certificate and supported
signature algorithms, or an error object with a symbolic error code. Successful
output example:

```json
{"certificate":"MIID7DCCA02gAwIBAgIQOZYpcFbeurZbzz9ngqCZsTAKBggqhkjOPQQDBDBgMQswCQYDVQQGEwJFRTEbMBkGA1UECgwSU0sgSUQgU29sdXRpb25zIEFTMRcwFQYDVQRhDA5OVFJFRS0xMDc0NzAxMzEbMBkGA1UEAwwSVEVTVCBvZiBFU1RFSUQyMDE4MB4XDTE4MTAyMzE1MzM1OVoXDTIzMTAyMjIxNTk1OVowfzELMAkGA1UEBhMCRUUxKjAoBgNVBAMMIUrDlUVPUkcsSkFBSy1LUklTVEpBTiwzODAwMTA4NTcxODEQMA4GA1UEBAwHSsOVRU9SRzEWMBQGA1UEKgwNSkFBSy1LUklTVEpBTjEaMBgGA1UEBRMRUE5PRUUtMzgwMDEwODU3MTgwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAASKvaAJSGYBrLcvq0KjgM1sOAS9vbtqeSS2OkqyY4i5AazaetYmCtXKOqUUeljOJUGBUzljDFlAEPHs5Fn+vFT7+cGkOVCA93PBYKVsA9avcWyMwgQQJoW6kA4ZN9yD/mijggGrMIIBpzAJBgNVHRMEAjAAMA4GA1UdDwEB/wQEAwIGQDBIBgNVHSAEQTA/MDIGCysGAQQBg5EhAQIBMCMwIQYIKwYBBQUHAgEWFWh0dHBzOi8vd3d3LnNrLmVlL0NQUzAJBgcEAIvsQAECMB0GA1UdDgQWBBRYwsjA5GJ7HWPvD8ByThPTZ6j3PDCBigYIKwYBBQUHAQMEfjB8MAgGBgQAjkYBATAIBgYEAI5GAQQwEwYGBACORgEGMAkGBwQAjkYBBgEwUQYGBACORgEFMEcwRRY/aHR0cHM6Ly9zay5lZS9lbi9yZXBvc2l0b3J5L2NvbmRpdGlvbnMtZm9yLXVzZS1vZi1jZXJ0aWZpY2F0ZXMvEwJFTjAfBgNVHSMEGDAWgBTAhJkpxE6fOwI09pnhClYACCk+ezBzBggrBgEFBQcBAQRnMGUwLAYIKwYBBQUHMAGGIGh0dHA6Ly9haWEuZGVtby5zay5lZS9lc3RlaWQyMDE4MDUGCCsGAQUFBzAChilodHRwOi8vYy5zay5lZS9UZXN0X29mX0VTVEVJRDIwMTguZGVyLmNydDAKBggqhkjOPQQDBAOBjAAwgYgCQgDBTN1LM08SeH18xKQplqAmV8AQhVvrOxRELCmYp54Qr0XTi2i7kMw0k8gVOV84RlPQP6/ayjs4+ytRbIdkBZK1vQJCARF17/gWYUu7bmy/AXT6fWgyuDV5j2UC2cWDFhPUYyS99rdLGSfP10rP9mPK87Y+4HkfJB/qDyENnJYPa5mUsuFK","supportedSignatureAlgorithms":[{"cryptoAlgorithm":"ECC","hashFunction":"SHA-224","paddingScheme":"NONE"},{"cryptoAlgorithm":"ECC","hashFunction":"SHA-256","paddingScheme":"NONE"},{"cryptoAlgorithm":"ECC","hashFunction":"SHA-384","paddingScheme":"NONE"},{"cryptoAlgorithm":"ECC","hashFunction":"SHA-512","paddingScheme":"NONE"},{"cryptoAlgorithm":"ECC","hashFunction":"SHA3-224","paddingScheme":"NONE"},{"cryptoAlgorithm":"ECC","hashFunction":"SHA3-256","paddingScheme":"NONE"},{"cryptoAlgorithm":"ECC","hashFunction":"SHA3-384","paddingScheme":"NONE"},{"cryptoAlgorithm":"ECC","hashFunction":"SHA3-512","paddingScheme":"NONE"}]}
```

The `supportedSignatureAlgorithms` field contains an array of objects that
specify the signature algorithms that the card supports. Each object has three
members:

- `cryptoAlgorithm`, the cryptographic algorithm, `ECC` for elliptic curve
  cryptography or `RSA` for the Rivest-Shamir-Adleman algorithm;
- `hashFunction`, the cryptographic hash function, any of the SHA-2 or SHA-3
  standard algorithms, see _Allowed hash functions_ below in section _Sign_;
- `paddingScheme`, the padding scheme used, for example `PKCS1.5` for PKCS#1
  v1.5 padding.

### Sign

The signing command signs the provided document hash with the signing key.

The signing command requires the Base64-encoded document hash, hash function,
origin URL and previously retrieved Base64-encoded user signing certificate as
JSON-encoded command-line arguments:

    web-eid -c sign '{"hash": "MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4", "hashFunction": "SHA-384", "origin": "https://ria.ee", "certificate": "MIID7DCCA02gAwIBAgIQGWaqJX+JmHFbyFd4ba1pajAKBggqhkjOPQQDBDBgMQswCQYDVQQGEwJFRTEbMBkGA1UECgwSU0sgSUQgU29sdXRpb25zIEFTMRcwFQYDVQRhDA5OVFJFRS0xMDc0NzAxMzEbMBkGA1UEAwwSVEVTVCBvZiBFU1RFSUQyMDE4MB4XDTE4MTAxODA5NTA0N1oXDTIzMTAxNzIxNTk1OVowfzELMAkGA1UEBhMCRUUxKjAoBgNVBAMMIUrDlUVPUkcsSkFBSy1LUklTVEpBTiwzODAwMTA4NTcxODEQMA4GA1UEBAwHSsOVRU9SRzEWMBQGA1UEKgwNSkFBSy1LUklTVEpBTjEaMBgGA1UEBRMRUE5PRUUtMzgwMDEwODU3MTgwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAATF0tc74ZjE9UNp4iWwMFQ/zolrDB9XH//FJdwT6ynQBT8v6HNdxRF+z+8P81eRMHNb+VehUNUob/s5et7iW0bK28yQrlTcyHfQNxHMfBJFzDl+6QImU2fXKKK4oopV28ujggGrMIIBpzAJBgNVHRMEAjAAMA4GA1UdDwEB/wQEAwIGQDBIBgNVHSAEQTA/MDIGCysGAQQBg5EhAQIBMCMwIQYIKwYBBQUHAgEWFWh0dHBzOi8vd3d3LnNrLmVlL0NQUzAJBgcEAIvsQAECMB0GA1UdDgQWBBTig5wckMK9lsiS+SbcAuZUPIWx1DCBigYIKwYBBQUHAQMEfjB8MAgGBgQAjkYBATAIBgYEAI5GAQQwEwYGBACORgEGMAkGBwQAjkYBBgEwUQYGBACORgEFMEcwRRY/aHR0cHM6Ly9zay5lZS9lbi9yZXBvc2l0b3J5L2NvbmRpdGlvbnMtZm9yLXVzZS1vZi1jZXJ0aWZpY2F0ZXMvEwJFTjAfBgNVHSMEGDAWgBTAhJkpxE6fOwI09pnhClYACCk+ezBzBggrBgEFBQcBAQRnMGUwLAYIKwYBBQUHMAGGIGh0dHA6Ly9haWEuZGVtby5zay5lZS9lc3RlaWQyMDE4MDUGCCsGAQUFBzAChilodHRwOi8vYy5zay5lZS9UZXN0X29mX0VTVEVJRDIwMTguZGVyLmNydDAKBggqhkjOPQQDBAOBjAAwgYgCQgFgoBAifjq0O56O8ivxAWI6zyBwQ8Vpag1qanuh7Qcxspac4mZshc+maWG2ZcxLSNSOJ1a8kxOKe+3PCbittcfwPgJCANc9dTngWTc/8PLLXM62W3FeRnhQqFtw+5askIKEBw5e6maOrxP2mcz9yvnfg0jS52gQ0r905Af0bwp6vVxObxVU"}'

Allowed hash function values are SHA-224, SHA-256, SHA-384, SHA-512, SHA3-224,
SHA3-256, SHA3-384, SHA3-512, and the hash function has to be supported by the
card (see the `hashFunction` member of `supportedSignatureAlgorithms` array elements
in the `get-signing-certificate` command output). The document hash length has to match
the hash function output length and the hash function has to be supported by
the electronic ID signing implementation.

The user signing certificate for the `certificate` field can be retrieved
with the `get-signing-certificate` command as described above:

    web-eid -c get-signing-certificate '{...other arguments as above...}'

The result will be written to standard output as a JSON-encoded message that
either contains the Base64-encoded signature and the signature algorithm used
(see the description of the `supportedSignatureAlgorithms` field above in section
_Get certificate_), or an error code. Successful output example:

```json
{
    "signatureAlgorithm": {"hashFunction": "SHA-384", "paddingScheme": "NONE", "cryptoAlgorithm": "ECC"},
    "signature": "oIw20YRlryXgAhGbHEKBCzQetVAE/S2VjqEQ1h+Kc9Scujcl37oOCmAgoHmEkG4Fpmp/z2waGw8ciJ1yXNpgzIaLhtyytFnFmcwR3zp6OKZTqHuEvTEAxZkxC6gLCxJh"
}
```

## Origin URL

All commands require the mandatory `origin` field in arguments that must contain the URL of the
the website origin, i.e. the URL serving the web application. `origin` URL must be in the form of
`<scheme> "://" <hostname> [ ":" <port> ]` as defined in
[MDN Web Docs](https://developer.mozilla.org/en-US/docs/Web/API/Location/origin),
where `scheme` must be `https`. Note that the `origin` URL must not end with a slash `/`.

## Changing the user interface language

All commands support an optional `lang` parameter that, if provided, must
contain a two-letter ISO 639-1 language code. If translations exist for the given
language, then the user interface will be displayed in this language.
Currently, English, Estonian, Finnish, Croatian and Russian translations are included.

The following example will display the user interface in Estonian:

    web-eid -c get-signing-certificate '{"lang": "et", "origin": "https://ria.ee"}'

See also section [_Adding and updating translations_](#adding-and-updating-translations) below.

## Input-output mode

Input-output mode is intended for communicating with the Web eID browser extension.
Start the application without options and arguments to activate the input-output mode:

    web-eid

Input-output mode supports the same commands, arguments and output as
command-line mode. The command and arguments should be written as a
JSON-encoded message to the application standard input:

```json
{
  "command": "authenticate",
  "arguments": { "nonce": "...", "origin": "...", "origin-cert": "..." }
}
```

The message should start with message length prefix in native-endian byte order
in accordance with the [WebExtensions native messaging specification](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Native_messaging).

The application exits after writing the result to the standard output.

To notify the browser extension that it is ready to receive commands, the
application initiates communication by sending its version to standard
output in input-output mode with the following message (actual version
number varies):

```json
{ "version": "1.0.0" }
```

There is a Python script in `tests/input-output-mode/test.py` that demonstrates
how to use input-output mode, it can be run with:

    python tests/input-output-mode/test.py

## Logging

To enable logging,

- in Linux, run the following command in the console:

      echo 'logging=true' > ~/.config/RIA/web-eid.conf

- in macOS, run the following commands in the console:

      defaults write \
        "$HOME/Library/Containers/eu.web-eid.web-eid/Data/Library/Preferences/eu.web-eid.web-eid.plist" \
        logging true
      defaults write "$HOME/Library/Containers/eu.web-eid.web-eid-safari/Data/Library/Preferences/eu.web-eid.web-eid-safari.plist" \
        logging true

- in Windows, add the following registry key:

      [HKEY_CURRENT_USER\SOFTWARE\RIA\web-eid]
      "logging"="true"

The application writes logs to

- `~/.local/share/RIA/web-eid/web-eid.log` in Linux
- `~/Library/Containers/eu.web-eid.web-eid/Data/Library/Application\ Support/RIA/web-eid/web-eid.log` in macOS
- `~/Library/Containers/eu.web-eid.web-eid-safari/Data/Library/Application\ Support/RIA/web-eid-safari/web-eid-safari.log` of Safari in macOS
- `C:/Users/<USER>/AppData/Local/RIA/web-eid/web-eid.log` in Windows.

## Internal design

The Web eID native application is built with the [Qt](https://www.qt.io/) framework. It consists of

- the Qt application bootstrapping code that starts the Qt event loop (in `src/app` and `src/controller/application.{cpp,hpp}`),
- a controller component that is responsible for creating other component objects and coordinating communication between them and the user (in `src/controller/controller.{cpp,hpp}`),
- command handlers that implement the actual PKI operations (authenticate, get signing certificate, sign) using the `libelectronic-id` library (in `src/controller/command-handlers`),
- thread management code, including the card reader and smart card event monitoring thread (in `src/controller/threads`),
- a dynamic user interface dialog built with Qt Widgets (interface in `src/controller/ui.hpp`, implementation in `src/ui`).
- Code is compatible with Qt5 and Qt6 versions.

The controller has an event-driven internal design that supports unexpected events like card or reader removal or insertion during all operations. Communication with the smart card and card monitoring run in separate threads to assure responsive, non-blocking operations.

## Safari extension

The Web eID extension for Safari is built as a [Safari web extension](https://developer.apple.com/documentation/safariservices/safari_web_extensions). The source code of the Safari extension is in the `src/mac` subdirectory.

Safari web extensions are similar to [WebExtensions](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions) extensions that communicate with a native application, except that the native application is bundled with the extension. The extension can use a combination of JavaScript, CSS, and native code written in Objective-C or Swift. As extensions are built on the standard macOS app model, they are bundled inside an app and distributed through the App Store.

The extension's injected JavaScript script comes from the [_web-eid-webextension_ GitHub repository](https://github.com/web-eid/web-eid-webextension) and is shared with other browsers. The _web-eid-webextension_ repository is linked to `src/mac/js` as a Git submodule.

The injected script, the Safari web extension and the extension's native app live in different sandboxed environments, each with specific limits on what it can access. Like with WebExtensions, communication between the injected script and the web extension happens with message passing. The two runtime environments share a common format for message passing, and each provides an interface for sending and receiving messages.

However, Apple does not provide a dedicated two-way communication channel between the extension and the extension's native app. Therefore a combination of [`NSNotificationCenter`](https://developer.apple.com/documentation/foundation/NSNotificationCenter) and [`NSUserDefaults`](https://developer.apple.com/documentation/foundation/nsuserdefaults) is used for sending and receiving messages between the extension and the extension's native app.

The extension's native component that coordinates launching the app and exchanging messages with it is in `safari-extension.mm`.

The extension's native app in `main.mm` is a thin adapter layer on top of the Qt-based Web eID native app libraries and calls the Web eID app API functions directly from Objective-C code. It also manages message exchange with the app extension.

## Build environment setup

You can examine the files in the `.github/workflows/` directory to see how continuous integration build environment has been set up for different operating systems.

### Ubuntu Linux

Run all commands starting from `RUN apt-get update` from the following
`Dockerfile`:

https://github.com/mrts/docker-qt-cmake-gtest-valgrind-ubuntu/blob/master/Dockerfile

### Windows

- Download Visual Studio 2022 community installer from https://visualstudio.microsoft.com/ and install _Desktop C++ Development_
- Install WIX toolset

      dotnet tool install --global wix --version 6.0.2
      wix extension -g add WixToolset.UI.wixext/6.0.2
      wix extension -g add WixToolset.Util.wixext/6.0.2
      wix extension -g add WixToolset.Bal.wixext/6.0.2

- Download and install Git for Windows from https://git-scm.com/download/win
- Download and install CMake from https://cmake.org/download/
- Install _vcpkg_ by running the following commands in Powershell:

      git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
      cd C:\vcpkg
      .\bootstrap-vcpkg.bat
      .\vcpkg integrate install

- Install _Qt_ with the official [_Qt Online Installer_](https://www.qt.io/download-qt-installer),
  choose _Custom installation > Qt 6.10.0 > MSVC 2022 64-bit_.

### macOS

- Install _Homebrew_ if not already installed:

      /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

- Install _CMake_, _Google Test_, _OpenSSL_ and _Qt_ with _Homebrew_:

      brew install cmake web-eid/gtest/gtest openssl qt@6 node

- Create symlink to _OpenSSL_ location and setup environment variables required
  by _CMake_:

      export OPENSSL_ROOT_DIR=/usr/local/opt/openssl@3.0
      export QT_DIR=/usr/local/opt/qt6/lib/cmake/Qt6

## Building and testing

    git clone --recurse-submodules git@github.com:web-eid/web-eid-app.git
    cd web-eid-app
    ./build.sh
    ./test.sh
    ./build/src/app/web-eid -c get-signing-certificate '{"origin":"https://ria.ee"}'

### Building and testing in Windows

Use _Powershell_ to run the following commands to build the project.

- Set the _Qt_ installation directory variable:

      $QT_ROOT = "C:\Qt\6.10.0\msvc2022_64"

- Set the _vcpkg_ installation directory variable:

      $VCPKG_ROOT = "C:\vcpkg"

- Set the build type variable:

      $BUILD_TYPE = "RelWithDebInfo"

- Run _CMake_:

      cmake -A x64 -B build -S .
          "-DCMAKE_PREFIX_PATH=${QT_ROOT}" `
          "-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" `
          "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}" `
          "-DVCPKG_MANIFEST_DIR=lib/libelectronic-id/.github"

- Run the build and installer build:

      cmake --build build --config ${BUILD_TYPE}
      cmake --build build --config ${BUILD_TYPE} --target installer

- Add _Qt_ binary directory to path:

      $env:PATH += ";${QT_ROOT}\bin"

- Run tests:

      ctest -V -C ${BUILD_TYPE} --test-dir build

## Adding and updating translations

You can use the free [Qt Linguist application](https://doc.qt.io/qt-6/qtlinguist-index.html)
to add and edit translations.

Run the following command to update Qt Linguist TS files:

      cmake --build build --config ${BUILD_TYPE} --target update_translations
