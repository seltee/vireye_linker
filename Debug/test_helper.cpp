#include "./test_helper.h"

void summ16(int8 *field){
	logi(4000);
	field[0] = 1;
	field[1] = 2;
	field[2] = 3;
}

void driveMass(int8 *field, int8 size){
	logi(5000);
	for (int i = 0; i < size; i++){
		if (field[i]){
			logs("BINGO BONGO");
			logi(i);
		}
	}
}

void randomBuilder(uint8 *field, int32 num){
	logs("RANDOM BUILDER");
	logi((int)field);
	int c = num%4;
	
	
	for (int i = 0; i < 9; i++){
		field[i] = 0;
	}

	switch(c){
		case 0:
			field[3] = 1;
			field[4] = 1;
			field[5] = 1;
			break;
			
		case 1:
			field[0] = 1;
			field[4] = 1;
			field[8] = 1;
			break;
			
		case 2:
			field[1] = 1;
			field[4] = 1;
			field[7] = 1;
			break;
			
		case 3:
			field[2] = 1;
			field[4] = 1;
			field[6] = 1;
			break;
	}
}
