#!/bin/bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
bundle="$1"
cache="$PWD/build/local-build-deps"
mkdir -p "$cache" "$bundle/LocalBuild"
fetch_verified() {
 local url="$1" hash="$2" file="$3"
 if [[ ! -f "$file" ]]; then
  local partial
  partial="$(mktemp "$cache/download.XXXXXX")"
  curl --fail --location --proto '=https' --proto-redir '=https' --retry 2 "$url" -o "$partial"
  printf '%s  %s\n' "$hash" "$partial" | shasum -a 256 -c -
  mv "$partial" "$file"
 fi
 printf '%s  %s\n' "$hash" "$file" | shasum -a 256 -c -
}
fetch_verified \
 https://github.com/Kitware/CMake/releases/download/v3.31.10/cmake-3.31.10-macos-universal.dmg \
 dfb08721dccf5a2e0a368fd006c1a3ffb8c9e063229e24bc9d882585e4ef1068 \
 "$cache/cmake-3.31.10-macos-universal.dmg"
fetch_verified \
 https://codeload.github.com/juce-framework/JUCE/tar.gz/51a8a6d7aeae7326956d747737ccf1575e61e209 \
 20b740351cc030086951d87614bc08b42d072f1e4db68db1ef1d988e66dbbdbe \
 "$cache/JUCE-8.0.6.tar.gz"
cp "$cache/cmake-3.31.10-macos-universal.dmg" "$cache/JUCE-8.0.6.tar.gz" "$bundle/LocalBuild/"
# Exactly this commit's existing instrument and tests; exclude worktree caches.
git archive --format=tar HEAD CMakeLists.txt Source tests | gzip -n > "$bundle/LocalBuild/BASSBOXX-source.tar.gz"
cp 'packaging/Build BASSBOXX on this Mac.command' "$bundle/"
chmod +x "$bundle/Build BASSBOXX on this Mac.command"
(
 cd "$bundle"
 shasum -a 256 LocalBuild/* 'Build BASSBOXX on this Mac.command' \
  'Install BASSBOXX.command' Installation.txt Build-info.txt Source.txt JUCE-LICENSE.md > Payload-SHA256.txt
)
