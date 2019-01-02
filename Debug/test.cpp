#include "./core.h"
#include "./test_helper.h"
#include "./sprites.h"

SpriteArray view;

int32 main(){
	displayVideoMode(VIDEO_MODE_SPRITE320x240_DOUBLE_SIZED);
	
	uint8 field[3*3];
	
	view.sprites = sprites2;
	view.field = field;
	
	int summer = 1;
	int position = 0;

	//while(1){
		logi((int)field);
		randomBuilder(field, position);
		position+=summer;

		if (position > 320 - 36){
			summer = -1;
		}
		if (position < 0){
			summer = 1;
		}
		
		displaySpriteArray(&view, 6, position, 0, 3, 3);
		displaySync();
	//}
		
	return 0;
}
