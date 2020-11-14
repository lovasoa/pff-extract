/**
 * pff file reader
 * Copyright Ophir LOJKINE
 * Released under the terms of the GPL
 *
 */

#define _DEFAULT_SOURCE 1
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#ifdef __APPLE__

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#else
#include <endian.h>
#endif
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <turbojpeg.h>

typedef struct {
  uint32_t size;
  uint8_t *data;
} jheader_t;

typedef struct {
  uint32_t version;
  uint32_t tile_size;
  uint32_t jheader_size;
  uint32_t ntiles;
  uint32_t width;
  uint32_t height;
  uint32_t jheader_num_elems;
  jheader_t *jheaders;
  uint64_t *tile_pointers;
} pff_t;

typedef struct {
  uint32_t footersize;
  uint32_t img_num;
  uint32_t zoom_level;
  uint32_t tile_num;
  uint32_t size;
  uint32_t jheader_num;
} tilefooter_t;

void fread_or_exit(void *ptr, size_t size, FILE *stream) {
  size_t read = fread(ptr, size, 1, stream);
  if (read != 1) {
    fprintf(stderr, "Failed to read %zu bytes at index %ld. The file is probably malformed.\n",
      size, ftell(stream));
    exit(1);
  }
}

uint32_t readuint32(FILE *file) {
  uint32_t r = 0;
  fread_or_exit(&r, sizeof(r), file);
  return be32toh(r);
}

void readat(long pos, uint32_t* ptr, FILE *file) {
  fseek(file, pos, SEEK_SET);
  *ptr = readuint32(file);
}

uint32_t width_in_tiles(pff_t* head) {
  uint32_t w = head->width / head->tile_size;
  if (head->width % head->tile_size != 0) w++;
  return w;
}

uint32_t height_in_tiles (pff_t* head) {
  uint32_t w = head->height / head->tile_size;
  if (head->width % head->tile_size != 0) w++;
  return w;
}

/**
 * Initializes a pff_t structure from a pff file
 **/
void read_head(pff_t *head, FILE *f) {
  readat(0x8, &head->version, f);
  readat(0x6c, &head->tile_size, f);
  readat(0x74, &head->jheader_size, f);
  readat(0x7c, &head->ntiles, f);
  readat(0x41c, &head->width, f);
  readat(0x420, &head->height, f);
  readat(0x424, &head->jheader_num_elems, f);

  fseek(f, 0, SEEK_END); long filesize = ftell(f);

  if (head->tile_size == 0 || head->jheader_size == 0 || head->width == 0 ||
      head->height == 0 || head->jheader_num_elems == 0 ||
      head->jheader_num_elems * 4 > filesize || head->ntiles * 8 > filesize) {
      perror("Invalid file");
      exit(1);
  }

  //Read the table of JFIF headers from the file
  head->jheaders = calloc(head->jheader_num_elems, sizeof(jheader_t));
  if (head->jheaders == 0) {
    perror("Unable to allocate memory for the JFIF headers table");
    exit(1);
  }
  fseek(f, 0x428, SEEK_SET);
  uint32_t i;
  for (i=0; i < head->jheader_num_elems; i++) {
    uint32_t size = readuint32(f);
    head->jheaders[i].size = size;
    uint8_t* data = malloc((size_t) size);
    if (data == NULL) {
      fprintf(stderr, "Failed to allocate %u bytes for tile header %u.", size, i);
      exit(1);
    }
    fread_or_exit(data, size, f);
    head->jheaders[i].data = data;
  }

  //Read the table of tile pointers
  fseek(f, 0x424 + head->jheader_size, SEEK_SET);
  head->tile_pointers = calloc(head->ntiles, 8);
  uint64_t ptr = 0;
  for (i=0; i < head->ntiles; i++) {
    fread_or_exit(&ptr, 8, f);
    if (ptr == 0) {
      fprintf(stderr, "Malformed file: null tile pointer\n");
      exit(1);
    }
    head->tile_pointers[i] = be64toh(ptr);
  }
}

/**
 * Write a tile to a file
 */
