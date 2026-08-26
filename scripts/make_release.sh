#!/bin/sh
set -eu
version=0.1.0
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
dist="$root/dist"
stage="$dist/AmiWeather-$version"
archive="$dist/AmiWeather-$version.lha"
rm -rf "$stage"
mkdir -p "$stage/Docs"
cp "$root/build/amiga/AmiWeather" "$stage/AmiWeather"
cp "$root/docs/user_readme.txt" "$stage/README"
cp "$root/LICENSE" "$stage/LICENSE"
cp "$root/docs/architecture.md" "$stage/Docs/Architecture.txt"
sha256=$(sha256sum "$stage/AmiWeather" | awk '{print $1}')
sha1=$(sha1sum "$stage/AmiWeather" | awk '{print $1}')
md5=$(md5sum "$stage/AmiWeather" | awk '{print $1}')
cat > "$stage/AmiWeather.readme" <<EOF
Short:        Native weather client for classic AmigaOS
Author:       Per Gustav Ousdal
Contact:      amiweather@ousdal.org
Uploader:     (not specified)
Type:         comm/misc
Version:      $version
Architecture: 68020+, AmigaOS 3.x
Requires:     intuition.library, gadtools.library, graphics.library
Distribution: Aminet; MIT License

Description:
AmiWeather displays current conditions, a five-day daily forecast, extended
7/14-day forecast views, and the next twelve local forecast hours. It supports
Open-Meteo location search, up to eight saved locations, startup defaults, and
Celsius/Fahrenheit plus km/h/mph preferences. Weather Details shows available
current measurements. The interface uses native Intuition and GadTools
controls with small compiled-in few-color weather symbols.

Installation:
Copy AmiWeather to any drawer and run it from Workbench or Shell. No icon or
tooltypes are required.

Usage:
Use the Project, Location, Forecast, and Settings menus for secondary actions.
Change... searches; Saved Locations... saves/uses/removes/defaults places;
Preferences... selects units; Daily Forecast..., 7 Day Forecast..., 14 Day
Forecast..., Hourly..., and Weather Details... show stored forecast data; and
Refresh updates all weather sections.

Network requirements:
A working bsdsocket.library-compatible TCP/IP stack, DNS access, and AmiSSL 5
are required for HTTPS. AmiTCP_NG is tested but not mandatory.

Credits:
Weather/geocoding service: Open-Meteo. HTTP/API SDK: AmiAPI. HTTPS/TLS:
AmiSSL 5. No endorsement is implied.

Checksums (AmiWeather executable in this archive):
SHA-256: $sha256
SHA-1:   $sha1
MD5:     $md5
EOF
rm -f "$archive"
(CDPATH= cd -- "$dist" && jlha af "$(basename "$archive")" "$(basename "$stage")" >/dev/null)
sha256sum "$archive" > "$archive.sha256"
echo "Release archive: $archive"
echo "Archive SHA-256: $(awk '{print $1}' "$archive.sha256")"
echo "Executable SHA-256: $sha256"
