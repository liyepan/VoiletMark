#!/usr/bin/env bash
# 把 .app 里引用到的 Homebrew 动态库收进 Contents/Frameworks，并把引用改成包内相对路径。
# Homebrew 的 Qt 会链到 /opt/homebrew 下的 libpng / freetype 等，macdeployqt 不会带它们。
# 用法: bundle-libs.sh <VoiletMark.app>
set -euo pipefail

app=$1
if [ ! -d "$app" ]; then
    echo "找不到 $app" >&2
    exit 1
fi

frameworks=$app/Contents/Frameworks
mkdir -p "$frameworks"

round=0
while [ "$round" -lt 12 ]; do
    round=$((round + 1))
    changed=0
    while IFS= read -r file; do
        [ -f "$file" ] || continue
        while IFS= read -r dep; do
            case "$dep" in
                /opt/homebrew/*|/usr/local/*)
                    base=$(basename "$dep")
                    if [ ! -f "$frameworks/$base" ]; then
                        cp "$dep" "$frameworks/$base"
                        chmod 755 "$frameworks/$base"
                        install_name_tool -id "@executable_path/../Frameworks/$base" "$frameworks/$base" >/dev/null 2>&1 || true
                        changed=1
                    fi
                    install_name_tool -change "$dep" "@executable_path/../Frameworks/$base" "$file" >/dev/null 2>&1 || true
                    ;;
            esac
        done < <(otool -L "$file" | tail -n +2 | awk '{print $1}')
    done < <(find "$app" -type f \( -perm -111 -o -name '*.dylib' \) -print)

    [ "$changed" -eq 1 ] || break
done

left=$(find "$app" -type f \( -perm -111 -o -name '*.dylib' \) -exec otool -L {} + 2>/dev/null | grep -E '^[[:space:]]*(/opt/homebrew|/usr/local)/' | wc -l | tr -d ' ')
if [ "$left" -ne 0 ]; then
    echo "还有 $left 个 Homebrew 依赖没处理：" >&2
    find "$app" -type f \( -perm -111 -o -name '*.dylib' \) -exec otool -L {} + 2>/dev/null | grep -E '^[[:space:]]*(/opt/homebrew|/usr/local)/' | head -20 >&2
    exit 1
fi
