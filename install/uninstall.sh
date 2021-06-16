#!/bin/bash

sudo rm -rf \
  /Applications/Utilities/web-eid.app \
  /Applications/Utilities/web-eid-safari.app \
  /Library/Google/Chrome/NativeMessagingHosts/eu.webeid.json \
  /Library/Application\ Support/Mozilla/NativeMessagingHosts/eu.webeid.json \
  /Library/Application\ Support/Google/Chrome/External\ Extensions/ncibgoaomkmdpilpocfeponihegamlic.json
PLIST=/Library/Preferences/org.mozilla.firefox.plist
sudo defaults write ${PLIST} ExtensionSettings \
  -dict-add "'{e68418bc-f2b0-4459-a9ea-3e72b6751b07}'" "{ 'installation_mode' = 'blocked'; }"
