#!/usr/bin/env python3

import sys
import os
import re
import struct
import argparse
import json
import mmap_conv

# Memory map configuration for SG2002
MEMORY_MAP = {
    "ddr": {
        "base": 0x80000000,
        "size": 0x10000000,  # 256MB
        "type": "ddr"
    },
    "mmio": {
        "base": 0x70000000,
        "size": 0x10000000,
        "type": "mmio"
    }
}

def main():
    parser = argparse.ArgumentParser(description='Generate memory map for SG2002')
    parser.add_argument('--output', '-o', required=True, help='Output file path')
    args = parser.parse_args()

    # Convert memory map to binary format
    mmap_bin = mmap_conv.mmap_to_bin(MEMORY_MAP)
    
    # Write to output file
    with open(args.output, 'wb') as f:
        f.write(mmap_bin)

if __name__ == '__main__':
    main() 
