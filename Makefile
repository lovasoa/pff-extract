CC := cc
CCFLAGS := -lturbojpeg -O3 -Wall -Wextra -Werror

pff-extract: pff-extract.c
	$(CC) -o $@ $< $(CCFLAGS)

clean:
	rm -rf pff-extract
