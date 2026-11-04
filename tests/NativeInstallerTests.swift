import Foundation

@main struct NativeInstallerTests {
    static let fm = FileManager.default
    static func require(_ value: Bool, _ message: String) throws {
        if !value { throw InstallFailure(message: "TEST FAILED: \(message)") }
    }
    static func fixture(_ root: URL, _ marker: String, id: String = "com.prohoezak.thefunkboxx") throws {
        try fm.createDirectory(at: root.appendingPathComponent("Contents"), withIntermediateDirectories: true)
        let plist = try PropertyListSerialization.data(fromPropertyList: ["CFBundleIdentifier": id], format: .xml, options: 0)
        try plist.write(to: root.appendingPathComponent("Contents/Info.plist"))
        try marker.write(to: root.appendingPathComponent("marker"), atomically: true, encoding: .utf8)
    }
    static func marker(_ url: URL) throws -> String { try String(contentsOf: url.appendingPathComponent("marker"), encoding: .utf8) }
    static func main() {
        do { try runTests() }
        catch { fputs("FAIL: \(error.localizedDescription)\n", stderr); exit(1) }
    }
    static func runTests() throws {
        let root = fm.temporaryDirectory.appendingPathComponent("bassboxx-installer-tests-\(UUID().uuidString)")
        try fm.createDirectory(at: root, withIntermediateDirectories: true)
        defer { try? fm.removeItem(at: root) }
        let resources = root.appendingPathComponent("resources")
        try fixture(resources.appendingPathComponent("TheFunkBoxx.component"), "new-au")
        try fixture(resources.appendingPathComponent("TheFunkBoxx.vst3"), "new-vst3")
        let home = root.appendingPathComponent("user")
        let au = home.appendingPathComponent("Library/Audio/Plug-Ins/Components/TheFunkBoxx.component")
        let vst3 = home.appendingPathComponent("Library/Audio/Plug-Ins/VST3/TheFunkBoxx.vst3")
        let installer = PluginInstaller(resources: resources, home: home, verify: { url in
            try require(["component", "vst3"].contains(url.pathExtension), "staged bundle retains its recognized plugin extension")
        })
        let fresh = try installer.install()
        try require(fresh.backup == nil && fresh.locations.count == 2, "first install")
        try require(try marker(au) == "new-au" && marker(vst3) == "new-vst3", "both products installed")
        try "old-au".write(to: au.appendingPathComponent("marker"), atomically: true, encoding: .utf8)
        try "old-vst3".write(to: vst3.appendingPathComponent("marker"), atomically: true, encoding: .utf8)
        let upgrade = try installer.install()
        guard let backup = upgrade.backup else { throw InstallFailure(message: "Upgrade backup missing") }
        try require(try marker(backup.appendingPathComponent("TheFunkBoxx.component")) == "old-au", "AU backup preserved")
        try require(try marker(backup.appendingPathComponent("TheFunkBoxx.vst3")) == "old-vst3", "VST3 backup preserved")
        // A bad signature is rejected before changing either installed product.
        let rejected = PluginInstaller(resources: resources, home: home, verify: { url in
            if url.lastPathComponent == "TheFunkBoxx.vst3" { throw InstallFailure(message: "Fixture signature rejection") }
        })
        var rejectedAsExpected = false
        do { _ = try rejected.install() } catch { rejectedAsExpected = true }
        try require(rejectedAsExpected, "invalid source rejected")
        try require(try marker(au) == "new-au" && marker(vst3) == "new-vst3", "signature rejection preserves installed copies")
        // A real filesystem transaction failing after its first commit must roll back both products.
        try "rollback-au".write(to: au.appendingPathComponent("marker"), atomically: true, encoding: .utf8)
        try "rollback-vst3".write(to: vst3.appendingPathComponent("marker"), atomically: true, encoding: .utf8)
        let failing = PluginInstaller(resources: resources, home: home, verify: { _ in }, move: { from, to in
            if from.lastPathComponent.hasPrefix(".bassboxx-stage-") && to.lastPathComponent == "TheFunkBoxx.vst3" {
                throw InstallFailure(message: "Fixture second commit failure")
            }
            try fm.moveItem(at: from, to: to)
        })
        var failedAsExpected = false
        do { _ = try failing.install() } catch { failedAsExpected = true }
        try require(failedAsExpected, "transaction failure surfaced")
        try require(try marker(au) == "rollback-au" && marker(vst3) == "rollback-vst3", "both previous copies restored")
        // A different product using the same filename must never be overwritten.
        try fixture(au, "unrelated", id: "example.other.product")
        var unrelatedRejected = false
        do { _ = try installer.install() } catch { unrelatedRejected = true }
        try require(unrelatedRejected && (try marker(au)) == "unrelated", "unrelated bundle left untouched")
        try require(!fm.fileExists(atPath: home.appendingPathComponent("Applications/TheFunkBoxx.app").path), "standalone app never installed or changed")
        for folder in [au.deletingLastPathComponent(), vst3.deletingLastPathComponent()] {
            let names = try fm.contentsOfDirectory(atPath: folder.path)
            try require(!names.contains(where: { $0.hasPrefix(".bassboxx-stage-") }), "staging directories cleaned")
        }
        if CommandLine.arguments.count == 3 && CommandLine.arguments[1] == "--signed-products" {
            let signed = URL(fileURLWithPath: CommandLine.arguments[2], isDirectory: true)
            let signedHome = root.appendingPathComponent("signed-user")
            let real = try PluginInstaller(resources: signed, home: signedHome).install()
            try require(real.locations.count == 2, "actual Developer ID products installed using production verifier")
            var unsignedRejected = false
            do { _ = try PluginInstaller(resources: resources, home: root.appendingPathComponent("unsigned-user")).install() }
            catch { unsignedRejected = true }
            try require(unsignedRejected, "production verifier rejects unsigned products")
            print("PASS: actual Developer ID signature verification and signed-product installation; unsigned products refused")
        }
        print("PASS: native installer first install, upgrade backups, signature rejection, transactional rollback, unrelated-product preservation and cleanup")
    }
}
