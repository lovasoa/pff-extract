# pff-extract
pff (zoomify single-file zoomable image format) to jpeg converter.

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
