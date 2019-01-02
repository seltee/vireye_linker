#pragma once
#include "stdafx.h"
#include <vector>

enum {
	SECTION_TYPE_NONE = 0,
	SECTION_TYPE_CODE,
	SECTION_TYPE_ROM,
	SECTION_TYPE_INITED_RAM,
	SECTION_TYPE_ZERO_RAM
};

struct Symbol {
	char *name;
	unsigned int segmentCodeShift;
	unsigned int segmentRomShift;
	unsigned int segmentRamShift;
	unsigned int segmentZeroRamShift;
	char bind;
	char type;
	char sectionType;
	char q;
	unsigned int size;
	unsigned int value;
};

struct Relocation {
	Symbol *symbol;
	unsigned int offset;
	unsigned int relocationTarget;
};

class ElfReader
{
public:
	ElfReader();
	~ElfReader();

	std::vector<Relocation *> relocations;
	std::vector<Symbol *> symbols;

	bool read(char *fileName);

	unsigned char *code;
	unsigned int codeSize;
	unsigned char *rodata;
	unsigned int rodataSize;
	unsigned char *ram;
	unsigned int ramSize;
	unsigned int zeroRamSize;
};

