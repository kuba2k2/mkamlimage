// Copyright (c) Kuba Szczodrzyński 2026-3-3.

#include "amlimage.h"

#include "utils.c"

static const char *file_type_str(uint32_t file_type) {
	switch (file_type) {
		case FILE_NORMAL:
			return "normal";
		case FILE_SPARSE:
			return "sparse";
		case FILE_UBI:
			return "ubi";
		case FILE_UBIFS:
			return "ubifs";
		default:
			return "(unknown)";
	}
}

static void convert_file_v1_v2(aml_file_t *v2, aml_file_v1_t *v1) {
	v2->id		   = v1->id;
	v2->file_type  = v1->file_type;
	v2->offset	   = v1->offset;
	v2->img_offset = v1->img_offset;
	v2->size	   = v1->size;
	memcpy(v2->main_type, v1->main_type, sizeof(v1->main_type));
	memcpy(v2->sub_type, v1->sub_type, sizeof(v1->sub_type));
	v2->verify	  = v1->verify;
	v2->is_backup = v1->is_backup;
	v2->backup_id = v1->backup_id;
}

int main(int argc, char *argv[]) {
	int ret = EXIT_FAILURE;

	// Process optional command-line switches
	bool list = false;
	while (argc > 1 && argv[1][0] == '-') {
		if (strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "-l") == 0) {
			argv += 1;
			argc -= 1;
			list = true;
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[1]);
			return ret;
		}
	}

	// Check required argument count
	if (argc < 2) {
		fprintf(
			stderr,
			"Usage: %s [OPTION] INPUT_FILE\n"
			"\n"
			"Options:\n"
			"  --list, -l    List files, do not extract",
			argv[0]
		);
		return ret;
	}

	// Open the input file
	FILE *file_in = fopen(argv[1], "rb");
	if (file_in == NULL) {
		perror("Cannot open file for reading");
		goto exit;
	}

	// Read the image header
	aml_image_t *image = malloc(sizeof(*image));
	if (image == NULL) {
		perror("Cannot allocate image header");
		goto close_input;
	}
	if (fread(image, sizeof(*image), 1, file_in) != 1) {
		perror("Reading image header failed");
		goto free_image;
	}

	// Validate image magic
	if (image->magic != IMAGE_MAGIC) {
		fprintf(stderr, "Invalid header magic: expected 0x%08X, found 0x%08X\n", IMAGE_MAGIC, image->magic);
		goto free_image;
	}

	// Check image version
	uint32_t version = image->version;
	if (version < 1 || version > 2) {
		fprintf(stderr, "Invalid header version: expected {1,2}, found %u\n", version);
		goto free_image;
	}

	uint32_t num_items = image->num_items;
	fprintf(stderr, "Found version %u image with %u file(s)\n", version, num_items);

	// Read the file headers
	uint32_t files_size = (version == 1 ? sizeof(aml_file_v1_t) : sizeof(aml_file_t)) * num_items;
	void *files			= malloc(files_size);
	if (files == NULL) {
		perror("Cannot allocate file headers");
		goto free_image;
	}
	if (fread(files, files_size, 1, file_in) != 1) {
		perror("Reading file headers failed");
		goto free_files;
	}

	if (list) {
		for (uint32_t i = 0; i < num_items; ++i) {
			aml_file_t file = {0};
			if (version == 1)
				convert_file_v1_v2(&file, (aml_file_v1_t *)files + i);
			else
				memcpy(&file, (aml_file_t *)files + i, sizeof(file));

			const char *file_type = file_type_str(file.file_type);
			printf(
				"File '%s,%s,%s' - offset: 0x%08llx, size: %llu, verify: %s\n",
				file_type,
				file.main_type,
				file.sub_type,
				file.img_offset,
				file.size,
				file.verify ? "yes" : "no"
			);
		}
		goto free_files;
	}

	// Allocate a read/write buffer
	uint8_t *buf = malloc(BUFFER_SIZE);
	if (buf == NULL) {
		perror("Cannot allocate read/write buffer");
		goto free_files;
	}

	// Copy each input file
	char filename[4096];
	for (uint32_t i = 0; i < num_items; ++i) {
		aml_file_t file = {0};
		if (version == 1)
			convert_file_v1_v2(&file, (aml_file_v1_t *)files + i);
		else
			memcpy(&file, (aml_file_t *)files + i, sizeof(file));

		const char *file_type = file_type_str(file.file_type);
		snprintf(filename, sizeof(filename) - 1, "%s.%s.%s.%s.img", argv[1], file_type, file.main_type, file.sub_type);

		// Open the output file
		fprintf(stderr, "Extracting '%s,%s' to %s\n", file.main_type, file.sub_type, filename);
		FILE *file_out = fopen(filename, "wb");
		if (file_out == NULL) {
			perror("Cannot open file for writing");
			goto free_buf;
		}

		// Seek to the offset in image
		if (fseek(file_in, (long)file.img_offset, SEEK_SET) != 0) {
			perror("Cannot seek input file");
			fclose(file_out);
			goto free_buf;
		}

		// Transfer data in chunks
		file_copy(buf, file.size, file_out, file_in, NULL);

		fclose(file_out);
	}

	ret = EXIT_SUCCESS;

free_buf:
	free(buf);
free_files:
	free(files);
free_image:
	free(image);
close_input:
	fclose(file_in);
exit:
	return ret;
}
