#include "Saver.h"
#include <string.h>

#define VERSION 1
#define SUB_VERSION 0
#define CODE_BLOCK_SIZE (4 * 1024)

std::string coreFunctions[] = {
	"logs",
	"logi",
	"displayVideoMode",
	"displaySprite",
	"displaySpriteMask",
	"displaySpriteArray",
	"displaySync",
	"inputState",
	"memset"
};
const int coreFunctionCount = sizeof(coreFunctions) / sizeof(std::string);

Saver::Saver()
{
}

Saver::~Saver()
{
}

Symbol *Saver::findSymbol(Relocation *rel, ElfReader *reader) {
	Symbol *foundedSym = 0;
	if (rel->symbol->name) {
		for (int c = 0; c < reader->symbols.size(); c++) {
			if (reader->symbols.at(c)->name && strcmp(reader->symbols.at(c)->name, rel->symbol->name) == 0 && reader->symbols.at(c)->size) {
				foundedSym = reader->symbols.at(c);
			}
		}
	}
	if (!foundedSym && rel->symbol->bind == 1) {
		for (int c = 0; c < coreFunctionCount; c++) {
			if (strcmp(coreFunctions[c].c_str(), rel->symbol->name) == 0) {
				foundedSym = rel->symbol;
			}
		}
	}
	return foundedSym;
}

SaveRelocation *Saver::createRelocation(Relocation *rel, ElfReader *reader, int source) {
	Symbol *foundedSym = rel->symbol->type == 0 ? findSymbol(rel, reader) : rel->symbol;

	SaveRelocation *saveRelocation = 0;
	if (foundedSym) {
		printf("RELOCATION %i %s\n", foundedSym->sectionType, foundedSym->name);
		saveRelocation = new SaveRelocation();
		saveRelocation->shift = rel->offset;
		saveRelocation->bind = rel->symbol->bind;
		saveRelocation->source = source;

		if (foundedSym->sectionType == SECTION_TYPE_NONE) {
			saveRelocation->type = SAVE_SECTION_TYPE_LIB;
			saveRelocation->nameShift = symNameSize;
			char buff[256];
			sprintf(buff, "core.%s", foundedSym->name);
			memcpy(symNameTable + symNameSize, buff, strlen(buff) + 1);
			symNameSize += strlen(buff) + 1;
		}

		if (foundedSym->sectionType == SECTION_TYPE_CODE) {
			printf("CODE CODE CODE %i %i %i %i\n", foundedSym->name, foundedSym->value, foundedSym->segmentCodeShift, (foundedSym->value & 0xfffffffe) + foundedSym->segmentCodeShift);
			saveRelocation->type = SAVE_SECTION_TYPE_CODE;
			saveRelocation->targetShift = (foundedSym->value & 0xfffffffe) + foundedSym->segmentCodeShift;
			saveRelocation->nameShift = 0;
		}

		if (foundedSym->sectionType == SECTION_TYPE_ROM) {
			printf("ROM ROM ROM\n");
			saveRelocation->type = SAVE_SECTION_TYPE_ROM;
			saveRelocation->targetShift = foundedSym->value + foundedSym->segmentRomShift;
			saveRelocation->nameShift = 0;
		}

		if (foundedSym->sectionType == SECTION_TYPE_INITED_RAM) {
			saveRelocation->type = SAVE_SECTION_TYPE_RAM;
			saveRelocation->targetShift = foundedSym->value + foundedSym->segmentRamShift;
			saveRelocation->nameShift = 0;
		}

		if (foundedSym->sectionType == SECTION_TYPE_ZERO_RAM) {
			saveRelocation->type = SAVE_SECTION_TYPE_RAM;
			saveRelocation->targetShift = reader->ramSize + foundedSym->value + foundedSym->segmentZeroRamShift;
			saveRelocation->nameShift = 0;
		}
	}

	return saveRelocation;
}

