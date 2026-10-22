#!/usr/bin/env bash
# 一键构建（qmake）。指定 Qt：
#   QMAKE=$HOME/Qt5/5.15.2/gcc_64/bin/qmake ./scripts/build.sh
#   QT_PREFIX=$HOME/Qt5/5.15.2/gcc_64 ./scripts/build.sh
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ -z "${QT_PREFIX:-}" ]]; then
    for candidate in "$HOME/Qt5/5.15.2/gcc_64" "$HOME/Qt/5.15.2/gcc_64"; do
        if [[ -d "$candidate" ]]; then
            QT_PREFIX="$candidate"
            break
        fi
    done
fi

qmake_bin="${QMAKE:-}"
if [[ -z "$qmake_bin" ]]; then
    if command -v qmake >/dev/null 2>&1; then
        qmake_bin="$(command -v qmake)"
    elif [[ -n "${QT_PREFIX:-}" && -x "$QT_PREFIX/bin/qmake" ]]; then
        qmake_bin="$QT_PREFIX/bin/qmake"
    fi
fi

if [[ -z "$qmake_bin" ]]; then
    echo "找不到 qmake；请设置 QT_PREFIX 或 QMAKE 指向 Qt 安装目录。" >&2
    exit 1
fi

echo "使用 qmake: $qmake_bin"
build_dir="$root/build"
mkdir -p "$build_dir"
(
    cd "$build_dir"
    "$qmake_bin" "$root/VoiletMark.pro"
    make -j"$(nproc)"
)

echo
echo "构建完成: $build_dir/VoiletMark"
