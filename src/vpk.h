#pragma once
// https://developer.valvesoftware.com/wiki/VPK_(file_format)

const unsigned int VPKHeader_v2_Signature = 0x55aa1234;
const unsigned int VPKHeader_v2_Version = 2;

typedef struct VPKHeader_v2
{
	const unsigned int Signature;
	const unsigned int Version;

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