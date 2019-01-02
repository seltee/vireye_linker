// Linker.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "ElfReader.h"
#include "Saver.h"
#include <cstdlib>
#include <string>

int main(int argc, char *argv[])
{
	ElfReader reader;
	Saver saver;
	bool error = false;

	const char *name = 0;

	for (int i = 1; i < argc; i++) {
		printf("%i - %s\n", i, argv[i]);
	}

	for (int i = 1; i < argc; i++) {
		int length = strlen(argv[i]);
		if (length > 2 && argv[i][length - 1] == 'o' && argv[i][length - 2] == '.') {
			if (!reader.read(argv[i]) || !reader.code) {
				printf("Data is corrupted\n");
				error = true;
			}
		}
		else {
			if (length > 4 && argv[i][length - 1] == 'e' && argv[i][length - 2] == 'c' && argv[i][length - 3] == 's' && argv[i][length - 4] == '.') {
				if (!name) {
					name = argv[i];
				}else {
					printf("Output file can be only one\n");
					error = true;
				}
				
			}
			else {
				printf("Unknown parameter %s\n", argv[i]);
				error = true;
			}
		}
	}

	if (!error) {
		if (!name) {
			name = "out.sce";
		}

		printf("Program summary:\n");
		printf("Code size: %i\n", reader.codeSize);
		printf("Rom size: %i\n", reader.rodataSize);
		printf("Ram size: %i\n", reader.ramSize + reader.zeroRamSize);
		printf("\n");

		bool result = saver.save(name, &reader);
		if (result) {
			printf("Saved as %s\n", name);
		}
		else {
			printf("Unable to save due to errors\n");
		}
	}
	else {
		printf("Errors are found, can't link\n");
	}
		
	system("pause");
    return 0;
}

