// Copyright (c) Kuba Szczodrzyński 2026-3-3.

#include <stdint.h>
#include <stdio.h>

#define BUFFER_SIZE (512 * 1024)

extern unsigned int crc32(const unsigned char *message, unsigned int len, unsigned int crc);

static int file_copy(void *buf, uint64_t size, FILE *dst, FILE *src, unsigned int *crc_ptr) {
	unsigned int crc = (crc_ptr != NULL) ? *crc_ptr : 0;
	// Transfer data in chunks
	while (size) {
		// Cap read length to buffer size
		uint32_t len = size;
		if (len > BUFFER_SIZE)
			len = BUFFER_SIZE;
		// Read from input file
		if (fread(buf, len, 1, src) != 1) {
			perror("Reading data failed");
			return -1;
		}
		// Update the CRC32
		if (crc_ptr != NULL)
			crc = crc32(buf, len, crc);
		// Write to output file
		if (fwrite(buf, len, 1, dst) != 1) {
			perror("Writing data failed");
			return -1;
		}
		// Decrease remaining size
		size -= len;
	}
	if (crc_ptr != NULL)
		*crc_ptr = crc;
	return 0;
}
