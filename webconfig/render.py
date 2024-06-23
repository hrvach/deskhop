#!/usr/bin/python3

# Takes a HTML file, outputs a minified and compressed version that self-decompresses when loaded.
# This way, the device config page can be fitted in a small 64 kB "flash" partition and distributed
# with the main binary.

from jinja2 import Environment, FileSystemLoader
from form import *
import base64
import zlib

# Input and output
TEMPLATE_PATH = "templates/"
INPUT_FILENAME = "main.html"
PACKER_FILENAME = "packer.j2"
OUTPUT_FILENAME = "config.htm"
OUTPUT_UNPACKED = "config-unpacked.htm"

def render(filename, *args, **kwargs):
    env = Environment(loader=FileSystemLoader(TEMPLATE_PATH))
    template = env.get_template(filename)
    return template.render(*args, **kwargs)


def write_file(payload, filename=OUTPUT_FILENAME):
    with open(filename, 'w') as file:
        file.write(payload)


def encode_file(payload):
    # Compress using raw DEFLATE
    compressed_data = zlib.compress(payload.encode('utf-8'))[2:-4]

    # Encode to base64
    base64_compressed_data = base64.b64encode(compressed_data).decode('utf-8')

    return base64_compressed_data


if __name__ == "__main__":    
    # Read main template contents
    webpage = render(
        INPUT_FILENAME, 
        screen_A=output_A(), 
        screen_B=output_B(), 
        status=output_status(),
        config=output_config(),
    )

    # Compress file and encode to base64
    encoded_data = {'payload': encode_file(webpage)}

    # Tiny Inflate JS decoder (https://github.com/foliojs/tiny-inflate)
    # Decompress the data and replace existing HTML with the decoded version
    self_extracting_webpage = render(PACKER_FILENAME, encoded_data)

    # Write data to output filename
    write_file(self_extracting_webpage)
    
    # Write unpacked webpage
    write_file(webpage, OUTPUT_UNPACKED)