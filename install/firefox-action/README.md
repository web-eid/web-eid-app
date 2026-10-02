# WiX custom action for installing a Firefox extension with enterprise policy

*FirefoxAction* is a custom action for WiX that installs the given Firefox
extension with the Firefox enterprise policy engine.

The custom action should be added to a WiX project as follows, it must be deferred
and not use impersonation (i.e. run in privileged mode for registry access):

    <Binary Id="FirefoxAction.CA.dll" SourceFile="$(sys.SOURCEFILEDIR)FirefoxAction.CA.dll" />
    <CustomAction Id="ExtensionSettingsInstall" Return="check" Execute="deferred" Impersonate="no"
      BinaryKey="FirefoxAction.CA.dll" DllEntry="ExtensionSettingsInstall" />
    <CustomAction Id="ExtensionSettingsRemove" Return="check" Execute="deferred" Impersonate="no"
      BinaryKey="FirefoxAction.CA.dll" DllEntry="ExtensionSettingsRemove" />

The Firefox extension UUID and AMO URL must be passed in via `CustomSettingsData` properties via
dedicated custom actions as follows:

    <CustomAction Id="SetExtensionSettingsForInstall" Property="ExtensionSettingsInstall"
      Value="EXTENSIONSETTINGS_UUID=$(var.FIREFOX_UUID);EXTENSIONSETTINGS_URL=$(var.FIREFOX_URL)" />
    <CustomAction Id="SetExtensionSettingsForRemove" Property="ExtensionSettingsRemove"
      Value="EXTENSIONSETTINGS_UUID=$(var.FIREFOX_UUID);EXTENSIONSETTINGS_URL=$(var.FIREFOX_URL)" />

The custom actions must be scheduled in `InstallExecuteSequence` as follows: 

    <InstallExecuteSequence>
      <Custom Action="SetExtensionSettingsForInstall" Before="InstallInitialize" />
      <Custom Action="ExtensionSettingsInstall" Before="InstallFinalize">
        NOT REMOVE="ALL"
      </Custom>
      <Custom Action="SetExtensionSettingsForRemove" Before="InstallInitialize" />
      <Custom Action="ExtensionSettingsRemove" Before="InstallFinalize">
        REMOVE="ALL" AND NOT UPGRADINGPRODUCTCODE
      </Custom>
    </InstallExecuteSequence>

The code has been forked from the [OpenEID Windows installer project](https://github.com/open-eid/windows-installer/blob/master/FirefoxActionWix/).
