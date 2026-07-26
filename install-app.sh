#!/usr/bin/env bash
# Install the built Oh-a-synth standalone app into /Applications.
#
# Usage: ./install-app.sh
# Build it first with:
#   cd plugin && cmake -B build -DCMAKE_BUILD_TYPE=Release && \
#     cmake --build build --config Release -j8
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP="$ROOT/plugin/build/OhASynth_artefacts/Release/Standalone/Oh-a-synth.app"
DEST="/Applications/Oh-a-synth.app"

if [ ! -d "$APP" ]; then
  echo "error: standalone app not found at:" >&2
  echo "  $APP" >&2
  echo "Build the plugin first (see README → Plugin build)." >&2
  exit 1
fi

# Quit a running copy so we don't overwrite a live bundle.
if pgrep -x "Oh-a-synth" >/dev/null 2>&1; then
  echo "Quitting running Oh-a-synth..."
  pkill -x "Oh-a-synth" || true
  sleep 1
fi

echo "Installing to $DEST"
rm -rf "$DEST"
# ditto preserves bundle structure, symlinks, and code signatures
ditto "$APP" "$DEST"

# These builds are ad-hoc signed, not notarized. Clearing quarantine avoids
# Gatekeeper blocking the app when it was downloaded rather than built here.
xattr -dr com.apple.quarantine "$DEST" 2>/dev/null || true

# Refresh Launch Services so the icon and Spotlight entry update immediately.
/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Support/lsregister \
  -f "$DEST" 2>/dev/null || true

echo "Done. Launch it from Launchpad, Spotlight, or:  open -a Oh-a-synth"
