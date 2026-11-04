import Foundation

struct InstallFailure: LocalizedError {
    let message: String
    var errorDescription: String? { message }
}
struct InstallResult {
    let locations: [URL]
    let backup: URL?
}
final class PluginInstaller {
    private let resources: URL
    private let home: URL
    private let verify: (URL) throws -> Void
    private let move: (URL, URL) throws -> Void
    private let fm = FileManager.default
    private let products = [("BASSBOXX.component", "Components"), ("BASSBOXX.vst3", "VST3")]
    init(resources: URL, home: URL = FileManager.default.homeDirectoryForCurrentUser) {
        self.resources = resources
        self.home = home
        self.verify = Self.verifyDeveloperID
        self.move = { try FileManager.default.moveItem(at: $0, to: $1) }
    }
    #if INSTALLER_TEST
    init(resources: URL, home: URL, verify: @escaping (URL) throws -> Void,
         move: @escaping (URL, URL) throws -> Void = { try FileManager.default.moveItem(at: $0, to: $1) }) {
        self.resources = resources; self.home = home; self.verify = verify; self.move = move
    }
    #endif
    private static func verifyDeveloperID(_ bundle: URL) throws {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/codesign")
        process.arguments = ["--verify", "--deep", "--strict", "-R",
                             "anchor apple generic and certificate leaf[subject.OU] = \"5QF6Q5ATG5\"", bundle.path]
        let pipe = Pipe(); process.standardOutput = pipe; process.standardError = pipe
        try process.run(); process.waitUntilExit()
        let details = String(data: pipe.fileHandleForReading.readDataToEndOfFile(), encoding: .utf8) ?? ""
        guard process.terminationStatus == 0 else {
            throw InstallFailure(message: "macOS could not verify \(bundle.lastPathComponent). \(details)")
        }
    }
    private func checkIdentity(_ bundle: URL) throws {
        let data = try Data(contentsOf: bundle.appendingPathComponent("Contents/Info.plist"))
        let value = try PropertyListSerialization.propertyList(from: data, options: [], format: nil)
        guard let info = value as? [String: Any], info["CFBundleIdentifier"] as? String == "com.prohoezak.bassboxx" else {
            throw InstallFailure(message: "A different product uses the BASSBOXX filename at \(bundle.path). It was left untouched.")
        }
    }
    func install() throws -> InstallResult {
        struct Entry { let destination: URL; let staged: URL }
        var entries: [Entry] = []
        var backups: [(URL, URL)] = []
        var installed: [URL] = []
        let backup = home.appendingPathComponent("Library/Application Support/BASSBOXX/Backups/Installer-\(UUID().uuidString)", isDirectory: true)
        defer { for entry in entries { try? fm.removeItem(at: entry.staged) } }
        // Stage and verify both products before changing any previous installation.
        for (name, folder) in products {
            let source = resources.appendingPathComponent(name, isDirectory: true)
            try checkIdentity(source); try verify(source)
            let parent = home.appendingPathComponent("Library/Audio/Plug-Ins/\(folder)", isDirectory: true)
            let destination = parent.appendingPathComponent(name, isDirectory: true)
            if fm.fileExists(atPath: destination.path) { try checkIdentity(destination) }
            try fm.createDirectory(at: parent, withIntermediateDirectories: true)
            let staged = parent.appendingPathComponent(".bassboxx-stage-\(UUID().uuidString)", isDirectory: true)
            entries.append(Entry(destination: destination, staged: staged))
            try fm.copyItem(at: source, to: staged)
            try verify(staged)
        }
        do {
            for entry in entries {
                if fm.fileExists(atPath: entry.destination.path) {
                    try fm.createDirectory(at: backup, withIntermediateDirectories: true)
                    let saved = backup.appendingPathComponent(entry.destination.lastPathComponent, isDirectory: true)
                    try move(entry.destination, saved)
                    backups.append((entry.destination, saved))
                }
                try move(entry.staged, entry.destination)
                installed.append(entry.destination)
            }
        } catch {
            let original = error.localizedDescription
            var restorationErrors: [String] = []
            for location in installed.reversed() {
                do { try fm.removeItem(at: location) } catch { restorationErrors.append(error.localizedDescription) }
            }
            for (destination, saved) in backups.reversed() {
                do { try move(saved, destination) } catch { restorationErrors.append(error.localizedDescription) }
            }
            var message = "Installation stopped: \(original)."
            if restorationErrors.isEmpty { message += " Previous plugin copies were restored." }
            else { message += " Recovery copies remain at \(backup.path). \(restorationErrors.joined(separator: "; "))" }
            throw InstallFailure(message: message)
        }
        return InstallResult(locations: installed, backup: backups.isEmpty ? nil : backup)
    }
}