bool Saver::save(const char *filename, ElfReader *reader) {
	FILE *f = fopen(filename, "wb");

	SaveMainHeader saveMainHeader;
	SaveUsualHeader saveUsualHeader;
	SaveCodePartHeader saveCodePartHeader;
	
	int entry = -1;
	for (int i = 0; i < reader->symbols.size(); i++) {
		if (reader->symbols.at(i)->name && strcmp(reader->symbols.at(i)->name, "main") == 0) {
			entry = reader->symbols.at(i)->segmentCodeShift + reader->symbols.at(i)->value & 0xfffffffe;
		}
	}
	if (!entry == -1) {
		printf("Entry not found\n");
		fclose(f);
		return false;
	}

	//file header
	memcpy(saveMainHeader.mark, "WUMC", 4);
	saveMainHeader.version = VERSION;
	saveMainHeader.subVersion = SUB_VERSION;
	saveMainHeader.architecture = ARHITECTURE_THUMB;
	saveMainHeader.maxCodeBlockSize = CODE_BLOCK_SIZE;
	saveMainHeader.ramSize = reader->ramSize + reader->zeroRamSize;
	saveMainHeader.romSize = reader->rodataSize;
	saveMainHeader.codeSize = reader->codeSize;
	saveMainHeader.entry = entry;
	fwrite(&saveMainHeader, sizeof(SaveMainHeader), 1, f);

	unsigned int numOfBlocks = (reader->codeSize / CODE_BLOCK_SIZE) + 1;

	saveUsualHeader.type = SAVE_BLOCK_TYPE_CODE_PART;
	saveUsualHeader.headerSize = sizeof(SaveCodePartHeader);
	saveUsualHeader.size = numOfBlocks;
	saveUsualHeader.version = 0;
	fwrite(&saveUsualHeader, sizeof(SaveUsualHeader), 1, f);

	symNameTable = new unsigned char[4 * 1024];
	int c;
	for (int block = 0; block < numOfBlocks; block++) {
		int blockSize = (block == numOfBlocks - 1) ? reader->codeSize - block * CODE_BLOCK_SIZE : CODE_BLOCK_SIZE;
		int globalShift = block*CODE_BLOCK_SIZE;
		symNameSize = 0;

		std::vector<SaveRelocation*> saveRelocations;

		for (int i = 0; i < reader->relocations.size(); i++) {
			Relocation *rel = reader->relocations.at(i);
			Symbol *foundedSym;
			if (rel->offset >= globalShift && rel->offset < globalShift + blockSize && rel->relocationTarget == SECTION_TYPE_CODE) {
				printf("Rel %s %i %i %i %i\n", rel->symbol->name, rel->offset, rel->symbol->bind, rel->symbol->type, rel->symbol->sectionType);

				SaveRelocation *saveRel = createRelocation(rel, reader, SAVE_SOURCE_CODE);
				if (saveRel) {
					saveRelocations.push_back(saveRel);
				}
				else {
					printf("Relocation error, unknown symbol %s\n", rel->symbol->name);
					fclose(f);
					return false;
				}
			}			
		}

		//printf("final of block %i\n\n", block);
		//for (int i = 0; i < saveRelocations.size(); i++) {
		//	SaveRelocation *saveRelocation = saveRelocations.at(i);
		//	printf("%i - %s %i %i %i\n", i, saveRelocation->type == SAVE_SECTION_TYPE_LIB ? (const char *)(symNameTable + saveRelocation->nameShift) : "NoName", saveRelocation->type, saveRelocation->shift, saveRelocation->targetShift);
		//}

		//saving header
		saveCodePartHeader.codeLength = blockSize;
		saveCodePartHeader.globalShift = globalShift;
		saveCodePartHeader.symNameTableLength = symNameSize;
		saveCodePartHeader.relocationsCount = saveRelocations.size();
		fwrite(&saveCodePartHeader, sizeof(SaveCodePartHeader), 1, f);

		//saving code
		fwrite(reader->code + globalShift, blockSize, 1, f);

		//saving symtable
		fwrite(symNameTable, symNameSize, 1, f);

		//saving relocations
		//fwrite(saveRelocations.data(), sizeof(SaveRelocation), saveRelocations.size(), f);
		for (int i = 0; i < saveRelocations.size(); i++) {
			fwrite(saveRelocations.at(i), 1, sizeof(SaveRelocation), f);
		}
	}
	delete symNameTable;
	symNameTable = 0;

	//saving rodata
	saveUsualHeader.type = SAVE_BLOCK_TYPE_RODATA;
	saveUsualHeader.version = 0;
	saveUsualHeader.headerSize = 0;
	saveUsualHeader.size = reader->rodataSize;
	fwrite(&saveUsualHeader, sizeof(SaveUsualHeader), 1, f);

	fwrite(reader->rodata, reader->rodataSize, 1, f);

	//saving ram
	saveUsualHeader.type = SAVE_BLOCK_TYPE_RAM;
	saveUsualHeader.version = 0;
	saveUsualHeader.headerSize = 0;
	saveUsualHeader.size = reader->ramSize;
	fwrite(&saveUsualHeader, sizeof(SaveUsualHeader), 1, f);

	fwrite(reader->ram, reader->ramSize, 1, f);

	//saving ram relocation
	std::vector<SaveRelocation*> saveRamRelocations;
	for (int i = 0; i < reader->relocations.size(); i++) {
		Relocation *rel = reader->relocations.at(i);
		if (rel->relocationTarget == SECTION_TYPE_INITED_RAM) {
			printf("Create relocation\n");
			SaveRelocation *saveRel = createRelocation(rel, reader, SAVE_SOURCE_RAM);
			printf("Done\n");
			if (saveRel) {
				saveRamRelocations.push_back(saveRel);
			}
			else {
				printf("Relocation error, unknown symbol %s\n", rel->symbol->name);
			}
		}
	}

	if (saveRamRelocations.size() > 0) {
		printf("SAVING RAM RELOCATIONS %i\n", saveRamRelocations.size());
		saveUsualHeader.type = SAVE_BLOCK_TYPE_RAM_RELOCATION;
		saveUsualHeader.version = 0;
		saveUsualHeader.headerSize = 0;
		saveUsualHeader.size = saveRamRelocations.size();
		fwrite(&saveUsualHeader, sizeof(SaveUsualHeader), 1, f);

		for (int i = 0; i < saveRamRelocations.size(); i++) {
			fwrite(saveRamRelocations.at(i), 1, sizeof(SaveRelocation), f);
		}
	}


	//saveUsualHeader.type = SAVE_BLOCK_TYPE_RAM;

	saveUsualHeader.type = SAVE_BLOCK_TYPE_END;
	saveUsualHeader.version = 0;
	saveUsualHeader.headerSize = 0;
	saveUsualHeader.size = 0;
	fwrite(&saveUsualHeader, sizeof(SaveUsualHeader), 1, f);

	fclose(f);
	return true;
}