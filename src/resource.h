#pragma once
#include <stdint.h>

typedef struct TextureFileResource {
	char* full_path; // e.g. "materials/metal/metalwall001.vtf"
	uint16_t archive_index;
	uint32_t entry_offset;
	uint32_t entry_length;
} TextureFileResource;

TextureFileResource* g_texture_registry = NULL;
int g_texture_count = 0;