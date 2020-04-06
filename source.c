#include "raylib.h"
#include "raymath.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#define WIN_H 720
#define WIN_W 1280
#define TFPS 300 

#define RESPAWN_WAIT 1500
#define ATTACK_WAIT 210

// ===========
// Player Data
// ===========
typedef struct player
{
	Camera camera;
	BoundingBox hitbox;
	float health;
	Ray lgray; // laser gun ray
	bool islasing;

	Music mainloop, endloop; // shouldn't *really* be here but not creating a game class now...
} player;
player doomguy = { 0 };
// ===============
// End Player Data
// ===============

// =======
// Bad Guy
// =======
typedef struct daemon
{
	float xpos, zpos; 
	BoundingBox hitbox;
	float health, previoushealth;
	bool dead;
	int deathcounter; 
	bool awaitingrespawn;

	bool resting;
	int attackcounter;
	int damage;

	Texture2D sprite;
} daemon;
daemon enemy = { 0 };
void respawn( void );

// ======================
// Update/ Init Functions
// ======================
static void DrawFrame( void );
static void UpdateGame( void );
static void InitGame( void );
static void UnloadGame( void );
// ==========================
// End Update/ Init Functions
// ==========================

// globals
int roundnum;
int respawncounter;
bool endgame;
bool playingmusic;

int main(void)
{
	InitWindow(WIN_W, WIN_H, "DOOM: CLONE");
	InitAudioDevice();
	InitGame();

	while (!WindowShouldClose())
	{
		UpdateGame();
		DrawFrame();
	}

	UnloadGame();
	CloseAudioDevice();
	CloseWindow();

	return 0;
}

