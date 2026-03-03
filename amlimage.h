// Copyright (c) Kuba Szczodrzyński 2026-3-3.

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2 * !!(condition)]))
#define IMAGE_MAGIC				0x27B51956
#define FILE_NORMAL				0x000
#define FILE_SPARSE				0x0FE
#define FILE_UBI				0x1FE
#define FILE_UBIFS				0x2FE

typedef struct __attribute__((packed)) aml_image_t {
	uint32_t crc;
	uint32_t version;
	uint32_t magic;
	uint64_t size;
	uint32_t align;
	uint32_t num_items;
	uint8_t reserved[36];
} aml_image_t;

typedef struct aml_file_v1_t {
	uint32_t id;
	uint32_t file_type;
	uint64_t offset;
	uint64_t img_offset;
	uint64_t size;
	char main_type[32];
	char sub_type[32];
	uint32_t verify;
	uint16_t is_backup;
	uint16_t backup_id;
	uint8_t reserved[24];
} aml_file_v1_t;

typedef struct aml_file_t {
	uint32_t id;
	uint32_t file_type;
	uint64_t offset;
	uint64_t img_offset;
	uint64_t size;
	char main_type[256];
	char sub_type[256];
	uint32_t verify;
	uint16_t is_backup;
	uint16_t backup_id;
	uint8_t reserved[24];
} aml_file_t;

typedef struct aml_hdr_t {
	aml_image_t image;
	aml_file_t files[];
} aml_hdr_t;

void build_bug_on() {
	BUILD_BUG_ON(sizeof(aml_image_t) != 64);
	BUILD_BUG_ON(sizeof(aml_file_v1_t) != 128);
	BUILD_BUG_ON(sizeof(aml_file_t) != 576);
	BUILD_BUG_ON(sizeof(aml_hdr_t) != 64);
}
