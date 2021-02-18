# web-eid-app

![European Regional Development Fund](https://github.com/e-gov/RIHA-Frontend/raw/master/logo/EU/EU.png)

The Web eID command-line application performs cryptographic digital signing and
authentication operations with electronic ID smart cards for the Web eID
browser extension (it is the [native messaging host](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Native_messaging)
for the extension). Also works standalone without the extension in command-line
mode.

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
                           'get-certificate', 'authenticate', 'sign'.
  arguments                Arguments to the given command as a JSON-encoded
                           string.
```

### Get certificate

Pass the certificate type (either `auth` or `sign`) and origin URL as
JSON-encoded command-line arguments to the `get-certificate`
command to retrieve the certificate:

    web-eid -c get-certificate '{"type": "auth", "origin": "https://ria.ee"}'

Passing `"type": "auth"` will retrieve the authentication certificate and
`"type": "sign"` the signing certificate.

The result will be written to standard output as a JSON-encoded message that
either contains the requested Base64-encoded certificate or an error object
with a symbolic error code. Successful output example:

    {"certificate": "MIIEAzCCA2WgAwIBAgIQOWkBWXNDJm1byFd3XsWkvjAKBggqhkjOPQQDBDBgMQswCQYDVQQGEwJFRTEbMBkGA1UECgwSU0sgSUQgU29sdXRpb25zIEFTMRcwFQYDVQRhDA5OVFJFRS0xMDc0NzAxMzEbMBkGA1UEAwwSVEVTVCBvZiBFU1RFSUQyMDE4MB4XDTE4MTAxODA5NTA0N1oXDTIzMTAxNzIxNTk1OVowfzELMAkGA1UEBhMCRUUxKjAoBgNVBAMMIUrDlUVPUkcsSkFBSy1LUklTVEpBTiwzODAwMTA4NTcxODEQMA4GA1UEBAwHSsOVRU9SRzEWMBQGA1UEKgwNSkFBSy1LUklTVEpBTjEaMBgGA1UEBRMRUE5PRUUtMzgwMDEwODU3MTgwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAR5k1lXzvSeI9O/1s1pZvjhEW8nItJoG0EBFxmLEY6S7ki1vF2Q3TEDx6dNztI1Xtx96cs8r4zYTwdiQoDg7k3diUuR9nTWGxQEMO1FDo4Y9fAmiPGWT++GuOVoZQY3XxijggHDMIIBvzAJBgNVHRMEAjAAMA4GA1UdDwEB/wQEAwIDiDBHBgNVHSAEQDA+MDIGCysGAQQBg5EhAQIBMCMwIQYIKwYBBQUHAgEWFWh0dHBzOi8vd3d3LnNrLmVlL0NQUzAIBgYEAI96AQIwHwYDVR0RBBgwFoEUMzgwMDEwODU3MThAZWVzdGkuZWUwHQYDVR0OBBYEFOQsvTQJEBVMMSmhyZX5bibYJubAMGEGCCsGAQUFBwEDBFUwUzBRBgYEAI5GAQUwRzBFFj9odHRwczovL3NrLmVlL2VuL3JlcG9zaXRvcnkvY29uZGl0aW9ucy1mb3ItdXNlLW9mLWNlcnRpZmljYXRlcy8TAkVOMCAGA1UdJQEB/wQWMBQGCCsGAQUFBwMCBggrBgEFBQcDBDAfBgNVHSMEGDAWgBTAhJkpxE6fOwI09pnhClYACCk+ezBzBggrBgEFBQcBAQRnMGUwLAYIKwYBBQUHMAGGIGh0dHA6Ly9haWEuZGVtby5zay5lZS9lc3RlaWQyMDE4MDUGCCsGAQUFBzAChilodHRwOi8vYy5zay5lZS9UZXN0X29mX0VTVEVJRDIwMTguZGVyLmNydDAKBggqhkjOPQQDBAOBiwAwgYcCQgH1UsmMdtLZti51Fq2QR4wUkAwpsnhsBV2HQqUXFYBJ7EXnLCkaXjdZKkHpABfM0QEx7UUhaI4i53jiJ7E1Y7WOAAJBDX4z61pniHJapI1bkMIiJQ/ti7ha8fdJSMSpAds5CyHIyHkQzWlVy86f9mA7Eu3oRO/1q+eFUzDbNN3Vvy7gQWQ="}

Error example:

    {"error": {"code": "ERR_WEBEID_NATIVE_FATAL", "message": "Invalid origin"}}

### Authenticate

Authentication command requires the nonce, origin URL and Base64-encoded origin
certificate as JSON-encoded command-line arguments:

    web-eid -c authenticate '{"nonce": "12345678123456781234567812345678", "origin": "https://ria.ee", "origin-cert": "MIIHQjCCBiqgAwIBAgIQDzBMjsxeynwIiZ83A6z+JTANBgkqhkiG9w0BAQsFADBwMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3d3cuZGlnaWNlcnQuY29tMS8wLQYDVQQDEyZEaWdpQ2VydCBTSEEyIEhpZ2ggQXNzdXJhbmNlIFNlcnZlciBDQTAeFw0xOTA5MTEwMDAwMDBaFw0yMDEwMDcxMjAwMDBaMFUxCzAJBgNVBAYTAkVFMRAwDgYDVQQHEwdUYWxsaW5uMSEwHwYDVQQKDBhSaWlnaSBJbmZvc8O8c3RlZW1pIEFtZXQxETAPBgNVBAMMCCoucmlhLmVlMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAsckbR484WWjD3MfWEPxLYQlr79YdvsxZ/AHZDZsiGO0GhMZPOkpuxcIqMZIHMFWFLK/7u/5P+otISVTBbBbmfSvyjzVjWy4CZFiSXhkMfyoZzWRb3pHnl/5AmAepJ8aCTESRX+H7Vag9Q1lgzbaqLGS8jCOiWaaTT6+TYUSj97adOp9vbLLuelocCKwMtlBkBU0cwR/jaLpBKzlzWiBeQR4pyJJ4uHoDIV1ftx6qGABqicKTn6ksORoGLI8e+JAfDl/yzWB4Me56MVb+8fYb3XU1sncCYZtJ8aYv3sVm8vaaHbCIjjxBWlLmLZVkv5YSPxjYxOLBHokA2nN9owbhGcWx7EpJd1ZjBhW1OrTBxpAj1NBvMSttRk1Oil3BdMAchgwfkirGSgmc3cTKcwZB4JLaUu3udrFihnRVdr6is5x0jya+1DvQdNVzKJiR9fZP/2AxxLO5785w1spYTZ+4pcu6RrHaABZ/T/lK5zEEM2BelAKgXVQSOe1MwrChg7pDWRKNjuBYkgc/2AJ/8slMtQgsM3C15KouqtzblLSzRxuDfjx7HYeKhe4YPdR/gL7M6KFP0vH38Jc/FLAlQSQDVEXYpSo22kJLJu35rcMfLQIScvy0gP2i/RD2V+/c/zaQcZzZblKsl8tR4LE1MMo7cxcnlR5nN6ogYHIKpA45mKECAwEAAaOCAvEwggLtMB8GA1UdIwQYMBaAFFFo/5CvAgd1PMzZZWRiohK4WXI7MB0GA1UdDgQWBBRg9ZbQFUAlIXPTxSOkzqf189Hk1jAbBgNVHREEFDASgggqLnJpYS5lZYIGcmlhLmVlMA4GA1UdDwEB/wQEAwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwdQYDVR0fBG4wbDA0oDKgMIYuaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL3NoYTItaGEtc2VydmVyLWc2LmNybDA0oDKgMIYuaHR0cDovL2NybDQuZGlnaWNlcnQuY29tL3NoYTItaGEtc2VydmVyLWc2LmNybDBMBgNVHSAERTBDMDcGCWCGSAGG/WwBATAqMCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy5kaWdpY2VydC5jb20vQ1BTMAgGBmeBDAECAjCBgwYIKwYBBQUHAQEEdzB1MCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wTQYIKwYBBQUHMAKGQWh0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydFNIQTJIaWdoQXNzdXJhbmNlU2VydmVyQ0EuY3J0MAwGA1UdEwEB/wQCMAAwggEEBgorBgEEAdZ5AgQCBIH1BIHyAPAAdwC72d+8H4pxtZOUI5eqkntHOFeVCqtS6BqQlmQ2jh7RhQAAAW0fPlWCAAAEAwBIMEYCIQDcMTeRN3aAyS04CHOVKJPGggrzvuoPzkgt3t9yv7ovbgIhAJ3K9wbP0le/HLTNNIcSwMAtS9UIrYARI6T6DATJI7u5AHUAh3W/51l8+IxDmV+9827/Vo1HVjb/SrVgwbTq/16ggw8AAAFtHz5V1wAABAMARjBEAiBM7HM08sJ/PmMqxk+hmEK8oVfFlLxsO0DMzSiATp618QIgA5o9fj/TsRITYhGhM3LB8Hg1rF7kM4WEjNpR5HzRb8swDQYJKoZIhvcNAQELBQADggEBABBWZf2mSKdE+IndCEzd9+NaGnMoa5rCTKNLsptdtrr9IuPxEJiuMZCVAAtlYqJzFRsuOFa3DoSZ+ToV8KQsf2pAZasHc4VnJ6ULk55SDoGHvyUf8LETFcXeDGnhunw1WFpajQOKIYkrYsp7Jzrd3XDbJ/h9FHCtKHQSGCqHu9f0TxnDtXk9jOvVSAAI7g9R6pC8DfI2kFYCk48rKCA31VZO3vH9dYzkuJv9UlFG7qxHEkyqpFKPLmoillsKWPeKjpFAD0jv5GB2C5SZ2sMTE90kMiV9PcqTi3TXLMvyYbrCa1nhNezj4So82o1Q+qBfLdCag7t+ZGKefl2UJYjDoU0="}'

The result will be written to standard output as a JSON-encoded message that
either contains the OpenID X509 ID Token or an error code. Successful output
example:

    {"auth-token": "eyJhbGciOiJFUzM4NCIsInR5cCI6IkpXVCIsIng1YyI6WyJNSUlFQXpDQ0EyV2dBd0lCQWdJUU9Xa0JXWE5ESm0xYnlGZDNYc1drdmpBS0JnZ3Foa2pPUFFRREJEQmdNUXN3Q1FZRFZRUUdFd0pGUlRFYk1Ca0dBMVVFQ2d3U1Uwc2dTVVFnVTI5c2RYUnBiMjV6SUVGVE1SY3dGUVlEVlFSaERBNU9WRkpGUlMweE1EYzBOekF4TXpFYk1Ca0dBMVVFQXd3U1ZFVlRWQ0J2WmlCRlUxUkZTVVF5TURFNE1CNFhEVEU0TVRBeE9EQTVOVEEwTjFvWERUSXpNVEF4TnpJeE5UazFPVm93ZnpFTE1Ba0dBMVVFQmhNQ1JVVXhLakFvQmdOVkJBTU1JVXJEbFVWUFVrY3NTa0ZCU3kxTFVrbFRWRXBCVGl3ek9EQXdNVEE0TlRjeE9ERVFNQTRHQTFVRUJBd0hTc09WUlU5U1J6RVdNQlFHQTFVRUtnd05Ta0ZCU3kxTFVrbFRWRXBCVGpFYU1CZ0dBMVVFQlJNUlVFNVBSVVV0TXpnd01ERXdPRFUzTVRnd2RqQVFCZ2NxaGtqT1BRSUJCZ1VyZ1FRQUlnTmlBQVI1azFsWHp2U2VJOU8vMXMxcFp2amhFVzhuSXRKb0cwRUJGeG1MRVk2UzdraTF2RjJRM1RFRHg2ZE56dEkxWHR4OTZjczhyNHpZVHdkaVFvRGc3azNkaVV1UjluVFdHeFFFTU8xRkRvNFk5ZkFtaVBHV1QrK0d1T1ZvWlFZM1h4aWpnZ0hETUlJQnZ6QUpCZ05WSFJNRUFqQUFNQTRHQTFVZER3RUIvd1FFQXdJRGlEQkhCZ05WSFNBRVFEQStNRElHQ3lzR0FRUUJnNUVoQVFJQk1DTXdJUVlJS3dZQkJRVUhBZ0VXRldoMGRIQnpPaTh2ZDNkM0xuTnJMbVZsTDBOUVV6QUlCZ1lFQUk5NkFRSXdId1lEVlIwUkJCZ3dGb0VVTXpnd01ERXdPRFUzTVRoQVpXVnpkR2t1WldVd0hRWURWUjBPQkJZRUZPUXN2VFFKRUJWTU1TbWh5Wlg1YmliWUp1YkFNR0VHQ0NzR0FRVUZCd0VEQkZVd1V6QlJCZ1lFQUk1R0FRVXdSekJGRmo5b2RIUndjem92TDNOckxtVmxMMlZ1TDNKbGNHOXphWFJ2Y25rdlkyOXVaR2wwYVc5dWN5MW1iM0l0ZFhObExXOW1MV05sY25ScFptbGpZWFJsY3k4VEFrVk9NQ0FHQTFVZEpRRUIvd1FXTUJRR0NDc0dBUVVGQndNQ0JnZ3JCZ0VGQlFjREJEQWZCZ05WSFNNRUdEQVdnQlRBaEprcHhFNmZPd0kwOXBuaENsWUFDQ2srZXpCekJnZ3JCZ0VGQlFjQkFRUm5NR1V3TEFZSUt3WUJCUVVITUFHR0lHaDBkSEE2THk5aGFXRXVaR1Z0Ynk1emF5NWxaUzlsYzNSbGFXUXlNREU0TURVR0NDc0dBUVVGQnpBQ2hpbG9kSFJ3T2k4dll5NXpheTVsWlM5VVpYTjBYMjltWDBWVFZFVkpSREl3TVRndVpHVnlMbU55ZERBS0JnZ3Foa2pPUFFRREJBT0Jpd0F3Z1ljQ1FnSDFVc21NZHRMWnRpNTFGcTJRUjR3VWtBd3BzbmhzQlYySFFxVVhGWUJKN0VYbkxDa2FYamRaS2tIcEFCZk0wUUV4N1VVaGFJNGk1M2ppSjdFMVk3V09BQUpCRFg0ejYxcG5pSEphcEkxYmtNSWlKUS90aTdoYThmZEpTTVNwQWRzNUN5SEl5SGtReldsVnk4NmY5bUE3RXUzb1JPLzFxK2VGVXpEYk5OM1Z2eTdnUVdRPSJdfQ.eyJhdWQiOlsiaHR0cHM6Ly9yaWEuZWUiLCJ1cm46Y2VydDpzaGEtMjU2OjZmMGRmMjQ0ZTRhODU2Yjk0YjNiM2I0NzU4MmEwYTUxYTMyZDY3NGRiYzcxMDcyMTFlZDIzZDRiZWM2ZDljNzIiXSwiZXhwIjoiMTU4Njg3MTE2OSIsImlhdCI6IjE1ODY4NzA4NjkiLCJpc3MiOiJ3ZWItZWlkIGFwcCB2MC45LjAtMS1nZTZlODlmYSIsIm5vbmNlIjoiMTIzNDU2NzgxMjM0NTY3ODEyMzQ1Njc4MTIzNDU2NzgiLCJzdWIiOiJKw5VFT1JHLEpBQUstS1JJU1RKQU4sMzgwMDEwODU3MTgifQ.0Y5CdMiSZ14rOnd7sbp-XeBQ7qrJVd21yTmAbiRnzAXtwqW8ZROg4jL4J7bpQ2fwyUz4-dVwLoVRVnxfJY82b8NXuxXrDb-8MXXmVYrMW0q0kPbEzqFbEnPYHjNnKAN0"}

The OpenID X509 ID Token is a standard JSON Web Token that can be validated
with e.g. the [JWT.IO online validator](https://jwt.io/).

### Sign

Signing command requires the Base64-encoded document hash, hash algorithm,
origin URL and previously retrieved Base64-encoded user signing certificate as
JSON-encoded command-line arguments:

    web-eid -c sign '{"doc-hash": "MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4", "hash-algo": "SHA-384", "origin": "https://ria.ee", "user-eid-cert": "MIID7DCCA02gAwIBAgIQGWaqJX+JmHFbyFd4ba1pajAKBggqhkjOPQQDBDBgMQswCQYDVQQGEwJFRTEbMBkGA1UECgwSU0sgSUQgU29sdXRpb25zIEFTMRcwFQYDVQRhDA5OVFJFRS0xMDc0NzAxMzEbMBkGA1UEAwwSVEVTVCBvZiBFU1RFSUQyMDE4MB4XDTE4MTAxODA5NTA0N1oXDTIzMTAxNzIxNTk1OVowfzELMAkGA1UEBhMCRUUxKjAoBgNVBAMMIUrDlUVPUkcsSkFBSy1LUklTVEpBTiwzODAwMTA4NTcxODEQMA4GA1UEBAwHSsOVRU9SRzEWMBQGA1UEKgwNSkFBSy1LUklTVEpBTjEaMBgGA1UEBRMRUE5PRUUtMzgwMDEwODU3MTgwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAATF0tc74ZjE9UNp4iWwMFQ/zolrDB9XH//FJdwT6ynQBT8v6HNdxRF+z+8P81eRMHNb+VehUNUob/s5et7iW0bK28yQrlTcyHfQNxHMfBJFzDl+6QImU2fXKKK4oopV28ujggGrMIIBpzAJBgNVHRMEAjAAMA4GA1UdDwEB/wQEAwIGQDBIBgNVHSAEQTA/MDIGCysGAQQBg5EhAQIBMCMwIQYIKwYBBQUHAgEWFWh0dHBzOi8vd3d3LnNrLmVlL0NQUzAJBgcEAIvsQAECMB0GA1UdDgQWBBTig5wckMK9lsiS+SbcAuZUPIWx1DCBigYIKwYBBQUHAQMEfjB8MAgGBgQAjkYBATAIBgYEAI5GAQQwEwYGBACORgEGMAkGBwQAjkYBBgEwUQYGBACORgEFMEcwRRY/aHR0cHM6Ly9zay5lZS9lbi9yZXBvc2l0b3J5L2NvbmRpdGlvbnMtZm9yLXVzZS1vZi1jZXJ0aWZpY2F0ZXMvEwJFTjAfBgNVHSMEGDAWgBTAhJkpxE6fOwI09pnhClYACCk+ezBzBggrBgEFBQcBAQRnMGUwLAYIKwYBBQUHMAGGIGh0dHA6Ly9haWEuZGVtby5zay5lZS9lc3RlaWQyMDE4MDUGCCsGAQUFBzAChilodHRwOi8vYy5zay5lZS9UZXN0X29mX0VTVEVJRDIwMTguZGVyLmNydDAKBggqhkjOPQQDBAOBjAAwgYgCQgFgoBAifjq0O56O8ivxAWI6zyBwQ8Vpag1qanuh7Qcxspac4mZshc+maWG2ZcxLSNSOJ1a8kxOKe+3PCbittcfwPgJCANc9dTngWTc/8PLLXM62W3FeRnhQqFtw+5askIKEBw5e6maOrxP2mcz9yvnfg0jS52gQ0r905Af0bwp6vVxObxVU"}'

Allowed hash algorithm values are SHA-224, SHA-256, SHA-384, SHA-512, SHA3-224,
SHA3-256, SHA3-384, SHA3-512. The document hash length has to match the hash
algorithm output length and the hash algorithm has to be supported by the electronic
ID signing implementation.

The user signing certificate for the `user-eid-cert` field can be retrieved
with the `get-certificate` command as described above, by passing `sign` in the
`type` field:

    web-eid -c get-certificate '{"type": "sign", ...other arguments as above...}'

The result will be written to standard output as a JSON-encoded message that
either contains the Base64-encoded signature or an error code. Successful
output example:

    {"signature": "O0vhA3XSflWsE/v0xcdLGPG0mbWHySSPXWJkRni8vklWKhlzWvGuHD98rWZzf31VsuldBlhJo9eflZvmKK/tUuTjiwXw2BLq3E+qv6Vs6nLHJNJs/ki6Lm/s+bwffyrH"}

## Input-output mode

Input-output mode is intended for communicating with the Web eID browser extension.
Start the application without options and arguments to activate input-output mode:

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

The application writes logs to

- `~/.local/share/web-eid/web-eid.log` in Linux
- `~/Library/Application Support/web-eid/web-eid.log` in macOS
- `C:/Users/<USER>/AppData/Local/web-eid/web-eid.log` in Windows.

## Build environment setup

### Ubuntu Linux

Run all commands starting from `RUN apt-get update` from the following
`Dockerfile`:

https://github.com/mrts/docker-qt-cmake-gtest-valgrind-ubuntu/blob/master/Dockerfile

### Windows

- Download Visual Studio 2019 community installer from https://visualstudio.microsoft.com/ and install _Desktop C++ Development_
- Download WIX toolset from https://wixtoolset.org/ and install version 3.11.2
- Download and install Git for Windows from https://git-scm.com/download/win
- Install _vcpkg_ by running the following commands in Powershell:

      git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
      cd C:\vcpkg
      .\bootstrap-vcpkg.bat
      .\vcpkg integrate install

- Install _Google Test_ and _OpenSSL_ with _vcpkg_:

      .\vcpkg install --recurse --triplet x64-windows --clean-after-build gtest openssl

- Install _Qt_ with the official [_Qt Online Installer_](https://www.qt.io/download-qt-installer), choose _Default Qt 5.15 desktop installation_.
  Export _Qt_ directory with e.g. `set Qt5_DIR=C:\qt` before building.

### macOS

- Install _Homebrew_ if not already installed:

      /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

- Install _CMake_, _Google Test_, _OpenSSL_ and _Qt_ with _Homebrew_:

      brew install cmake web-eid/gtest/gtest openssl qt

- Create symlink to _OpenSSL_ location and setup environment variables required
  by _CMake_:

      ln -sf /usr/local/Cellar/openssl@1.1/1.1.1* /usr/local/opt/openssl
      export OPENSSL_ROOT_DIR=/usr/local/opt/openssl
      export Qt5_DIR=/usr/local/opt/qt

## Building

    git clone --recurse-submodules git@github.com:web-eid/web-eid-app.git
    cd web-eid-app
    ./build.sh
    ./test.sh
    ./build/src/app/web-eid -c get-certificate '{"type":"auth", "origin":"https://ria.ee"}'
