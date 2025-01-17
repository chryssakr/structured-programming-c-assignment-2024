/*
 * monomaxia.c
 *
 * A simple 2D “Monomaxia” (battleship duel) demo using the raylib library.
 *
 * Controls:
 *    - Player1 (labelled 'A' on map):
 *        Movement with W, A, S, D
 *        Fire with Left Shift
 *    - Player2 (labelled 'B' on map):
 *        Movement with arrow keys
 *        Fire with Right Shift
 *
 * Compile on terminal (Mac):
 *    gcc monomaxia.c -o monomaxia -lraylib
 *
 * Then run:
 *    ./monomaxia.exe
 */

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// ---------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------
#define SCREEN_SCALE 64 // Each cell is 64×64 pixels
#define MAP_WIDTH 20
#define MAP_HEIGHT 10

#define MAX_PLAYERS 2
#define MAX_PROJECTILES 5
#define MAX_SPEED 1 // Movement speed (in cells) per frame

// For drawing a net:
#define NET_LINE_SPACING 32 // in pixels

// ---------------------------------------------------------------------
//  Structs
// ---------------------------------------------------------------------

typedef struct
{
    int x, y;    // Position in map cells
    int dx, dy;  // Movement direction
    bool active; // Whether projectile is still moving
} Projectile;

typedef struct
{
    int x, y; // Position (map cells)
    int hp;   // “Health” points
    // Movement each frame (set by input)
    int vx, vy;
    // Array of projectiles
    Projectile projectiles[MAX_PROJECTILES];
} Ship;

typedef struct
{
    char name[50];
    Ship ship;
} Player;

typedef struct
{
    Player players[MAX_PLAYERS];
    bool gameOver;
    char map[MAP_HEIGHT][MAP_WIDTH];
} GameState;

// ---------------------------------------------------------------------
//  Forward Declarations
// ---------------------------------------------------------------------
void InitGame(GameState *game);
void DrawGame(const GameState *game);
void UpdateGame(GameState *game);

// Helper subroutines
static void InitMap(GameState *game);
static void InitShip(Ship *ship, int startX, int startY);
static void InitProjectile(Projectile *p);

static void HandleInput(GameState *game);
static void UpdateShips(GameState *game);
static void UpdateProjectiles(GameState *game);
static void CheckHits(GameState *game);

// New helper for drawing the “bay” background & net
static void DrawBayBackground(int screenWidth, int screenHeight);

