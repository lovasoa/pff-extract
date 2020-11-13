# pff-extract
Pyramidal File Format (PFF) to jpeg converter. PFF is zoomify's single-file zoomable image format.

The pff file format is proprietary, and [I reversed-engineered it](https://github.com/lovasoa/pff-extract/wiki/Zoomify-PFF-file-format-documentation) using only a few sample files. This program is based on this reverse-engineering work, and thus might not work on every pff file. If it doesn't work for you, please [open a bug](https://github.com/lovasoa/pff-extract/issues/new).

## Installation

We distribute linux and MacOS binaries on our [release page](https://github.com/lovasoa/pff-extract/releases).
If you are on windows, you can use the linux (ubuntu) binaries with [WSL](https://docs.microsoft.com/en-us/windows/wsl/install-win10).

## Usage
```
pff-extract input.pff [output.jpg]
```

## Compilation
```
gcc -o pff-extract pff-extract.c -lturbojpeg
```

### Dependencies
 * [libjpeg-turbo](http://www.libjpeg-turbo.org/) v2.0+ to decompress the tiles
   and to compress the resulting image.
   In ubuntu 20.04+, it can be installed with `sudo apt install libturbojpeg0-dev`