static void InitGame()
{
	srand(time(0));
	endgame = false;

	doomguy.camera.position = (Vector3){ -16.5f, 2.0f, 10.f };
	doomguy.camera.target = (Vector3){ 0.0f, 1.8f, 0.0f };
	doomguy.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	doomguy.camera.fovy = 100.f;
	doomguy.camera.type = CAMERA_PERSPECTIVE;
	doomguy.hitbox.min = (Vector3){ doomguy.camera.position.x - 1.f, 2.f, doomguy.camera.position.z-2.f };
	doomguy.hitbox.max = (Vector3){ doomguy.camera.position.x + 2.f, 0.f, doomguy.camera.position.z+2.f };
	doomguy.health = 100;
	doomguy.mainloop = LoadMusicStream("D:/Programming Projects/doomclone/resources/DOOMDOOM.mp3");
	doomguy.endloop = LoadMusicStream("D:/Programming Projects/doomclone/resources/Doom OST - E1M3 - Dark Halls.mp3");

	enemy.xpos = 10;
	enemy.zpos = 10;
	enemy.hitbox.max.x = enemy.xpos + 3;
	enemy.hitbox.max.y = 4;
	enemy.hitbox.max.z = enemy.zpos + 3;
	enemy.hitbox.min.x = enemy.xpos - 3;
	enemy.hitbox.min.y = 0;
	enemy.hitbox.min.z = enemy.zpos - 3;
	enemy.sprite = LoadTexture("D:/Programming Projects/doomclone/resources/monsters.png");
	enemy.health = 50;
	enemy.previoushealth = 50;
	enemy.dead = false;
	enemy.attackcounter = ATTACK_WAIT; // 0.7 seconds
	enemy.resting = false;
	enemy.damage = 25;

	roundnum = 1;
	respawncounter = RESPAWN_WAIT;

	SetCameraMode(doomguy.camera, CAMERA_FIRST_PERSON);
	SetTargetFPS(TFPS);
}
static void UnloadGame()
{
	UnloadTexture(enemy.sprite);
	UnloadMusicStream(doomguy.mainloop);
}
static void UpdateGame()
{
	BoundingBox bluewall, limewall, goldwall, orangewall;
	bluewall.min = (Vector3){ -20.5, 0, -16 };
	bluewall.max = (Vector3){ -16.5, 5, 16 };
	limewall.min = (Vector3){ 16.5, 0, -16 };
	limewall.max = (Vector3){ 17.5, 5, 16 };
	goldwall.min = (Vector3){ -16, 0, 16.5 };
	goldwall.max = (Vector3){ 16, 5, 17.7 };
	orangewall.min = (Vector3){ -16, 0, -17.5 };
	orangewall.max = (Vector3){ 16, 5, -16.5 };
	// Crude Collisions
	if (CheckCollisionBoxes(doomguy.hitbox, bluewall) || CheckCollisionBoxes(doomguy.hitbox, limewall) || CheckCollisionBoxes(doomguy.hitbox, goldwall) || CheckCollisionBoxes(doomguy.hitbox, orangewall))
	{
		doomguy.camera.position.x = 0;
		doomguy.camera.position.z = 0;
	}

	UpdateCamera(&doomguy.camera);
	doomguy.camera.position.y = 2.f;
	doomguy.camera.up.x = 0.f;
	doomguy.camera.up.z = 0.f;
	doomguy.hitbox.min = (Vector3){ doomguy.camera.position.x - 2.f, 0.f, doomguy.camera.position.z - 2.f };
	doomguy.hitbox.max = (Vector3){ doomguy.camera.position.x + 2.f, 2.f, doomguy.camera.position.z + 2.f };

	enemy.hitbox.max.x = enemy.xpos + 3;
	enemy.hitbox.max.y = 6;
	enemy.hitbox.max.z = enemy.zpos + 3;
	enemy.hitbox.min.x = enemy.xpos - 3;
	enemy.hitbox.min.y = 0;
	enemy.hitbox.min.z = enemy.zpos - 3;

	bool hitenemy = false;
	if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
	{
		doomguy.lgray.direction = Vector3Normalize(Vector3Subtract(doomguy.camera.target, doomguy.camera.position));
		doomguy.lgray.position = doomguy.camera.position;
		hitenemy = CheckCollisionRayBox(doomguy.lgray, enemy.hitbox);
		doomguy.islasing = true;
	}
	else
	{
		doomguy.islasing = false;
	}

	if (hitenemy)
	{
		enemy.health -= 1;
	}

	if (enemy.health <= 0)
	{
		enemy.dead = true;
		enemy.deathcounter++;
	}

	if (!enemy.dead)
	{
		if (CheckCollisionBoxes(doomguy.hitbox, enemy.hitbox) && !enemy.resting)
		{
			doomguy.health -= enemy.damage;
			enemy.resting = true;
		}
		if (enemy.resting)
		{
			enemy.attackcounter -= 1;
			if (enemy.attackcounter <= 0)
			{
				enemy.attackcounter = ATTACK_WAIT;
				enemy.resting = false;
			}
		}
		if (doomguy.health <= 0)
		{
			endgame = true;
		}
		Vector2 enemyloc = { enemy.xpos, enemy.zpos };
		Vector2 playerloc = { doomguy.camera.position.x, doomguy.camera.position.z };
		if (Vector2Distance(playerloc, enemyloc) > 1)
		{
			Vector2 movedir = Vector2Normalize(Vector2Subtract(playerloc, enemyloc));
			enemy.xpos += 0.025 * movedir.x;
			enemy.zpos += 0.025 * movedir.y;
		}
	}
	else if (enemy.awaitingrespawn)
	{
		respawncounter -= 1;
		if (respawncounter <= 0)
		{
			respawn();
		}
	}
}
static void DrawFrame()
{
	BeginDrawing();
	ClearBackground(RAYWHITE);

	UpdateMusicStream(doomguy.mainloop);
	if (!playingmusic)
	{
		PlayMusicStream(doomguy.mainloop);
	}

	if (!endgame)
	{
		BeginMode3D(doomguy.camera);

		DrawPlane((Vector3) { 0.0f, 0.0f, 0.0f }, (Vector2) { 32.0f, 32.0f }, LIGHTGRAY); // Draw ground
		DrawCube((Vector3) { -16.0f, 2.5f, 0.0f }, 1.0f, 5.0f, 32.0f, BLUE);     // Draw a blue wall
		DrawCube((Vector3) { 16.0f, 2.5f, 0.0f }, 1.0f, 5.0f, 32.0f, LIME);      // Draw a green wall
		DrawCube((Vector3) { 0.0f, 2.5f, 16.0f }, 32.0f, 5.0f, 1.0f, GOLD);      // Draw a yellow wall
		DrawCube((Vector3) { 0.f, 2.5f, -16.f }, 32.f, 5.f, 1.f, ORANGE);

		Rectangle frameRec = { 0.f, 0.f, (float)enemy.sprite.width / 4, (float)enemy.sprite.height };
		frameRec.x = (enemy.deathcounter) % 4 * (float)enemy.sprite.width / 4;
		if (!enemy.dead)
			DrawBillboardRec(doomguy.camera, enemy.sprite, frameRec, (Vector3) { enemy.xpos, 4, enemy.zpos }, 8, WHITE);

		EndMode3D();

		DrawText(".", 1280 / 2, 320, 30, RED); // very simple xhair
		DrawText(FormatText("ROUND: %i", roundnum), 50, 60, 50, BROWN);
		if (enemy.dead && !enemy.awaitingrespawn)
		{
			roundnum++;
			enemy.awaitingrespawn = true;
		}
		else if (enemy.dead)
		{
			DrawText("ENEMY SLAIN!", 50, 100, 25, DARKBROWN);
			DrawText(FormatText("Respawns in: %i", respawncounter / 300), 50, 125, 25, DARKBROWN);
		}
		DrawText(FormatText("HEALTH: %i", (int)doomguy.health), 25, 500, 60, WHITE);

		DrawFPS(10, 10);
	}
	else
	{
		StopMusicStream(doomguy.mainloop);
		UpdateMusicStream(doomguy.endloop);
		PlayMusicStream(doomguy.endloop);
		ClearBackground(BLACK);
		DrawText(FormatText("GAME OVER"), 325, 100, 100, RED);
		DrawText(FormatText("YOU SURVIVED: %i ROUND(S)", roundnum), 200, 400, 60, RED);
		DrawText(FormatText("PRESS \'ESC\' TO EXIT"), 800, 680, 30, GREEN);
	}



	EndDrawing();
}

void respawn()
{
	enemy.dead = false;
	enemy.awaitingrespawn = false; 
	int spawnloc = rand() % 4;
	switch (spawnloc)
	{
	case 0:
		enemy.xpos = -15;
		enemy.zpos = -15;
		break;
	case 1:
		enemy.xpos = 15;
		enemy.zpos = -15;
		break;
	case 2:
		enemy.xpos = -15;
		enemy.zpos = 15;
		break;
	case 3:
		enemy.xpos = 15;
		enemy.zpos = 15;
		break;
	}
	enemy.health = enemy.previoushealth * 2;
	enemy.previoushealth = enemy.health;
	respawncounter = RESPAWN_WAIT;
}