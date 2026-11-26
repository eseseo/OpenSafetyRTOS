"""Stamp the final AN386 load image, including the initial SRAM data."""
import pathlib
import struct
import subprocess
import sys
import tempfile
import zlib


def stamp(objcopy, image):
    with tempfile.TemporaryDirectory(dir=image.parent) as work:
        raw = pathlib.Path(work) / "load.bin"
        crc = pathlib.Path(work) / "crc.bin"
        result = pathlib.Path(work) / "stamped.elf"
        subprocess.run([objcopy, "-O", "binary", "-R", ".image_crc", image, raw], check=True)
        crc.write_bytes(struct.pack("<I", zlib.crc32(raw.read_bytes())))
        subprocess.run([objcopy, "--update-section", f".image_crc={crc}", image, result], check=True)
        result.replace(image)


if __name__ == "__main__":
    stamp(sys.argv[1], pathlib.Path(sys.argv[2]))
