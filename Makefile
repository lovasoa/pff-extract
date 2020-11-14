CC := cc
CFLAGS += -lturbojpeg -O3 -Wall -Wextra -Werror -pedantic

pff-extract: pff-extract.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf pff-extract
