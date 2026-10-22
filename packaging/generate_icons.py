#!/usr/bin/env python3
"""从 logo.png 生成各平台图标。

logo.png 是黑色字标 + 透明背景：放在深色 Windows 任务栏里会“看不见”。
所以先给它加一块白色圆角底板，再导出 ico / icns / png。

用法：python3 packaging/generate_icons.py
"""

import os
import struct
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(ROOT, "logo.png")
OUT_DIR = os.path.join(ROOT, "packaging")

MASTER = 256
SUPERSAMPLE = 2
PLATE_RADIUS = 52
GLYPH_RATIO = 0.66
PLATE_COLOR = (255, 255, 255)
BORDER_COLOR = (222, 226, 230)

SIZES = (16, 24, 32, 48, 64, 128, 256)


def run(args, **kwargs):
    return subprocess.run(args, check=True, capture_output=True, **kwargs)


def load_rgba(path, size):
    return run(["ffmpeg", "-v", "error", "-i", path, "-vf",
                f"scale={size}:{size}:flags=lanczos", "-pix_fmt", "rgba",
                "-f", "rawvideo", "-"]).stdout


def save_png(path, rgba, size):
    run(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "rgba",
         "-s", f"{size}x{size}", "-i", "-", path], input=rgba)


def rounded_mask(size, radius):
    """圆角矩形覆盖率，左上角为 (0,0)。"""
    mask = bytearray(size * size)
    r = radius
    for y in range(size):
        for x in range(size):
            dx = min(x, size - 1 - x)
            dy = min(y, size - 1 - y)
            if dx >= r or dy >= r:
                mask[y * size + x] = 255
                continue
            ddx = r - dx
            ddy = r - dy
            dist = (ddx * ddx + ddy * ddy) ** 0.5
            if dist <= r:
                mask[y * size + x] = 255
    return mask


def compose_master():
    size = MASTER * SUPERSAMPLE
    radius = PLATE_RADIUS * SUPERSAMPLE
    border = 3 * SUPERSAMPLE
    glyph_size = int(size * GLYPH_RATIO)
    offset = (size - glyph_size) // 2

    plate = rounded_mask(size, radius)
    glyph = load_rgba(SOURCE, glyph_size)

    canvas = bytearray(size * size * 4)
    for y in range(size):
        for x in range(size):
            idx = (y * size + x) * 4
            if not plate[y * size + x]:
                continue
            dx = min(x, size - 1 - x)
            dy = min(y, size - 1 - y)
            color = BORDER_COLOR if (dx < border or dy < border) else PLATE_COLOR
            canvas[idx:idx + 4] = bytes((color[0], color[1], color[2], 255))

    for gy in range(glyph_size):
        for gx in range(glyph_size):
            src = (gy * glyph_size + gx) * 4
            alpha = glyph[src + 3] / 255.0
            if alpha <= 0.0:
                continue
            dst = ((gy + offset) * size + (gx + offset)) * 4
            for channel in range(3):
                canvas[dst + channel] = int(
                    glyph[src + channel] * alpha + canvas[dst + channel] * (1.0 - alpha))

    # 缩小回 256，顺便得到平滑的边缘
    return run(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "rgba",
                "-s", f"{size}x{size}", "-i", "-", "-vf",
                f"scale={MASTER}:{MASTER}:flags=lanczos", "-frames:v", "1",
                "-f", "rawvideo", "-"], input=bytes(canvas)).stdout


def bmp_entry(rgba, size):
    header = struct.pack("<IiiHHIIiiII", 40, size, size * 2, 1, 32, 0,
                         size * size * 4, 0, 0, 0, 0)
    rows = []
    for y in range(size - 1, -1, -1):
        row = bytearray()
        for x in range(size):
            i = (y * size + x) * 4
            r, g, b, a = rgba[i:i + 4]
            row += bytes((b, g, r, a))
        rows.append(bytes(row))

    mask_row_bytes = ((size + 31) // 32) * 4
    mask = bytearray()
    for y in range(size - 1, -1, -1):
        bits = bytearray(mask_row_bytes)
        for x in range(size):
            if rgba[(y * size + x) * 4 + 3] < 128:
                bits[x // 8] |= 0x80 >> (x % 8)
        mask += bits
    return header + b"".join(rows) + bytes(mask)


def write_ico(path, images):
    header = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    entries = b""
    payload = b""
    for size, data in images:
        dim = 0 if size >= 256 else size
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(data), offset)
        payload += data
        offset += len(data)
    with open(path, "wb") as handle:
        handle.write(header + entries + payload)


def chunk(kind, data):
    return kind + struct.pack(">I", 8 + len(data)) + data


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    master = compose_master()
    master_png = os.path.join(OUT_DIR, "voiletmark.png")
    save_png(master_png, master, MASTER)

    pngs = {}
    master_bytes = open(master_png, "rb").read()
    for size in SIZES:
        if size == MASTER:
            pngs[size] = master_bytes
            continue
        temp = os.path.join(OUT_DIR, f"_tmp_{size}.png")
        run(["ffmpeg", "-y", "-v", "error", "-i", master_png, "-vf",
             f"scale={size}:{size}:flags=lanczos", "-frames:v", "1", temp])
        pngs[size] = open(temp, "rb").read()
        os.remove(temp)

    images = []
    for size in SIZES:
        if size == MASTER:
            images.append((size, pngs[size]))
        else:
            images.append((size, bmp_entry(load_rgba(master_png, size), size)))
    write_ico(os.path.join(OUT_DIR, "voiletmark.ico"), images)

    body = chunk(b"ic07", pngs[128]) + chunk(b"ic08", pngs[256])
    with open(os.path.join(OUT_DIR, "voiletmark.icns"), "wb") as handle:
        handle.write(b"icns" + struct.pack(">I", 8 + len(body)) + body)

    print("icons written to", OUT_DIR)


if __name__ == "__main__":
    main()
