#pragma once
#include "stdafx.h"
#include "ElfReader.h"
#include "Headers.h"

class Saver
{
public:
	Saver();
	~Saver();

	bool save(const char *filename, ElfReader *reader);

private:
	SaveRelocation *createRelocation(Relocation *rel, ElfReader *reader, int source);
	Symbol *findSymbol(Relocation *rel, ElfReader *reader);

	unsigned char *symNameTable;
	int symNameSize;
};

