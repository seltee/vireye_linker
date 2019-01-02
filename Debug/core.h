#pragma once

typedef char int8;
typedef short int16;
typedef int int32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#define VIDEO_MODE_SPRITE320x240_DOUBLE_SIZED 1

struct SpriteArray{
	const uint8 *sprites;
	uint8 *field;
};

//Debug functions
extern "C" __declspec(dllimport) void logs(const int8 *string);
extern "C" __declspec(dllimport) void logi(int32 val);

//Display functions
extern "C" __declspec(dllimport) void displayVideoMode(int8 mode);
extern "C" __declspec(dllimport) void displaySprite(const uint8 *data, int16 x, int16 y, uint8 width, uint8 height);
extern "C" __declspec(dllimport) void displaySpriteMask(const int8 *data, uint8 color, int16 x, int16 y, uint8 width, uint8 height);
extern "C" __declspec(dllimport) void displaySpriteArray(const SpriteArray *spriteArray, uint8 blockSize, int16 x, int16 y, uint8 width, uint8 height);
extern "C" __declspec(dllimport) void displaySync();

//Input functions
extern "C" __declspec(dllimport) uint8 inputState(int8 button);
