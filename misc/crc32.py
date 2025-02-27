import sys
import struct
import binascii

FLASH_SECTOR_SIZE = 4096
MAGIC_VALUE = 0xf00d

elf_filename = sys.argv[1]
output_filename = sys.argv[2]
version = sys.argv[3]

with open(elf_filename, 'r+b') as f:
    data = f.read()

    data = data[:-FLASH_SECTOR_SIZE]
    crc32_value = binascii.crc32(data) & 0xFFFFFFFF

with open(output_filename, 'wb') as f:
    f.write(struct.pack('<IHI', MAGIC_VALUE, int(version), crc32_value))
