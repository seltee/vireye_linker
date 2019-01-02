#include "ElfReader.h"
#include <string.h>

typedef struct
{
	unsigned char		e_ident[16];
	unsigned short      e_type;
	unsigned short      e_machine;
	unsigned int		e_version;
	unsigned int		e_entry;
	unsigned int		e_phoff;
	unsigned int		e_shoff;
	unsigned int		e_flags;
	unsigned short      e_ehsize;
	unsigned short      e_phentsize;
	unsigned short      e_phnum;
	unsigned short      e_shentsize;
	unsigned short      e_shnum;
	unsigned short      e_shstrndx;
} Elf32Header;

typedef struct
{
	unsigned int   sh_name;
	unsigned int   sh_type;
	unsigned int   sh_flags;
	unsigned int   sh_addr;
	unsigned int   sh_offset;
	unsigned int   sh_size;
	unsigned int   sh_link;
	unsigned int   sh_info;
	unsigned int   sh_addralign;
	unsigned int   sh_entsize;
} Elf32Section;

typedef struct{
	unsigned int    r_offset;
	unsigned short  r_sym_index;
	unsigned char	r1;
	unsigned char   r_type;
}Elf32Rel;

typedef struct {
	unsigned int	st_name;
	unsigned int	st_value;
	unsigned int	st_size;
	unsigned char	st_info;
	unsigned char	st_other;
	unsigned short	st_shndx;
} Elf32Sym;

ElfReader::ElfReader()
{
	code = new unsigned char[8*1024*1024];
	codeSize = 0;
	rodata = new unsigned char[8 * 1024 * 1024];
	rodataSize = 0;
	ram = new unsigned char[8 * 1024 * 1024];
	ramSize = 0;
	zeroRamSize = 0;
}


ElfReader::~ElfReader()
{
}