void read_tile(pff_t *head, FILE* fin, uint32_t tilenum, uint8_t* dest, char* tile_directory) {
  //Read tile contents
  uint64_t begin = (tilenum == 0)
                      ? 0x424 + head->jheader_size + 8*head->ntiles
                      : head->tile_pointers[tilenum-1];
  uint64_t end = head->tile_pointers[tilenum];
  uint64_t size = end - begin - sizeof(tilefooter_t);
  fseek(fin, end-sizeof(tilefooter_t), SEEK_SET);
  //Read footer
  tilefooter_t footer;
  size_t read = fread(&footer, sizeof(tilefooter_t), 1, fin);
  if (read != 1) {
    fprintf(stderr, "\nERROR: Unable to read tile footer for tile %u\n", tilenum);
    return;
  }
  footer.jheader_num = be32toh(footer.jheader_num);
  if (footer.jheader_num >= head->jheader_num_elems) {
    fprintf(stderr, "Invalid tile footer for tile %u\n", tilenum);
    return;
  }
  //Read jheader
  jheader_t jheader = head->jheaders[footer.jheader_num];
  //Write tile
  size_t rawjpgsize = (size_t)jheader.size + (size_t)size;
  if (rawjpgsize < size) {
    fprintf(stderr, "Integer overflow, raw jpg tile too large.\n");
    return;
  }
  uint8_t* rawjpg = malloc(rawjpgsize);
  if (rawjpg == NULL) {
    fprintf(stderr, "ERROR: Unable to allocate memory for the raw jpeg tile.\n");
    return;
  }
  memcpy(rawjpg, jheader.data, jheader.size);
  fseek(fin, begin, SEEK_SET);
  read = fread(rawjpg+jheader.size, size, 1, fin);
  if (read != 1) {
    free(rawjpg);
    fprintf(stderr, "\nERROR: Unable to read tile body for tile %u\n", tilenum);
    return;
  }


  if (tile_directory != NULL) {
    char* filename = malloc(512);
    snprintf(filename, 512, "%s/tile_%05u.jpg", tile_directory, tilenum);
    FILE* tmpf = fopen(filename, "w");
    if (tmpf != NULL) {
      fwrite(rawjpg, rawjpgsize, 1, tmpf);
      fclose(tmpf);
    } else perror("Unable to write tile to tile directory");
  }

  //Read the jpg image
  tjhandle dec = tjInitDecompress();
  assert(dec != NULL);
  int rawrgb_size = 3 * head->tile_size * head->tile_size;
  uint8_t* rawrgb = tjAlloc(rawrgb_size);
  if (rawrgb == NULL) {
    fprintf(stderr, "Unable to allocate enough memory for a single tile.\n");
    exit(1);
  }
  int tilew = 0, tileh = 0, tileSubSamp = 0;
  int ret = tjDecompressHeader2(dec, rawjpg, rawjpgsize, &tilew, &tileh, &tileSubSamp);
  if (ret != -1) ret = tjDecompress2(dec, rawjpg, rawjpgsize, rawrgb,
                  head->tile_size, 0, head->tile_size, TJPF_RGB, 0);
  free(rawjpg);

  if (ret == -1) {
    int error_code = tjGetErrorCode(dec);
    char* error_str = tjGetErrorStr2(dec);
    if (error_code == TJERR_WARNING) {
      // Only a warning, there is some decoded data, so continue anyway
      fprintf(stderr, "\nWARNING: Tile %u may be corrupted: %s\n", tilenum, error_str);
    } else {
      // Fatal error
      fprintf(stderr, "\nERROR: Unable to open tile %u as a JPEG file: %s\n",
        tilenum, error_str);
      memset(rawrgb, 0, rawrgb_size);
    }
  }
  tjDestroy(dec);

  int numTilesX = width_in_tiles(head);
  size_t offset = ((size_t)(tilenum % numTilesX) +
                     (tilenum / numTilesX) * (size_t)head->width) * (size_t)head-> tile_size * 3;
  int i;
  for (i=0; i < tileh; i++) {
    memcpy(dest + offset + i * head->width * 3,
           rawrgb + i * tilew * 3,
           tilew * 3);
  }
}

void read_file(pff_t *head, FILE* fin, FILE* fout, char* tile_directory) {
  uint64_t imgrgb_size = 3 * (uint64_t)head->width * (uint64_t)head->height;
  if (imgrgb_size > INT_MAX) {
    fprintf(stderr, "ERROR: image is too large, cannot allocate %.2f Gb.\n", (float)imgrgb_size/1000000000);
    exit(1);
  }
  void* imgrgb = tjAlloc(imgrgb_size);
  if (imgrgb == NULL) {
    fprintf(stderr, "ERROR: Could not allocate enough memory to decode this image. Needed %.2f Gb.", (float)imgrgb_size/1000000000);
    exit(1);
  }
  memset(imgrgb, 0, imgrgb_size);
  
  uint32_t i, totalTiles = width_in_tiles(head) * height_in_tiles(head);
  for (i=0; i<totalTiles; i++) {
    printf("\r Extracting tile %u out of %u", (i+1), totalTiles);
    read_tile(head, fin, i, imgrgb, tile_directory);
  }

  printf("\nCompressing the output image\n");
  tjhandle comp = tjInitCompress();
  unsigned char* imgjpg = NULL;
  unsigned long imgjpgsize = 0;
  tjCompress2(comp, imgrgb, head->width, 0, head->height,
               TJPF_RGB, &imgjpg, &imgjpgsize, TJSAMP_444, 100, 0);

  fwrite(imgjpg, imgjpgsize, 1, fout);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: %s input.pff [out.jpg] [tile_directory]\nRead the PFF file input.pff and extract its largest zoom level to out.jpg. If tile_directory is specified, every single tile at that level is exported as a separate file in that folder.", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "r");
  if (f == NULL) {
    perror("Unable to open input file");
    return 2;
  }

  FILE *fjpg = fopen((argc>2) ? argv[2] : "out.jpg", "w");
  if (fjpg == NULL) {
    perror("Unable to open output file");
    fclose(f);
    return 3;
  }

  char* tile_directory = (argc>3) ? argv[3] : NULL;

  pff_t head;
  read_head(&head, f);
  printf("version: 0x%x\nsize: %u x %u\nnumber of tiles: %u\n",
      head.version,head.width, head.height, head.ntiles); 

  read_file(&head, f, fjpg, tile_directory);

  fclose(f);
  fclose(fjpg);

  return 0;
}