// ---------------------------------------------------------------------
//  Main Entry
// ---------------------------------------------------------------------
int main(void)
{
    const int screenWidth = MAP_WIDTH * SCREEN_SCALE;
    const int screenHeight = MAP_HEIGHT * SCREEN_SCALE;

    InitWindow(screenWidth, screenHeight, "Monomaxia on a Bay");
    SetTargetFPS(60);

    GameState game;
    InitGame(&game);

    while (!WindowShouldClose())
    {
        if (!game.gameOver)
        {
            // 1) Handle keyboard input -> movement & firing
            HandleInput(&game);

            // 2) Update ships
            UpdateShips(&game);

            // 3) Update projectiles
            UpdateProjectiles(&game);

            // 4) Check hits
            CheckHits(&game);
        }

        // 5) Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw the entire scene
        DrawGame(&game);

        // If game is over, show a message
        if (game.gameOver)
        {
            // Identify winner or tie
            int hpA = game.players[0].ship.hp;
            int hpB = game.players[1].ship.hp;
            if (hpA <= 0 && hpB <= 0)
            {
                DrawText("TIE! Nobody survived!", 40, 10, 30, RED);
            }
            else
            {
                int winnerIndex = (hpB > hpA) ? 1 : 0;
                char winnerMsg[100];
                snprintf(winnerMsg, sizeof(winnerMsg),
                         "GAME OVER! Winner: %s", game.players[winnerIndex].name);
                DrawText(winnerMsg, 40, 10, 30, RED);
            }
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

// ---------------------------------------------------------------------
//  InitGame
// ---------------------------------------------------------------------
void InitGame(GameState *game)
{
    memset(game, 0, sizeof(GameState));
    InitMap(game);

    // Player names
    strcpy(game->players[0].name, "Player1");
    strcpy(game->players[1].name, "Player2");

    // Initialize ships in the corners
    InitShip(&game->players[0].ship, 2, 2);
    InitShip(&game->players[1].ship, MAP_WIDTH - 2, MAP_HEIGHT - 2);

    game->gameOver = false;
}

// ---------------------------------------------------------------------
//  Map / Ship / Projectile
// ---------------------------------------------------------------------
static void InitMap(GameState *game)
{
    for (int r = 0; r < MAP_HEIGHT; r++)
    {
        for (int c = 0; c < MAP_WIDTH; c++)
        {
            if (r == 0 || r == MAP_HEIGHT - 1 || c == 0 || c == MAP_WIDTH - 1)
                game->map[r][c] = '#'; // boundary
            else
                game->map[r][c] = '.';
        }
    }
    // Some obstacles
    game->map[3][5] = 'X';
    game->map[5][8] = 'X';
    game->map[6][10] = 'X';
}

static void InitShip(Ship *ship, int startX, int startY)
{
    ship->x = startX;
    ship->y = startY;
    ship->hp = 3;
    ship->vx = 0;
    ship->vy = 0;

    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        InitProjectile(&ship->projectiles[i]);
    }
}

static void InitProjectile(Projectile *p)
{
    p->x = -1;
    p->y = -1;
    p->dx = 0;
    p->dy = 0;
    p->active = false;
}

// ---------------------------------------------------------------------
//  Drawing the “Bay”
// ---------------------------------------------------------------------
static void DrawBayBackground(int screenWidth, int screenHeight)
{
    // Draw a large brown border (the “land”) by drawing a rectangle
    // and then a slightly smaller “water” rectangle on top of it.
    DrawRectangle(0, 0, screenWidth, screenHeight, BROWN);

    // Fill an inner rectangle with “water” color
    DrawRectangle(0, 0, screenWidth, screenHeight, BLUE);

    // Draw net lines on top of the water
    for (int x = 0; x < screenWidth; x += NET_LINE_SPACING)
    {
        DrawLine(x, 0, x, screenHeight, Fade(LIGHTGRAY, 0.5f));
    }
    for (int y = 0; y < screenHeight; y += NET_LINE_SPACING)
    {
        DrawLine(0, y, screenWidth, y, Fade(LIGHTGRAY, 0.5f));
    }
}

// ---------------------------------------------------------------------
//  DrawGame
// ---------------------------------------------------------------------
void DrawGame(const GameState *game)
{
    int screenWidth = MAP_WIDTH * SCREEN_SCALE;
    int screenHeight = MAP_HEIGHT * SCREEN_SCALE;

    // Draw the bay background with net lines
    DrawBayBackground(screenWidth, screenHeight);

    // Next, draw obstacles from the map
    for (int r = 0; r < MAP_HEIGHT; r++)
    {
        for (int c = 0; c < MAP_WIDTH; c++)
        {
            char cell = game->map[r][c];
            if (cell == '#' || cell == 'X')
            {
                // Represent obstacles or boundary (X or #) as gray squares
                DrawRectangle(c * SCREEN_SCALE, r * SCREEN_SCALE,
                              SCREEN_SCALE, SCREEN_SCALE, DARKGRAY);
            }
        }
    }

    // Draw projectiles
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        for (int j = 0; j < MAX_PROJECTILES; j++)
        {
            const Projectile *p = &game->players[i].ship.projectiles[j];
            if (p->active)
            {
                Color col = (i == 0) ? RED : GREEN;
                DrawCircle(p->x * SCREEN_SCALE + SCREEN_SCALE / 2,
                           p->y * SCREEN_SCALE + SCREEN_SCALE / 2,
                           SCREEN_SCALE / 4.0f, col);
            }
        }
    }

    // Draw ships
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (game->players[i].ship.hp > 0)
        {
            // Ship color
            Color shipColor = (i == 0) ? RED : GREEN;
            // Label character
            char labelStr[2] = {(i == 0) ? 'A' : 'B', '\0'};

            int shipSize = SCREEN_SCALE / 2; // 32×32 if SCREEN_SCALE=64
            // Center it in the cell
            int offset = (SCREEN_SCALE - shipSize); // 16 px offset

            // The top-left corner of the current cell in pixels
            int cellX = game->players[i].ship.x * SCREEN_SCALE;
            int cellY = game->players[i].ship.y * SCREEN_SCALE;

            // Drawing the ship
            DrawRectangle(cellX + offset,
                          cellY + offset,
                          shipSize,
                          shipSize,
                          shipColor);

            // Drawing the label on the ship
            DrawText(labelStr,
                     cellX + offset + shipSize / 4, // horizontally centering the text
                     cellY + offset + shipSize / 4, // vertically centering
                     shipSize / 2,                  // text size = half the ship size
                     WHITE);

            // Show HP above the ship
            char hpStr[16];
            snprintf(hpStr, sizeof(hpStr), "HP:%d", game->players[i].ship.hp);
            DrawText(hpStr,
                     cellX + offset,
                     cellY + offset - 13,
                     14, // slightly smaller font
                     BLACK);
        }
    }
}

