#!/usr/bin/env bash
# 把 `make install` 装出来的目录打成 .deb
# 用法: build-deb.sh <安装树> <版本> <amd64|arm64> <输出目录>
set -euo pipefail

usage() {
    echo "用法: $0 <安装树> <版本> <amd64|arm64> <输出目录>" >&2
    exit 2
}

[ $# -eq 4 ] || usage

stage=$(cd "$1" && pwd)
version=$2
arch=$3
mkdir -p "$4"
out=$(cd "$4" && pwd)

if [ ! -x "$stage/usr/bin/VoiletMark" ]; then
    echo "安装树里找不到 usr/bin/VoiletMark: $stage" >&2
    exit 1
fi

repo=$(cd "$(dirname "$0")/.." && pwd)

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

root=$work/root
mkdir -p "$root"
cp -a "$stage"/. "$root"/
chmod 755 "$root"
find "$root" -type d -exec chmod 755 {} +
install -Dm644 "$repo/README.md" "$root/usr/share/doc/voiletmark/README.md"

mkdir -p "$root/DEBIAN"
installed_size=$(du -sk "$root/usr" | cut -f1)

cat > "$root/DEBIAN/control" <<EOF
Package: voiletmark
Version: $version
Section: editors
Priority: optional
Architecture: $arch
Depends: libc6 (>= 2.28), libstdc++6, libqt5core5a (>= 5.12), libqt5gui5 (>= 5.12), libqt5widgets5 (>= 5.12), libgl1
Recommends: git
Installed-Size: $installed_size
Maintainer: liyepan <liyepan@linuxacme.com>
Homepage: https://git.linuxacme.com/liyepan/VoiletMark
Description: Lightweight cross-platform Markdown editor
 VoiletMark is a Markdown editor built with Qt 5 and C++17.
 .
 Features: in-place Markdown rendering, source mode, syntax
 highlighting for 11 languages, editable tables, PDF export,
 folder sidebar with document outline, Git integration, Chinese
 and English interface, light / dark themes.
EOF

name=$(printf 'voiletmark_%s_%s.deb' "$version" "$arch")
dpkg-deb --build --root-owner-group "$root" "$out/$name" >/dev/null
echo "已生成 $out/$name"
