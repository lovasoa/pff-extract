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

uint32_t readuint32(FILE *file) {
  uint32_t r = 0;
  size_t read = fread(&r, sizeof(r), 1, file);
  assert(read == 1);
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
  uint32_t i, size;
  fseek(f, 0x428, SEEK_SET);
  for (i=0; i < head->jheader_num_elems; i++) {
    size = readuint32(f);
    head->jheaders[i].size = size;
    head->jheaders[i].data = malloc((size_t) size);
    size_t read = fread(head->jheaders[i].data, size, 1, f);
    assert(read == 1);
  }

  //Read the table of tile pointers
  fseek(f, 0x424 + head->jheader_size, SEEK_SET);
  head->tile_pointers = calloc(head->ntiles, 8);
  uint64_t ptr = 0;
  for (i=0; i < head->ntiles; i++) {
    size_t read = fread(&ptr, 8, 1, f);
    assert(read == 1);
    assert(ptr != 0);
    head->tile_pointers[i] = be64toh(ptr);
  }
}

/**
 * Write a tile to a file
 */
void read_tile(pff_t *head, FILE* fin, uint32_t tilenum, void* dest) {
  //Read tile contents
  uint64_t begin = (tilenum == 0)
                      ? 0x424 + head->jheader_size + 8*head->ntiles
                      : head->tile_pointers[tilenum-1];
  uint64_t end = head->tile_pointers[tilenum];
  fseek(fin, end-sizeof(tilefooter_t), SEEK_SET);
  uint64_t size = end-begin-sizeof(tilefooter_t);
  //Read footer
  tilefooter_t footer;
  size_t read;
  read = fread(&footer, sizeof(tilefooter_t), 1, fin);
  assert(read == 1);
  footer.jheader_num = be32toh(footer.jheader_num);
  //Read jheader
  jheader_t jheader = head->jheaders[footer.jheader_num];
  //Write tile
  unsigned int rawjpgsize = jheader.size + size;
  void* rawjpg = malloc(rawjpgsize);
  memcpy(rawjpg, jheader.data, jheader.size);
  fseek(fin, begin, SEEK_SET);
  read = fread(rawjpg+jheader.size, size, 1, fin);
  assert(read == 1);

  //Read the jpg image
  tjhandle dec = tjInitDecompress();
  void* rawrgb = tjAlloc(3*head->tile_size*head->tile_size);
  int tilew = 0, tileh = 0, tileSubSamp = 0;
  tjDecompressHeader2(dec, rawjpg, rawjpgsize, &tilew, &tileh, &tileSubSamp);
  int ret = tjDecompress2(dec, rawjpg, rawjpgsize, rawrgb,
                  head->tile_size, 0, head->tile_size, TJPF_RGB, 0);

  if (ret == -1) {
    fprintf(stderr, "\nUnable to open tile %d as a JPEG file: %s\n", tilenum, tjGetErrorStr());
    // There may still be some decoded data, so continue anyway
  }

  int numTilesX = width_in_tiles(head);
  uint32_t offset = ((tilenum % numTilesX) +
                     (tilenum / numTilesX) * head->width) * head-> tile_size * 3;
  int i;
  for (i=0; i < tileh; i++) {
    memcpy(dest + offset + i * head->width * 3,
           rawrgb + i * tilew * 3,
           tilew * 3);
  }
  free(rawjpg);
}

void read_file(pff_t *head, FILE* fin, FILE* fout) {
  void* imgrgb = tjAlloc(3 * head->width*head->height);
  
  uint32_t i, totalTiles = width_in_tiles(head) * height_in_tiles(head);
  for (i=0; i<totalTiles; i++) {
    printf("\r Extracting tile %d out of %d", (i+1), totalTiles);
    read_tile(head, fin, i, imgrgb);
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
    printf("Usage: %s input.pff [out.jpg]\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "r");
  if (f == NULL) {
    perror("Unable to open input file");
    return 2;
  }

  FILE *fjpg = fopen((argc>2) ? argv[2] : "out.jpg", "w");
  if (f == NULL) {
    perror("Unable to open output file");
    return 3;
  }

  pff_t head;
  read_head(&head, f);
  printf("version: 0x%x\nsize: %d x %d\nnumber of tiles: %d\n",
      head.version,head.width, head.height, head.ntiles); 

  read_file(&head, f, fjpg);

  fclose(f);
  fclose(fjpg);

  return 0;
}
