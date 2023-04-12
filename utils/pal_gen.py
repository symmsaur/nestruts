#!/usr/bin/env python3
"""Generate a palette for C++ from .pal file"""
import argparse

def generate(pal_file):
    counter = 0
    print("{")
    while (byte := pal_file.read(1)):
        counter += 1
        if counter % 3 == 1:
            print("    {", end="")
        print(f"0x{byte[0]:02x}", end="")
        if counter % 3 == 0:
            print("},")
        else:
            print(", ", end="")
    print("}")



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("filename")
    args = parser.parse_args()

    with open(args.filename, "rb") as pal_file:
        generate(pal_file)


if __name__ == "__main__":
    main()
