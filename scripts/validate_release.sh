#!/bin/sh
set -eu
version=0.1.0
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
dist="$root/dist"
stage="$dist/AmiWeather-$version"
archive="$dist/AmiWeather-$version.lha"
[ -f "$stage/AmiWeather" ] || { echo "missing staged executable" >&2; exit 1; }
[ -f "$stage/AmiWeather.readme" ] || { echo "missing Aminet readme" >&2; exit 1; }
[ -f "$stage/README" ] && [ -f "$stage/LICENSE" ] && [ -f "$stage/Docs/Architecture.txt" ]
case "$(file -b "$stage/AmiWeather")" in *AmigaOS*) ;; *) echo "staged file is not AmigaOS" >&2; exit 1;; esac
grep -q "Version:      $version" "$stage/AmiWeather.readme"
hash=$(sha256sum "$stage/AmiWeather" | awk '{print $1}')
grep -q "SHA-256: $hash" "$stage/AmiWeather.readme"
if find "$stage" -type f | grep -E '/(build|\.o$|\.a$|\.git)' >/dev/null; then
    echo "unexpected developer file in release" >&2
    exit 1
fi
[ -f "$archive" ] || { echo "missing archive" >&2; exit 1; }
lha t "$archive" >/dev/null
extract=$(mktemp -d "${TMPDIR:-/tmp}/amiweather-release.XXXXXX")
trap 'rm -rf "$extract"' EXIT HUP INT TERM
(CDPATH= cd -- "$extract" && lha x "$archive" >/dev/null)
extracted="$extract/AmiWeather-$version/AmiWeather"
[ -f "$extracted" ]
[ "$(sha256sum "$extracted" | awk '{print $1}')" = "$hash" ]
[ "$(sha256sum "$archive" | awk '{print $1}')" = "$(awk '{print $1}' "$archive.sha256")" ]
echo "Release validation passed"
