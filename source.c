#include "raylib.h"

int main()
{
	int WINDOW_WIDTH = 1260;
	int WINDOW_HEIGHT = 720;
	struct Color temp;
	temp.r = 255;
	temp.g = 0;
	temp.b = 255;
	temp.a = 255;

	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "DOOM CLONE");
	SetTargetFPS(300);
	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(temp);
		EndDrawing();
	}
	return 0;
}
