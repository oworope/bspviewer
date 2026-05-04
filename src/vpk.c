#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vpk.h>
#include <resource.h>

#ifdef _WIN32 // fix _strdup warning
#define strdup _strdup
#endif

const unsigned int VPKHeader_v2_Signature = 0x55aa1234;
const unsigned int VPKHeader_v2_Version = 2;

const unsigned short VPKDirectoryEntry_Terminator = 0xffff;

char* readString(FILE* f) {
	char buffer[1024];
	int i = 0;
	char c;

	while (fread(&c, 1, 1, f) && c != '\0') {
		if (i < 1023) {
			buffer[i++] = c;
		}
	}
	buffer[i] = '\0';

	if (i == 0 && feof(f)) return NULL;

	return strdup(buffer);
}

int readVPK(const char* filepath) {
	FILE* f = fopen(filepath, "rb");
	if (f == NULL) {
		fprintf(stderr, "Failed to open VPK file: %s\n", filepath);
		return -1;
	}

	VPKHeader_v2 header;
	fread(&header.Signature, 4, 1, f);
	fread(&header.Version, 4, 1, f);
	fread(&header.TreeSize, 4, 1, f);

	if (header.Version == 1) {
		fseek(f, 12, SEEK_SET);
	} else if (header.Version == 2) {
		fseek(f, 28, SEEK_SET);
	}

	if (header.Signature == VPKHeader_v2_Signature) {
		printf("%s: VPK Signature is correct. Version: %d\n", filepath, header.Version);
	}

	while (1) {
		char* ext = readString(f);
		if (ext == NULL || strlen(ext) == 0) break; // end of tree

		while (1) {
			char* path = readString(f);
			if (strlen(path) == 0) break;

			while (1) {
				char* name = readString(f);
				if (strlen(name) == 0) break; // end of files

				VPKDirectoryEntry entry;
				fread(&entry, sizeof(VPKDirectoryEntry), 1, f);

				if (entry.PreloadBytes > 0) {
					fseek(f, entry.PreloadBytes, SEEK_CUR);
				}

				char full_path[512];
				sprintf(full_path, "%s/%s.%s", path, name, ext);

				printf("%s: Found: %s\n", filepath, full_path);

				free(name);
			}
			free(path);
		}
		free(ext);
	}
	fclose(f);
	return 0;
}