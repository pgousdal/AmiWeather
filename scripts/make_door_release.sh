#!/bin/sh
set -eu

version=0.1.0
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
dist="$root/dist"
binary="$root/build/amiga/AmiWeatherDoor"
stage="$dist/AmiWeatherDoor-$version"
archive="$dist/AmiWeatherDoor-$version.lha"
external_readme="$dist/AmiWeatherDoor-$version.readme"
qualified_sha256=2c50287a4ef5ad7975c6d40210fb0c9dc856a0b27de92b2e65e7dc1c3bd5b33f

[ -f "$binary" ] || { echo "missing AmigaDoor binary: $binary" >&2; exit 1; }
file "$binary" | grep -q 'AmigaOS' || { echo "not an AmigaOS executable" >&2; exit 1; }
sha256=$(sha256sum "$binary" | awk '{print $1}')
[ "$sha256" = "$qualified_sha256" ] || {
    echo "qualified AmiWeatherDoor executable hash mismatch" >&2; exit 1;
}
sha1=$(sha1sum "$binary" | awk '{print $1}')
md5=$(md5sum "$binary" | awk '{print $1}')

rm -rf "$stage"
mkdir -p "$stage/Docs" "$stage/ABBS"
cp "$binary" "$stage/AmiWeatherDoor"
cp "$root/docs/door_readme.txt" "$stage/README"
cp "$root/LICENSE" "$stage/LICENSE"
cp "$root/docs/architecture.md" "$stage/Docs/Architecture.txt"
cp "$root/contrib/abbs/README.ABBS" "$stage/Docs/ABBS.txt"
cp "$root/contrib/abbs/AmiWeather.script" "$stage/ABBS/AmiWeather.script"
cp "$root/contrib/abbs/Node0Config.example" "$stage/ABBS/Node0Config.example"
cp "$root/contrib/abbs/Node0Menu.example" "$stage/ABBS/Node0Menu.example"

cat > "$external_readme" <<EOF
Short:        Weather BBS door for ABBS 3.x
Uploader:     amiweather@ousdal.org (Per Gustav Ousdal)
Author:       Per Gustav Ousdal
Type:         comm/bbs
Version:      $version
Architecture: m68k-amigaos
Distribution: Aminet

Description:
AmiWeatherDoor is a plain-ASCII terminal/BBS frontend for AmiWeather. ABBS
3.3 is the first and currently only runtime-qualified BBS. It provides
current conditions, hourly, daily, 7-day and 14-day forecasts, weather
details, location search, refresh, cancellation, and clean return to ABBS.

Usage:
H Hourly, D Daily, 7 7 Day, A 14 Day, W Weather Details, L Location,
R Refresh, Q Return to BBS. Empty location input or 0 cancels. The interface
uses bounded ASCII terminal I/O and does not require ANSI support.

Installation:
Copy AmiWeatherDoor and AmiWeather.script to ABBS:Doors/AmiWeather/. Add
the S Shell-door entry from Node0Config.example to Node0Config or NodeXConfig,
add matching Node0Menu text, and run OPEN n. Keep failat 2147483647 before
the executable and EndCLI after it so a failed door cannot abort cleanup.

Requirements:
AmigaOS 3.1 or later, 68020 or later, no FPU or MMU, a working
bsdsocket.library-compatible TCP/IP stack, DNS, Internet access, and AmiSSL 5.
The door provisions its own private 32 KiB runtime stack; no manual Stack
command is required.

Credits:
Weather/geocoding: Open-Meteo. HTTP/API SDK: AmiAPI. HTTPS/TLS: AmiSSL 5.

Contact:
amiweather@ousdal.org

License:
MIT License.

Checksums (AmiWeatherDoor executable in this archive):
SHA-256: $sha256
SHA-1:   $sha1
MD5:     $md5
EOF

cp "$external_readme" "$stage/AmiWeatherDoor.readme"
cmp -s "$external_readme" "$stage/AmiWeatherDoor.readme"

short=$(sed -n 's/^Short:[[:space:]]*//p' "$external_readme")
[ "$short" = "Weather BBS door for ABBS 3.x" ]
[ "${#short}" -le 40 ]
awk 'length($0) > 78 { exit 1 }' "$external_readme"
! LC_ALL=C grep -q "$(printf '\r')" "$external_readme"
! LC_ALL=C grep -q "$(printf '\0')" "$external_readme"
grep -q '^Short:' "$external_readme"
grep -q '^Uploader:.*amiweather@ousdal.org.*Per Gustav Ousdal' "$external_readme"
grep -q '^Author:[[:space:]]*Per Gustav Ousdal$' "$external_readme"
grep -q '^Type:[[:space:]]*comm/bbs$' "$external_readme"
grep -q '^Version:[[:space:]]*0.1.0$' "$external_readme"
grep -q '^Architecture:[[:space:]]*m68k-amigaos$' "$external_readme"
grep -q '^Distribution:[[:space:]]*Aminet$' "$external_readme"
grep -q 'AmigaOS 3.1' "$external_readme"
grep -q '68020' "$external_readme"
grep -q 'AmiSSL 5' "$external_readme"
grep -q 'bsdsocket.library' "$external_readme"
! grep -Eqi '(run|issue|set).{0,20}Stack[[:space:]]+[0-9]' "$external_readme"
! grep -qi 'AWDBG' "$external_readme"

rm -f "$archive" "$archive.sha256"
(CDPATH= cd "$dist" && jlha af "$(basename "$archive")" "$(basename "$stage")" >/dev/null)
sha256sum "$archive" > "$archive.sha256"
archive_sha256=$(sha256sum "$archive" | awk '{print $1}')
[ "$(awk '{print $1}' "$archive.sha256")" = "$archive_sha256" ]

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
lha l "$archive" >/dev/null
lha xw="$tmp" "$archive" >/dev/null
extracted="$tmp/AmiWeatherDoor-$version"
cmp -s "$binary" "$stage/AmiWeatherDoor"
cmp -s "$binary" "$extracted/AmiWeatherDoor"
cmp -s "$external_readme" "$stage/AmiWeatherDoor.readme"
cmp -s "$external_readme" "$extracted/AmiWeatherDoor.readme"
[ "$(sha256sum "$extracted/AmiWeatherDoor" | awk '{print $1}')" = "$sha256" ]
[ "$(sha1sum "$extracted/AmiWeatherDoor" | awk '{print $1}')" = "$sha1" ]
[ "$(md5sum "$extracted/AmiWeatherDoor" | awk '{print $1}')" = "$md5" ]

echo "Door archive: $archive"
echo "Archive SHA-256: $archive_sha256"
echo "Executable SHA-256: $sha256"
echo "Aminet readme: $external_readme"
