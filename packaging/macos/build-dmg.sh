#!/usr/bin/env bash
# 用 hdiutil 打一个拖拽式 dmg（里面带 /Applications 快捷方式）
# 用法: build-dmg.sh <VoiletMark.app> <版本> <arm64|x86_64> <输出目录>
set -euo pipefail

app=$1
version=$2
arch=$3
out=$4

if [ ! -d "$app" ]; then
    echo "找不到 $app" >&2
    exit 1
fi

mkdir -p "$out"
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT

ditto "$app" "$stage/VoiletMark.app"
ln -s /Applications "$stage/Applications"

name="VoiletMark-$version-macos-$arch.dmg"
hdiutil create -volname "VoiletMark $version" -srcfolder "$stage" -ov -format UDZO "$out/$name" >/dev/null
echo "已生成 $out/$name"
