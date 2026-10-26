#!/usr/bin/env bash
# 把 `make install` 装出来的目录打成 .rpm（用 rpmbuild，不需要 root）
# 用法: build-rpm.sh <安装树> <版本> <x86_64|aarch64> <输出目录>
set -euo pipefail

usage() {
    echo "用法: $0 <安装树> <版本> <x86_64|aarch64> <输出目录>" >&2
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

top=$work/rpmbuild
mkdir -p "$top/BUILD" "$top/BUILDROOT" "$top/RPMS" "$top/SOURCES" "$top/SPECS" "$top/SRPMS"
cp -a "$stage" "$top/SOURCES/root"
find "$top/SOURCES/root" -type d -exec chmod 755 {} +
install -Dm644 "$repo/README.md" "$top/SOURCES/root/usr/share/doc/voiletmark/README.md"

cat > "$top/SPECS/voiletmark.spec" <<EOF
Name:        voiletmark
Version:     $version
Release:     1
Summary:     Lightweight cross-platform Markdown editor
License:     Proprietary
URL:         https://git.linuxacme.com/liyepan/VoiletMark
BuildArch:   $arch
AutoReqProv: yes
# GNOME/Wayland 下要拿 Adwaita 标题栏得靠这个装饰插件；Fedora 默认没装，
# 用弱依赖让 dnf 顺带装上（源里没有这个包时忽略）
Recommends:  qadwaitadecorations-qt5

%description
VoiletMark is a Markdown editor built with Qt 5 and C++17. It renders
Markdown in place, exports PDF, highlights code in 11 languages, edits
tables directly, shows a folder sidebar with document outline and can
run Git commands against the opened repository.

%install
mkdir -p %{buildroot}
cp -a %{_sourcedir}/root/. %{buildroot}/

%files
%defattr(-,root,root,-)
%{_bindir}/VoiletMark
%{_datadir}/applications/voiletmark.desktop
%{_datadir}/icons/hicolor/256x256/apps/voiletmark.png
%{_docdir}/voiletmark/README.md
EOF

rpmbuild --define "_topdir $top" -bb "$top/SPECS/voiletmark.spec" >/dev/null
cp "$top/RPMS/$arch"/*.rpm "$out/"
echo "已生成 $(ls "$out"/*.rpm)"
