#include "player.h"
#include "raylib.h"
#include "raymath.h"
#include "resource_dir.h"

static RenderTexture2D charaBuffer;
static Texture charaD, charaU, charaL, charaR;
static Texture charaDrun, charaUrun, charaLrun, charaRrun;
static Vector2 charaPos;
static float charaSpeed = 200.0f;
static float charaScale = 4.0f;
static Texture currentAnim;
static int frameCount = 6;
static int currentFrame = 0;
static float frameTime = 0.0f;
static float frameDuration = 0.1f;


void Player_Init(void) {
    SearchAndSetResourceDir("resources");

    charaBuffer = LoadRenderTexture(64, 64);
	SetTextureFilter(charaBuffer.texture, TEXTURE_FILTER_POINT);
    // Idle animation
    charaD = LoadTexture("char_idle_down_anim_strip_6.png");
	charaU = LoadTexture("char_idle_up_anim_strip_6.png");
	charaL = LoadTexture("char_idle_left_anim_strip_6.png");
	charaR = LoadTexture("char_idle_right_anim_strip_6.png");
    // Run animation
	charaDrun = LoadTexture("char_run_down_anim_strip_6.png");
	charaUrun = LoadTexture("char_run_up_anim_strip_6.png");
	charaLrun = LoadTexture("char_run_left_anim_strip_6.png");
	charaRrun = LoadTexture("char_run_right_anim_strip_6.png");
	charaPos = {200,200};
	charaSpeed = 200.0f;
	charaScale = 4.0f;
	currentAnim = charaD;
}


void Player_UpdateDraw(void) {
		Vector2 motion = {0, 0};
		if (IsKeyDown(KEY_A))
		{
			motion.x += -1;
		}
		if (IsKeyDown(KEY_D))
		{
			motion.x += 1;
		}
		if (IsKeyDown(KEY_W))
		{
			motion.y += -1;
		}
		if (IsKeyDown(KEY_S))
		{
			motion.y += 1;
		}
		// Decide if running or idle
		bool isMoving = (Vector2Length(motion) > 0);

		// Normalize motion vector for consistent speed
		if (isMoving) {
			motion = Vector2Normalize(motion);
			Vector2 movementThisFrame = Vector2Scale(motion, GetFrameTime() * charaSpeed);
			charaPos = Vector2Add(charaPos, movementThisFrame);
		}

		// Update current animation texture depending on direction and moving state
		if (motion.x < 0) {
			currentAnim = isMoving ? charaLrun : charaL;
		} else if (motion.x > 0) {
			currentAnim = isMoving ? charaRrun : charaR;
		} else if (motion.y < 0) {
			currentAnim = isMoving ? charaUrun : charaU;
		} else if (motion.y > 0) {
			currentAnim = isMoving ? charaDrun : charaD;
		} else {
			// No input, stay with currentAnim (or default to down idle)
			if (!isMoving) currentAnim = charaD;
		}

		// Animate frames if moving, else reset to first frame
		if (isMoving) {
			frameTime += GetFrameTime();
			if (frameTime >= frameDuration) {
				frameTime = 0.0f;
				currentFrame++;
				if (currentFrame >= frameCount) currentFrame = 0;
			}
		} else {
			currentFrame = 0;
		}

		// Draw current frame from currentAnim into charaBuffer
		int frameWidth = currentAnim.width / frameCount;
		int frameHeight = currentAnim.height;

		Rectangle sourceRec = { currentFrame * frameWidth, 0, frameWidth, frameHeight };
		BeginTextureMode(charaBuffer);
			ClearBackground(BLANK);
			DrawTextureRec(currentAnim, sourceRec, (Vector2){0,0}, WHITE);
		EndTextureMode();

		BeginDrawing();
		ClearBackground(BLACK);
		DrawText("Automa-tag: Ryan's Revenge", 100, 100, 20,WHITE);

		Vector2 origin = { 0, 0 };
		Rectangle source = { 0, 0, (float)charaBuffer.texture.width, -(float)charaBuffer.texture.height };
		Rectangle dest = {charaPos.x, charaPos.y, source.width * charaScale, -source.height * charaScale};
		DrawTexturePro(charaBuffer.texture, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
	
		DrawFPS(10, 10);
		EndDrawing();
}

void Player_Unload(void) {
	UnloadTexture(charaD);
	UnloadTexture(charaU);
	UnloadTexture(charaL);
	UnloadTexture(charaR);
	UnloadTexture(charaDrun);
	UnloadTexture(charaUrun);
	UnloadTexture(charaLrun);
	UnloadTexture(charaRrun);
	UnloadRenderTexture(charaBuffer);
}