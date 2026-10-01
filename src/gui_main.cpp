#include "game.h"
#include "ui.h"
#include "raylib.h"

int main() {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(640, 780, "Minesweeper - UW Edition");
  SetTargetFPS(60);
  SetExitKey(KEY_NULL);
  {
    Game game;
    UI ui;
    while (!WindowShouldClose()) {
      ui.update(game);
      BeginDrawing();
      ui.draw(game);
      EndDrawing();
    }
  } // Unload fonts while the graphics context is still alive.
  CloseWindow();
}
