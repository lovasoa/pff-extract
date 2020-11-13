CC := cc
CFLAGS += -lturbojpeg -O3 -Wall -Wextra -Werror

pff-extract: pff-extract.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf pff-extract
