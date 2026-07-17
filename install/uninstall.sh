#!/bin/bash

set -euo pipefail

echo "This will remove Web eID and its browser integration files. Administrator" \
     "privileges are required."
read -r -p "Continue? [y/N] " confirm
case "${confirm}" in
  [yY]|[yY][eE][sS]) ;;
  *) echo "Aborted."; exit 1 ;;
esac

# Prompt for the admin password once up front; the sudo calls below reuse this ticket.
sudo -v

sudo rm -rf \
  "/Applications/Utilities/web-eid.app" \
  "/Applications/Utilities/web-eid-safari.app" \
  "/Library/Google/Chrome/NativeMessagingHosts/eu.webeid.json" \
  "/Library/Application Support/Mozilla/NativeMessagingHosts/eu.webeid.json" \
  "/Library/Application Support/Google/Chrome/External Extensions/ncibgoaomkmdpilpocfeponihegamlic.json"

PLIST=/Library/Preferences/org.mozilla.firefox.plist
# Block the extension so it can't be reinstalled from AMO without the native app, avoiding a
# dangling, non-functional extension. Reinstalling Web eID overwrites this back to
# 'normal_installed' (see install/macos-postinstall.in), so this is not a permanent block.
sudo defaults write "${PLIST}" ExtensionSettings \
  -dict-add "'{e68418bc-f2b0-4459-a9ea-3e72b6751b07}'" "{ 'installation_mode' = 'blocked'; }"

echo "Web eID has been uninstalled."
