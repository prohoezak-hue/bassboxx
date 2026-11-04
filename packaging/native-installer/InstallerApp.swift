import AppKit

final class InstallerDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow!
    private var button: NSButton!
    private var installed = false
    func applicationDidFinishLaunching(_ notification: Notification) {
        window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 510, height: 270),
                          styleMask: [.titled, .closable, .miniaturizable], backing: .buffered, defer: false)
        window.title = "BASSBOXX Plugin Installer"
        let title = NSTextField(labelWithString: "Install BASSBOXX")
        title.font = NSFont.boldSystemFont(ofSize: 25); title.frame = NSRect(x: 30, y: 207, width: 450, height: 35)
        window.contentView?.addSubview(title)
        let detail = NSTextField(wrappingLabelWithString: "Installs the AU and VST3 for your Mac user account.\n\nYour working standalone app stays unchanged. Quit MPC before clicking Install. Previous plugin copies are backed up automatically.")
        detail.font = NSFont.systemFont(ofSize: 14); detail.frame = NSRect(x: 30, y: 77, width: 450, height: 120)
        window.contentView?.addSubview(detail)
        button = NSButton(title: "Install", target: self, action: #selector(install))
        button.bezelStyle = .rounded; button.keyEquivalent = "\r"; button.frame = NSRect(x: 357, y: 25, width: 120, height: 35)
        window.contentView?.addSubview(button)
        window.center(); window.makeKeyAndOrderFront(nil); NSApp.activate(ignoringOtherApps: true)
        // Read-only CI smoke mode: opens the real window, writes a report and exits.
        // It never bypasses signature verification or installs any plugin.
        if let index = CommandLine.arguments.firstIndex(of: "--smoke-test"), index + 1 < CommandLine.arguments.count {
            let path = CommandLine.arguments[index + 1]
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                do {
                    let data = try JSONSerialization.data(withJSONObject: ["visible": self.window.isVisible, "button": self.button.title])
                    try data.write(to: URL(fileURLWithPath: path), options: .atomic)
                    NSApp.terminate(nil)
                } catch { fputs("Installer smoke report failed\n", stderr); exit(1) }
            }
        }
    }
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
    private func alert(_ title: String, _ text: String) {
        let alert = NSAlert(); alert.messageText = title; alert.informativeText = text; alert.addButton(withTitle: "OK"); alert.runModal()
    }
    @objc private func install() {
        if installed { NSApp.terminate(nil); return }
        if NSWorkspace.shared.runningApplications.contains(where: { ($0.localizedName ?? "").lowercased().hasPrefix("mpc") }) {
            alert("Quit MPC first", "Save your project and quit MPC, then click Install again."); return
        }
        button.isEnabled = false
        defer { button.isEnabled = true }
        do {
            guard let resources = Bundle.main.resourceURL else { throw InstallFailure(message: "Installer resources are missing.") }
            let result = try PluginInstaller(resources: resources.appendingPathComponent("Plugins", isDirectory: true)).install()
            installed = true; button.title = "Close"
            var message = "Restart MPC. In Preferences → Plugins, keep Use Audio Units checked and choose Rescan All. Select BASSBOXX as an AU instrument.\n\nYour standalone app was not changed."
            if let backup = result.backup { message += "\n\nPrevious plugin copies: \(backup.path)" }
            alert("BASSBOXX installed", message)
        } catch { alert("Installation stopped", error.localizedDescription) }
    }
}
@main struct InstallerMain {
    static func main() {
        let app = NSApplication.shared
        let delegate = InstallerDelegate()
        app.setActivationPolicy(.regular); app.delegate = delegate
        withExtendedLifetime(delegate) { app.run() }
    }
}
