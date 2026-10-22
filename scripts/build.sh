#!/usr/bin/env bash
# 一键构建。可用 QT_PREFIX 指定 Qt 前缀，例如：
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

cmake_args=(-S "$root" -B "$root/build" -DCMAKE_BUILD_TYPE=Release)
if [[ -n "${QT_PREFIX:-}" ]]; then
    cmake_args+=(-DCMAKE_PREFIX_PATH="$QT_PREFIX")
    echo "使用 Qt: $QT_PREFIX"
fi

cmake "${cmake_args[@]}"
cmake --build "$root/build" -j"$(nproc)"

echo
echo "构建完成: $root/build/VoiletMark"