bool ElfReader::read(char *fileName) {
	printf("\nReading %s\n", fileName);
	FILE *file = fopen(fileName, "r");
	if (file) {
		char megaFile[64 * 1024];
		fread(megaFile, 64 * 1024, 1, file);
		fclose(file);

		//checking file
		Elf32Header header;
		memcpy(&header, megaFile, sizeof(Elf32Header));

		if (header.e_ident[0] != '\x7f' || header.e_ident[1] != 'E' || header.e_ident[2] != 'L' || header.e_ident[3] != 'F') {
			printf("It's not an Elf\n");
			return false;
		}

		printf("info %i %i %i %i %i %i %i\n", header.e_type, header.e_machine, header.e_phoff, header.e_shoff, header.e_phnum, header.e_shnum, header.e_shstrndx);
		int nameTable = header.e_shstrndx;

		if (header.e_type != 1) {
			printf("It's not an object file\n");
			return false;
		}

		if (header.e_machine != 40) {
			printf("It's not an arm thumb\n");
			return false;
		}

		if (header.e_phnum != 0) {
			printf("We don't know what to do with this\n");
			return false;
		}

		//initializing reader
		Elf32Section *sections = new Elf32Section[header.e_shnum];
		Elf32Section *namesSection = 0;

		Elf32Sym *syms = 0;
		unsigned int symsCount = 0;
		Elf32Rel *reallocCode = 0;
		unsigned int reallocCodeCount = 0;
		Elf32Rel *reallocRam = 0;
		unsigned int reallocRamCount = 0;

		unsigned int fileCodeSize = 0;
		unsigned char *fileCode = 0;
		unsigned int fileRodataSize = 0;
		unsigned char *fileRodata = 0;
		unsigned int fileRamSize = 0;
		unsigned char *fileRam = 0;
		unsigned int fileZeroRamSize = 0;

		unsigned char codeSectionNumber = 0;
		unsigned char rodataSectionNumber = 0;
		unsigned char zeroRamSectionNumber = 0;
		unsigned char initRamSectionNumber = 0;

		for (int i = 0; i < header.e_shnum; i++) {
			memcpy(&sections[i], &megaFile[header.e_shoff + i * sizeof(Elf32Section)], sizeof(Elf32Section));
			//printf("%i - %i %i %i %i\n", i, sections[i].sh_name, sections[i].sh_offset, sections[i].sh_type, sections[i].sh_offset);
			if (i == nameTable) {
				namesSection = &sections[i];
			}
		}

		//parsing file
		for (int i = 0; i < header.e_shnum; i++) {
			if (sections[i].sh_type != 0) {
				char buff[128];
				strcpy(buff, &megaFile[namesSection->sh_offset + sections[i].sh_name]);
				printf("parsing %i - %s (type %i, size %i, shift %i)\n", i, buff, sections[i].sh_type, sections[i].sh_size, sections[i].sh_offset);

				//program section
				if (sections[i].sh_type == 1 && strcmp(buff, ".text") == 0) {
					printf("- Program Code\n");
					fileCodeSize = sections[i].sh_size;
					fileCode = new unsigned char[fileCodeSize];
					memcpy(fileCode, &megaFile[sections[i].sh_offset], fileCodeSize);
					codeSectionNumber = i;
				}

				//rodata section
				if (sections[i].sh_type == 1 && buff[1] == 'r'  && buff[2] == 'o'  && buff[3] == 'd'  && buff[4] == 'a'  && buff[5] == 't'  && buff[6] == 'a') {
					printf("- Rodata\n");
					fileRodataSize = sections[i].sh_size;
					fileRodata = new unsigned char[fileRodataSize];
					memcpy(fileRodata, &megaFile[sections[i].sh_offset], fileRodataSize);
					rodataSectionNumber = i;
				}

				//initialized ram section
				if (sections[i].sh_type == 1 && strcmp(buff, ".data") == 0) {
					printf("- Initialized data %i\n", sections[i].sh_size);
					fileRamSize = sections[i].sh_size;
					fileRam = new unsigned char[fileRamSize];
					memcpy(fileRam, &megaFile[sections[i].sh_offset], fileRamSize);
					initRamSectionNumber = i;
				}

				//zero ram section
				if (sections[i].sh_type == 8 && strcmp(buff, ".bss") == 0) {
					printf("- Zero initialized data %i\n", sections[i].sh_size);
					fileZeroRamSize = sections[i].sh_size;
					zeroRamSectionNumber = i;
				}
				
				if (sections[i].sh_type == 9 && buff[0] == '.' && buff[1] == 'r' && buff[2] == 'e' && buff[3] == 'l') {
					int count = sections[i].sh_size / sections[i].sh_entsize;
					printf("- Relocation %i %i %i %i\n", sections[i].sh_entsize, sections[i].sh_info, sections[i].sh_link, count);
					char *forWhat = buff + 1;
					while (*forWhat != '.' && *forWhat != 0) forWhat++;
					if (strlen(forWhat)) {
						if (strcmp(forWhat, ".text") == 0) {
							printf("For text\n");
							reallocCodeCount = count;
							reallocCode = new Elf32Rel[reallocCodeCount];
							memcpy(reallocCode, &megaFile[sections[i].sh_offset], sizeof(Elf32Rel)*reallocCodeCount);
						}
						if (strcmp(forWhat, ".data") == 0) {
							printf("For data\n");
							reallocRamCount = count;
							reallocRam = new Elf32Rel[reallocRamCount];
							memcpy(reallocRam, &megaFile[sections[i].sh_offset], sizeof(Elf32Rel)*reallocRamCount);
						}
					}
				}

				if (sections[i].sh_type == 2 && strcmp(buff, ".symtab") == 0) {
					symsCount = sections[i].sh_size / sections[i].sh_entsize;
					printf("Sym table %i %i %i %i\n", sections[i].sh_entsize, sections[i].sh_info, sections[i].sh_link, symsCount);
					syms = new Elf32Sym[symsCount];
					memcpy(syms, &megaFile[sections[i].sh_offset], sizeof(Elf32Sym)*symsCount);
				}
			}
		}

		//summary
		printf("File summary:\n");

		printf("code (%i):\n", fileCodeSize);
		for (int i = 0; i < fileCodeSize / 2; i++) {
			printf("%04X ", ((unsigned short*)(fileCode))[i]);
		}

		printf("\nrom:\n");
		for (int c = 0; c < fileRodataSize; c++) {
			printf("%02x%c ", fileRodata[c], fileRodata[c]);
		}
		printf("\nram:\n");
		for (int c = 0; c < fileRamSize; c++) {
			printf("%02x ", fileRam[c]);
		}
		printf("\n");

		//saving symbols
		if (symsCount) {
			printf("Symbol parsing\n");
			for (int i = 0; i < symsCount; i++) {
				Symbol *newSym = new Symbol();
				newSym->name = syms[i].st_name ? strdup(&megaFile[namesSection->sh_offset + syms[i].st_name]) : 0;
				newSym->segmentCodeShift = codeSize;
				newSym->segmentRomShift = rodataSize;
				newSym->segmentRamShift = ramSize;
				newSym->segmentZeroRamShift = zeroRamSize;
				
				newSym->bind = syms[i].st_info >> 4;
				newSym->type = syms[i].st_info & 0x0f;
				newSym->size = syms[i].st_size;
				newSym->value = syms[i].st_value;
				newSym->sectionType = SECTION_TYPE_NONE;

				printf("symbol %i - %s %i %i %i %i, %i %i\n", i, newSym->name, newSym->bind, newSym->type, newSym->size, newSym->value, newSym->sectionType, syms[i].st_other);

				if (syms[i].st_shndx) {
					if (syms[i].st_shndx == codeSectionNumber)
						newSym->sectionType = SECTION_TYPE_CODE;

					if (syms[i].st_shndx == rodataSectionNumber)
						newSym->sectionType = SECTION_TYPE_ROM;

					if (syms[i].st_shndx == zeroRamSectionNumber)
						newSym->sectionType = SECTION_TYPE_ZERO_RAM;

					if (syms[i].st_shndx == initRamSectionNumber)
						newSym->sectionType = SECTION_TYPE_INITED_RAM;
				}

				if (newSym->type != 4) {
					symbols.push_back(newSym);

					if (reallocCodeCount) {
						for (int r = 0; r < reallocCodeCount; r++) {
							int index = (reallocCode[r].r_sym_index >> 8) + (reallocCode[r].r1 << 16);
							if (index == i) {
								printf("relocation code %i - %i, %i\n", r, reallocCode[r].r_offset, codeSize);
								Relocation *rel = new Relocation();
								rel->symbol = newSym;
								rel->offset = reallocCode[r].r_offset + codeSize;
								rel->relocationTarget = SECTION_TYPE_CODE;
								relocations.push_back(rel);
							}
						}
					}

					if (reallocRamCount) {
						for (int r = 0; r < reallocRamCount; r++) {
							int index = (reallocRam[r].r_sym_index >> 8) + (reallocRam[r].r1 << 16);
							if (index == i) {
								printf("relocation ram %s %i - %i, %i, %i\n", newSym->name, r, reallocRam[r].r_offset, codeSize, newSym->sectionType);
								Relocation *rel = new Relocation();
								rel->symbol = newSym;
								rel->offset = reallocRam[r].r_offset + codeSize;
								rel->relocationTarget = SECTION_TYPE_INITED_RAM;
								relocations.push_back(rel);
							}
						}
					}
				}
				else {
					delete newSym;
				}
			}
		}

		//coping new code
		if (fileCodeSize) {
			memcpy(code + codeSize, fileCode, fileCodeSize);
			codeSize += fileCodeSize;
			delete fileCode;
		}

		//coping new rom
		if (fileRodataSize) {
			memcpy(&rodata[rodataSize], fileRodata, fileRodataSize);
			rodataSize += fileRodataSize;
			delete fileRodata;
		}

		//coping new ram
		if (fileRamSize) {
			memcpy(&ram[ramSize], fileRam, fileRamSize);
			ramSize += fileRamSize;
			delete fileRam;
		}

		//setting up zero ram size
		zeroRamSize += fileZeroRamSize;

		return true;
	}
	return false;
}
