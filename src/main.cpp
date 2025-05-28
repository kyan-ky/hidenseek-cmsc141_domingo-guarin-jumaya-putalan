#include "raylib.h"
#include "raymath.h"
#include "resource_dir.h"
#include "player.h"

int main ()
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	SetTargetFPS(60);
	InitWindow(1280, 720, "Automa-tag: Ryan's Revenge");
	SearchAndSetResourceDir("resources");

	int currScreen = 0;
	bool playerInit = false;
	Rectangle button = { 350, 280, 100, 40 };

	while (!WindowShouldClose())
    {
        if (currScreen == 0)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Vector2 mousePos = GetMousePosition();
                if (CheckCollisionPointRec(mousePos, button))
                {
                    currScreen  = 1;
                    if (!playerInit)
                    {
                        Player_Init();
                        playerInit = true;
                    }
                }
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Main Menu", 600, 200, 40, DARKGRAY);
            DrawRectangleRec(button, LIGHTGRAY);
            DrawText("Start Game", button.x + 40, button.y + 15, 20, BLACK);
            DrawFPS(10, 10);
            EndDrawing();
        }
        else if (currScreen  == 1)
        {
            Player_UpdateDraw();
            if (IsKeyPressed(KEY_ESCAPE))
            {
                currScreen  = 0;
                if (playerInit)
                {
                    Player_Unload();
                    playerInit = false;
                }
            }
        }
    }
	CloseWindow();
	return 0;
}