// Copyright (c) Kuba Szczodrzyński 2026-3-3.

#include "amlimage.h"

#define CRC32_FAST
#include "crc32.c"
#include "utils.c"

int main(int argc, char *argv[]) {
	int ret = EXIT_FAILURE;

	// Check required argument count
	if (argc < 3) {
		fprintf(stderr, "Usage: %s OUTPUT_FILE [FILE_TYPE,MAIN_TYPE,SUB_TYPE=INPUT_FILE]...\n", argv[0]);
		return ret;
	}

	// Allocate memory for the image header
	uint32_t num_items = argc - 2;
	uint32_t hdr_size  = sizeof(aml_image_t) + sizeof(aml_file_t) * num_items;
	aml_hdr_t *hdr	   = calloc(hdr_size, 1);
	if (hdr == NULL) {
		perror("Cannot allocate header");
		goto exit;
	}

	// Allocate memory for input filenames
	char **filenames = calloc(num_items, sizeof(char *));
	if (filenames == NULL) {
		perror("Cannot allocate filenames");
		goto free_hdr;
	}

	// Build a default image header
	hdr->image.version	 = 2;
	hdr->image.magic	 = IMAGE_MAGIC;
	hdr->image.align	 = 8;
	hdr->image.num_items = num_items;

	// Parse each argument as an input file
	uint32_t offset = hdr_size;
	for (uint32_t i = 0; i < num_items; ++i) {
		char *arg		  = argv[2 + i];
		aml_file_t *file = &hdr->files[i];

		// Split input filename by '='
		char *filename = strchr(arg, '=');
		if (filename == NULL)
			goto invalid_fmt;
		*filename++ = '\0';
		// Split file_type/main_type by ','
		const char *file_type = arg;
		char *main_type		  = strchr(file_type, ',');
		if (main_type == NULL)
			goto invalid_fmt;
		*main_type++ = '\0';
		// Split main_type/sub_type by ','
		char *sub_type = strchr(main_type, ',');
		if (sub_type == NULL)
			goto invalid_fmt;
		*sub_type++ = '\0';

		// Stat the input file to get its size
		struct stat input_stat;
		if (stat(filename, &input_stat) != 0) {
			fprintf(stderr, "Cannot stat input file '%s", filename);
			perror("'");
			goto free_hdr;
		}

		bool is_verify = strcmp(main_type, "VERIFY") == 0;
		if (!is_verify)
			// Align start offset for non-'VERIFY' files
			offset = (((offset - 1) / hdr->image.align) + 1) * hdr->image.align;

		// Fill in the file header
		file->id		 = i;
		file->offset	 = 0;
		file->img_offset = offset;
		file->size		 = input_stat.st_size;
		// Convert known file types
		if (strcmp(file_type, "normal") == 0)
			file->file_type = FILE_NORMAL;
		else if (strcmp(file_type, "sparse") == 0)
			file->file_type = FILE_SPARSE;
		else if (strcmp(file_type, "ubi") == 0)
			file->file_type = FILE_UBI;
		else if (strcmp(file_type, "ubifs") == 0)
			file->file_type = FILE_UBIFS;
		else {
			fprintf(stderr, "Invalid file type: '%s'\n", file_type);
			goto free_hdr;
		}
		// Validate and copy main_type/sub_type
		if (strlen(main_type) >= sizeof(file->main_type)) {
			fprintf(stderr, "Main type too long: '%s'\n", main_type);
			goto free_hdr;
		}
		strcpy(file->main_type, main_type);
		if (strlen(sub_type) >= sizeof(file->sub_type)) {
			fprintf(stderr, "Sub type too long: '%s'\n", sub_type);
			goto free_hdr;
		}
		strcpy(file->sub_type, sub_type);

		// Find and mark the partition being verified (if any)
		for (uint32_t j = 0; is_verify && j < i; j++) {
			if (strcmp(hdr->files[j].sub_type, sub_type) == 0) {
				hdr->files[j].verify = true;
				break;
			}
		}

		// Go through existing files to reuse any duplicates
		bool found = false;
		for (uint32_t j = 0; j < i; j++) {
			if (filenames[j] != NULL && strcmp(filenames[j], filename) == 0) {
				// Reuse offset of previous item
				file->img_offset = hdr->files[j].img_offset;
				// Mark as backup image
				file->is_backup = true;
				file->backup_id = hdr->files[j].id;
				// Clear filename to skip writing
				filename = NULL;
				found	 = true;
				break;
			}
		}

		// Store input filename
		filenames[i] = filename;
		// Move file offset by size, if no duplicate found
		if (!found)
			offset += input_stat.st_size;

		continue;
	invalid_fmt:
		fprintf(stderr, "Invalid argument format: '%s'\n", argv[2 + i]);
		goto free_filenames;
	}

	// Fill in total size of the image
	hdr->image.size = offset;

	// Open the output file
	FILE *file_out = fopen(argv[1], "wb");
	if (file_out == NULL) {
		perror("Cannot open file for writing");
		goto free_filenames;
	}

	// Write the header
	if (fwrite(hdr, hdr_size, 1, file_out) != 1) {
		perror("Writing header failed");
		goto close_output;
	}

	// Allocate a read/write buffer
	uint8_t *buf = malloc(BUFFER_SIZE);
	if (buf == NULL) {
		perror("Cannot allocate read/write buffer");
		goto close_output;
	}

	// Calculate CRC32 of the entire header
	uint32_t crc = crc32((void *)&hdr->image.version, hdr_size - sizeof(uint32_t), 0xFFFFFFFF);

	// Copy each input file
	for (uint32_t i = 0; i < num_items; ++i) {
		const aml_file_t *file = &hdr->files[i];

		// Skip if the file is a backup (reused)
		if (filenames[i] == NULL) {
			fprintf(stderr, "Skipping '%s,%s' (duplicate/backup)\n", file->main_type, file->sub_type);
			continue;
		}

		// Open the input file
		fprintf(stderr, "Adding '%s,%s' from %s\n", file->main_type, file->sub_type, filenames[i]);
		FILE *file_in = fopen(filenames[i], "rb");
		if (file_in == NULL) {
			perror("Cannot open file for reading");
			goto free_buf;
		}

		// Check the current output file position
		uint64_t pos = ftell(file_out);
		assert(pos <= file->img_offset);
		// Calculate alignment/padding length
		uint32_t padding = file->img_offset - pos;
		// Skip the padding in output image
		if (fseek(file_out, (long)padding, SEEK_CUR) != 0) {
			perror("Cannot seek output file");
			fclose(file_in);
			goto free_buf;
		}
		// Calculate CRC32 of the padding
		assert(padding <= hdr->image.align);
		memset(buf, 0, padding);
		crc = crc32(buf, padding, crc);

		// Transfer data in chunks
		file_copy(buf, file->size, file_out, file_in, &crc);

		fclose(file_in);
	}

	// Rewrite the header's CRC
	if (fseek(file_out, 0, SEEK_SET) != 0) {
		perror("Cannot seek back to output image header");
		goto free_buf;
	}
	if (fwrite(&crc, sizeof(crc), 1, file_out) != 1) {
		perror("Cannot write updated CRC32");
		goto free_buf;
	}

	ret = EXIT_SUCCESS;

free_buf:
	free(buf);
close_output:
	fclose(file_out);
free_filenames:
	free(filenames);
free_hdr:
	free(hdr);
exit:
	return ret;
}
