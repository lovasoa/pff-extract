# pff-extract
pff (zoomify single-file zoomable image format) to jpeg converter.

The pff file format is proprietary, and [I reversed-engineered it](https://github.com/lovasoa/pff-extract/wiki/PFF-file-format) using only a few sample files. This program is based on this reverse-engineering work, and thus might not work on every pff file. If it doesn't work for you, please open a bug.

## Usage
```
pff-extract input.pff [output.jpg]
```

## Compilation
```
gcc -o pff-extract pff-extract.c -lturbojpeg
```

### Dependencies
 * [libjpeg-turbo] to decompress the tiles and to compress the resulting image
