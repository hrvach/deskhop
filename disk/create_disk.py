#!/usr/bin/env python3
import os
import tempfile
import fs
from pyfatfs.PyFat import PyFat

def create_fat12_image(output_path: str, input_file_path: str) -> None:
    """
    Creates a 64 KB FAT12 image identical to the original mkdosfs + dd workflow.
    No sudo required.
    """
    image_size_for_calc = 2 * 1024 * 1024   # 2 MB — needed for correct geometry
    final_image_size = 64 * 1024            # 64 KB — what DeskHop embeds
    volume_label = "DESKHOP"

    # Use a named temporary file so pyfatfs can open it by path
    temp_file = tempfile.NamedTemporaryFile(delete=False)
    temp_path = temp_file.name
    temp_file.close()

    try:
        # 1. Zero-fill the 2 MB temporary image
        with open(temp_path, "wb") as f:
            f.write(bytes(image_size_for_calc))

        # 2. Format as FAT12 using pyfatfs low-level API
        formatter = PyFat(encoding="utf-8")
        formatter.mkfs(
            temp_path,
            fat_type=12,
            size=image_size_for_calc,
            label=volume_label.ljust(11)[:11]   # Ensure exactly 11 chars
        )
        formatter.close()

        # 3. Mount with high-level fs.py and copy config.htm
        fat_url = f"fat://{temp_path}"
        with fs.open_fs(fat_url) as fat_fs:
            with open(input_file_path, "rb") as src:
                fat_fs.writebytes("config.htm", src.read())

        # 4. Truncate to final 64 KB
        with open(temp_path, "rb") as src:
            data = src.read(final_image_size)

        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        with open(output_path, "wb") as dst:
            dst.write(data)

    finally:
        # Always remove temp file, even on KeyboardInterrupt
        try:
            os.unlink(temp_path)
        except OSError:
            pass


if __name__ == "__main__":
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_file = os.path.join(project_root, "disk", "disk.img")
    input_file = os.path.join(project_root, "webconfig", "config.htm")

    if not os.path.isfile(input_file):
        raise FileNotFoundError(f"Config file not found: {input_file}")

    create_fat12_image(output_file, input_file)
    print(f"disk.img created successfully → {output_file}")