// ---------------------------------------------------------------------
//  HandleInput
//    Read the keyboard and set each player’s
//    vx, vy, and possibly spawn projectiles
// ---------------------------------------------------------------------
static void HandleInput(GameState *game)
{
    // Reset velocities each frame
    game->players[0].ship.vx = 0;
    game->players[0].ship.vy = 0;
    game->players[1].ship.vx = 0;
    game->players[1].ship.vy = 0;

    // Player1 (WASD) + Fire = Left Shift
    if (IsKeyDown(KEY_W))
        game->players[0].ship.vy = -MAX_SPEED;
    if (IsKeyDown(KEY_S))
        game->players[0].ship.vy = MAX_SPEED;
    if (IsKeyDown(KEY_A))
        game->players[0].ship.vx = -MAX_SPEED;
    if (IsKeyDown(KEY_D))
        game->players[0].ship.vx = MAX_SPEED;

    if (IsKeyPressed(KEY_LEFT_SHIFT))
    {
        // Fire
        for (int i = 0; i < MAX_PROJECTILES; i++)
        {
            Projectile *p = &game->players[0].ship.projectiles[i];
            if (!p->active)
            {
                int dx = game->players[0].ship.vx;
                int dy = game->players[0].ship.vy;
                if (dx == 0 && dy == 0)
                    dy = -1; // default shoot upward if still

                p->x = game->players[0].ship.x;
                p->y = game->players[0].ship.y;
                p->dx = dx;
                p->dy = dy;
                p->active = true;
                break;
            }
        }
    }

    // Player2 (arrows) + Fire = Right Shift
    if (IsKeyDown(KEY_UP))
        game->players[1].ship.vy = -MAX_SPEED;
    if (IsKeyDown(KEY_DOWN))
        game->players[1].ship.vy = MAX_SPEED;
    if (IsKeyDown(KEY_LEFT))
        game->players[1].ship.vx = -MAX_SPEED;
    if (IsKeyDown(KEY_RIGHT))
        game->players[1].ship.vx = MAX_SPEED;

    if (IsKeyPressed(KEY_RIGHT_SHIFT))
    {
        // Fire
        for (int i = 0; i < MAX_PROJECTILES; i++)
        {
            Projectile *p = &game->players[1].ship.projectiles[i];
            if (!p->active)
            {
                int dx = game->players[1].ship.vx;
                int dy = game->players[1].ship.vy;
                if (dx == 0 && dy == 0)
                    dy = -1;

                p->x = game->players[1].ship.x;
                p->y = game->players[1].ship.y;
                p->dx = dx;
                p->dy = dy;
                p->active = true;
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------
//  UpdateShips
//    - Attempt to move each ship in the direction of (vx, vy)
//    - Check collision with obstacles
// ---------------------------------------------------------------------
static void UpdateShips(GameState *game)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        Ship *ship = &game->players[i].ship;
        int nx = ship->x + ship->vx;
        int ny = ship->y + ship->vy;

        // Collisions with map boundary or obstacle
        if (nx < 0 || nx >= MAP_WIDTH ||
            ny < 0 || ny >= MAP_HEIGHT ||
            game->map[ny][nx] == '#' ||
            game->map[ny][nx] == 'X')
        {
            // Collide: lose 1 HP, do not move
            ship->hp--;
            if (ship->hp <= 0)
                game->gameOver = true;
        }
        else
        {
            // No collision, update ship position
            ship->x = nx;
            ship->y = ny;
        }
    }
}

// ---------------------------------------------------------------------
//  UpdateProjectiles
// ---------------------------------------------------------------------
static void UpdateProjectiles(GameState *game)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        for (int j = 0; j < MAX_PROJECTILES; j++)
        {
            Projectile *p = &game->players[i].ship.projectiles[j];
            if (p->active)
            {
                int nx = p->x + p->dx;
                int ny = p->y + p->dy;
                if (nx < 0 || nx >= MAP_WIDTH ||
                    ny < 0 || ny >= MAP_HEIGHT ||
                    game->map[ny][nx] == '#' ||
                    game->map[ny][nx] == 'X')
                {
                    // obstacle or boundary
                    p->active = false;
                }
                else
                {
                    p->x = nx;
                    p->y = ny;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------
//  CheckHits
//    - For each projectile, see if it hits the opposing ship
// ---------------------------------------------------------------------
static void CheckHits(GameState *game)
{
    Ship *shipA = &game->players[0].ship;
    Ship *shipB = &game->players[1].ship;

    // If either is already 0 HP, do nothing
    if (shipA->hp <= 0 && shipB->hp <= 0)
    {
        game->gameOver = true;
        return;
    }

    // A's projectiles -> B
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        Projectile *p = &shipA->projectiles[i];
        if (p->active && p->x == shipB->x && p->y == shipB->y)
        {
            shipB->hp--;
            p->active = false;
            if (shipB->hp <= 0)
            {
                game->gameOver = true;
                return;
            }
        }
    }

    // B's projectiles -> A
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        Projectile *p = &shipB->projectiles[i];
        if (p->active && p->x == shipA->x && p->y == shipA->y)
        {
            shipA->hp--;
            p->active = false;
            if (shipA->hp <= 0)
            {
                game->gameOver = true;
                return;
            }
        }
    }
}
