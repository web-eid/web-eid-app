<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleExecutable</key>
	<string>${MACOSX_BUNDLE_EXECUTABLE_NAME}</string>
	<key>CFBundleIdentifier</key>
	<string>ee.ria.web-eid-safari.${MACOSX_BUNDLE_EXECUTABLE_NAME}</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>${MACOSX_BUNDLE_EXECUTABLE_NAME}</string>
	<key>CFBundlePackageType</key>
	<string>XPC!</string>
	<key>CFBundleShortVersionString</key>
	<string>${MACOSX_BUNDLE_SHORT_VERSION_STRING}</string>
	<key>CFBundleVersion</key>
	<string>${MACOSX_BUNDLE_BUNDLE_VERSION}</string>
	<key>LSMinimumSystemVersion</key>
	<string>${CMAKE_OSX_DEPLOYMENT_TARGET}</string>
	<key>NSExtension</key>
	<dict>
		<key>NSExtensionPointIdentifier</key>
		<string>com.apple.Safari.web-extension</string>
		<key>NSExtensionPrincipalClass</key>
		<string>SafariWebExtensionHandler</string>
	</dict>
</dict>
</plist>
