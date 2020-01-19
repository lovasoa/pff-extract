CC := cc
CCFLAGS := -lturbojpeg

pff-extract: pff-extract.c
	$(CC) -o $@ $< $(CCFLAGS)
