#!/bin/bash
# Preserve the original packaging entry point; new releases use a PKG installer.
set -Eeuo pipefail
exec bash "$(dirname "$0")/package-pkg-macos.sh" "$@"
