#  Resolves the firmware version baked in as the VERSION string.
#
#  The GitHub workflows export FIRMWARE_VERSION (the release tag, or dev-<sha>)
#  and that always wins, so a published build reports exactly what it was built
#  from. When the variable is unset — a local `pio run` — we fall back to
#  custom_firmware_version from platformio.ini, the YYMMDD date bumped on every
#  firmware change.

import os

Import("env")

version = os.environ.get("FIRMWARE_VERSION", "").strip()
source = "FIRMWARE_VERSION env var"

if not version:
    version = env.GetProjectOption("custom_firmware_version", "").strip()
    source = "platformio.ini custom_firmware_version"

if not version:
    raise SystemExit(
        "Error: no firmware version. Set custom_firmware_version in "
        "platformio.ini, or export FIRMWARE_VERSION."
    )

print("Firmware version: %s (from %s)" % (version, source))

env.Append(CPPDEFINES=[("FIRMWARE_VERSION", '\\"%s\\"' % version)])
