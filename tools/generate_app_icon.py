"""Render Photo Snap's Material-style landscape icon assets from the vector design."""

from io import BytesIO
from pathlib import Path
import struct

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
SCALE = 4
CANVAS = 512 * SCALE


def point(x, y):
    return (round(x * SCALE), round(y * SCALE))


image = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
draw = ImageDraw.Draw(image)
draw.rounded_rectangle((0, 0, CANVAS - 1, CANVAS - 1), radius=112 * SCALE, fill="#1565C0")
draw.rounded_rectangle((52 * SCALE, 52 * SCALE, 460 * SCALE, 460 * SCALE),
                        radius=52 * SCALE, fill="#F5F9FF")
draw.ellipse((306 * SCALE, 118 * SCALE, 402 * SCALE, 214 * SCALE), fill="#FFC107")
draw.polygon([point(*p) for p in [(72, 365), (194, 212), (273, 311), (318, 256),
                                   (440, 409), (440, 428), (72, 428)]], fill="#90CAF9")
draw.polygon([point(*p) for p in [(72, 391), (197, 262), (273, 339), (338, 266),
                                   (440, 391), (440, 428), (72, 428)]], fill="#26A69A")
draw.polygon([point(*p) for p in [(72, 359), (137, 293), (176, 333), (220, 280),
                                   (299, 362), (299, 428), (72, 428)]], fill="#00897B")
draw.polygon([point(*p) for p in [(194, 212), (273, 311), (300, 278), (266, 277),
                                   (243, 248), (224, 274)]], fill="#FFFFFF")
draw.polygon([point(*p) for p in [(197, 262), (273, 339), (289, 321), (269, 320),
                                   (244, 294), (226, 314)]], fill="#FFFFFF")

master = image.resize((1024, 1024), Image.Resampling.LANCZOS)
RESOURCES.mkdir(parents=True, exist_ok=True)
master.resize((512, 512), Image.Resampling.LANCZOS).save(RESOURCES / "photosnap.png", optimize=True)

master.save(RESOURCES / "photosnap.ico", format="ICO",
            sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])

icns_types = {16: b"icp4", 32: b"icp5", 64: b"icp6", 128: b"ic07",
              256: b"ic08", 512: b"ic09", 1024: b"ic10"}
chunks = []
for size, chunk_type in icns_types.items():
    buffer = BytesIO()
    master.resize((size, size), Image.Resampling.LANCZOS).save(buffer, format="PNG", optimize=True)
    payload = buffer.getvalue()
    chunks.append(chunk_type + struct.pack(">I", len(payload) + 8) + payload)
icns_payload = b"".join(chunks)
(RESOURCES / "photosnap.icns").write_bytes(b"icns" + struct.pack(">I", len(icns_payload) + 8) + icns_payload)
