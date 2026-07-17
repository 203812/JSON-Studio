#!/usr/bin/env python3
"""Pack PNGs into a macOS .icns. ImageMagick's ICNS coder only writes one size;
this writes the proper multi-size container so Finder and the Dock pick the
right resolution. Run make-icns.sh (which renders the PNGs first)."""
import struct
import sys

# OSType code -> pixel size. These types all accept PNG-encoded data on modern
# macOS.
ENTRIES = [
    (b"icp4", 16),
    (b"icp5", 32),
    (b"icp6", 64),
    (b"ic07", 128),
    (b"ic08", 256),
    (b"ic09", 512),
    (b"ic10", 1024),
]


def main(src_dir: str, out_path: str) -> None:
    chunks = []
    for ostype, size in ENTRIES:
        with open(f"{src_dir}/m{size}.png", "rb") as f:
            data = f.read()
        chunks.append(ostype + struct.pack(">I", len(data) + 8) + data)

    body = b"".join(chunks)
    header = b"icns" + struct.pack(">I", len(body) + 8)
    with open(out_path, "wb") as f:
        f.write(header + body)
    print(f"wrote {out_path} ({len(body) + 8} bytes, {len(ENTRIES)} sizes)")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
