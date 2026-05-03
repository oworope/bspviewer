#pragma once
// https://developer.valvesoftware.com/wiki/VPK_(file_format)

typedef struct VPKHeader_v2
{
	unsigned int Signature;
	unsigned int Version;

	// The size, in bytes, of the directory tree
	unsigned int TreeSize;

	// How many bytes of file content are stored in this VPK file (0 in CSGO)
	unsigned int FileDataSectionSize;

	// The size, in bytes, of the section containing MD5 checksums for external archive content
	unsigned int ArchiveMD5SectionSize;

	// The size, in bytes, of the section containing MD5 checksums for content in this file (should always be 48)
	unsigned int OtherMD5SectionSize;

	// The size, in bytes, of the section containing the public key and signature. This is either 0 (CSGO & The Ship) or 296 (HL2, HL2:DM, HL2:EP1, HL2:EP2, HL2:LC, TF2, DOD:S & CS:S)
	unsigned int SignatureSectionSize;
} VPKHeader_v2;

#pragma pack(push, 1)
typedef struct VPKDirectoryEntry
{
	unsigned int CRC; // A 32bit CRC of the file's data.
	unsigned short PreloadBytes; // The number of bytes contained in the index file.

	// A zero based index of the archive this file's data is contained in.
	// If 0x7fff, the data follows the directory.
	unsigned short ArchiveIndex;

	// If ArchiveIndex is 0x7fff, the offset of the file data relative to the end of the directory (see the header for more details).
	// Otherwise, the offset of the data from the start of the specified archive.
	unsigned int EntryOffset;

	// If zero, the entire file is stored in the preload data.
	// Otherwise, the number of bytes stored starting at EntryOffset.
	unsigned int EntryLength;

	const unsigned short Terminator;
} VPKDirectoryEntry;
#pragma pack(pop)

int readVPK(const char* filepath);