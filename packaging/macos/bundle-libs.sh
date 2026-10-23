#!/usr/bin/env bash
# 把 .app 里引用到的 Homebrew 动态库收进 Contents/Frameworks，并把引用改成包内相对路径。
# Homebrew 的 Qt 会链到 /opt/homebrew 下的 libpng / freetype 等，macdeployqt 不会带它们。
#
# 注意：dylib 的 otool -L 第一行是它自己的 install name（不是依赖），要单独用 -id 改，
# 否则会误当成依赖去 -change，改不动还一直报“没处理”。
#
# 用法: bundle-libs.sh <VoiletMark.app>
set -euo pipefail

app=$1
if [ ! -d "$app" ]; then
    echo "找不到 $app" >&2
    exit 1
fi

frameworks=$app/Contents/Frameworks
mkdir -p "$frameworks"

list_macho() {
    find "$app" -type f \( -perm -111 -o -name '*.dylib' \) -print
}

round=0
while [ "$round" -lt 12 ]; do
    round=$((round + 1))
    changed=0
    while IFS= read -r file; do
        # 先把这个文件自己的 install name 从 Homebrew 路径改掉
        self_id=$(otool -D "$file" 2>/dev/null | tail -n +2 | head -n1 || true)
        if [ -n "$self_id" ]; then
            case "$self_id" in
                /opt/homebrew/*|/usr/local/*)
                    install_name_tool -id "@executable_path/../Frameworks/$(basename "$self_id")" "$file" 2>/dev/null || true
                    ;;
            esac
        fi

        # 再把真正依赖的 Homebrew dylib 收进来
        while IFS= read -r dep; do
            case "$dep" in
                @*|/System/*|/usr/lib/*) continue ;;
            esac
            case "$dep" in
                /opt/homebrew/*|/usr/local/*)
                    base=$(basename "$dep")
                    if [ ! -f "$frameworks/$base" ]; then
                        cp "$dep" "$frameworks/$base"
                        chmod 755 "$frameworks/$base"
                        install_name_tool -id "@executable_path/../Frameworks/$base" "$frameworks/$base" 2>/dev/null || true
                        changed=1
                    fi
                    if ! install_name_tool -change "$dep" "@executable_path/../Frameworks/$base" "$file" 2>/dev/null; then
                        echo "改写引用失败: $file <- $dep" >&2
                    fi
                    ;;
            esac
        done < <(otool -L "$file" 2>/dev/null | tail -n +2 | awk '{print $1}')
    done < <(list_macho)

    [ "$changed" -eq 1 ] || break
done

remaining=$(mktemp)
while IFS= read -r file; do
    otool -L "$file" 2>/dev/null | tail -n +2 \
        | awk -v f="$file" '$1 ~ /^\/(opt\/homebrew|usr\/local)\// { print f ": " $1 }'
done < <(list_macho) > "$remaining"

if [ -s "$remaining" ]; then
    echo "还有 Homebrew 依赖没处理：" >&2
    head -30 "$remaining" >&2
    rm -f "$remaining"
    exit 1
fi
rm -f "$remaining"
echo "Homebrew 依赖已经收进 app"
