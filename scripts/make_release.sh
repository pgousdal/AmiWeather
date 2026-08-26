#!/bin/sh
set -eu
version=0.1.0
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
dist="$root/dist"
stage="$dist/AmiWeather-$version"
archive="$dist/AmiWeather-$version.lha"
external_readme="$dist/AmiWeather-$version.readme"
qualified_sha256="72206c126aa01c57b6dd7032cafaf98e3112829a5d9fb50b7d8f5a409701cbfa"
rm -rf "$stage"
mkdir -p "$stage/Docs"
cp "$root/build/amiga/AmiWeather" "$stage/AmiWeather"
cp "$root/docs/user_readme.txt" "$stage/README"
cp "$root/LICENSE" "$stage/LICENSE"
cp "$root/docs/architecture.md" "$stage/Docs/Architecture.txt"
sha256=$(sha256sum "$stage/AmiWeather" | awk '{print $1}')
if [ "$sha256" != "$qualified_sha256" ]; then
    echo "qualified AmiWeather executable hash mismatch" >&2
    exit 1
fi
sha1=$(sha1sum "$stage/AmiWeather" | awk '{print $1}')
md5=$(md5sum "$stage/AmiWeather" | awk '{print $1}')
cat > "$external_readme" <<EOF
Short:        Native weather client for AmigaOS
Uploader:     amiweather@ousdal.org (Per Gustav Ousdal)
Author:       Per Gustav Ousdal
Type:         comm/misc
Version:      $version
Architecture: m68k-amigaos
Distribution: Aminet

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

Requirements:
AmigaOS 3.1 or later, a 68020 or later processor, a working
bsdsocket.library-compatible TCP/IP stack, DNS access, and AmiSSL 5.
No FPU or MMU is required. AmiWeather provisions its own runtime stack;
no manual Shell Stack command is required.

Contact:
amiweather@ousdal.org

License:
MIT License.

Checksums (AmiWeather executable in this archive):
SHA-256: $sha256
SHA-1:   $sha1
MD5:     $md5
EOF
if awk 'length($0) > 78 { exit 1 }' "$external_readme"; then :; else
    echo "Aminet readme line exceeds 78 characters" >&2; exit 1
fi
short=$(sed -n 's/^Short:[[:space:]]*//p' "$external_readme")
[ "${#short}" -le 40 ] || { echo "Short description exceeds 40 characters" >&2; exit 1; }
grep -q '^Short:' "$external_readme"
grep -q '^Uploader:.*amiweather@ousdal.org.*Per Gustav Ousdal' "$external_readme"
grep -q '^Author:[[:space:]]*Per Gustav Ousdal$' "$external_readme"
grep -q '^Type:[[:space:]]*comm/misc$' "$external_readme"
grep -q '^Version:[[:space:]]*0.1.0$' "$external_readme"
grep -q '^Architecture:[[:space:]]*m68k-amigaos$' "$external_readme"
grep -q '^Distribution:[[:space:]]*Aminet$' "$external_readme"
! grep -q '^Contact:' "$external_readme"
! grep -q 'Distribution:.*MIT' "$external_readme"
! LC_ALL=C grep -q "$(printf '\r')" "$external_readme"
cp "$external_readme" "$stage/AmiWeather.readme"
cmp -s "$external_readme" "$stage/AmiWeather.readme"
rm -f "$archive"
(CDPATH= cd -- "$dist" && jlha af "$(basename "$archive")" "$(basename "$stage")" >/dev/null)
sha256sum "$archive" > "$archive.sha256"
echo "Release archive: $archive"
echo "Archive SHA-256: $(awk '{print $1}' "$archive.sha256")"
echo "Executable SHA-256: $sha256"
echo "Aminet readme: $external_readme"
