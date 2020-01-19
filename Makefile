CC := cc
CCFLAGS := -lturbojpeg -Wall -Werror

pff-extract: pff-extract.c
	$(CC) -o $@ $< $(CCFLAGS)
