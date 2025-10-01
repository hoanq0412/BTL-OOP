#include <iostream>
#include "raylib.h"
#include <deque>
#include "raymath.h"
using namespace std;

Color green = {173,204,96,255};
Color darkGreen = {43,51,24,255};

int cellSize = 30;
int cellCount = 25;
int offset = 75;

bool ElementInDeque(Vector2 element, deque<Vector2> dq) {
    for (unsigned int i = 0; i < dq.size(); i++) {
        if (Vector2Equals(dq[i], element)) {
            return true;
        }
    }
    return false;
}

double lastUpdateTime = 0;

bool eventTriggered(double interval) {
    double currentTime = GetTime();
    if (currentTime - lastUpdateTime >= interval) {
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

// ----------------- LEVEL -----------------
class Level {
public:
    double interval;
    Level(double interval = 0.2) { this->interval = interval; }
    virtual ~Level() {}
};

class LevelEasy : public Level {
public:
    LevelEasy() : Level(0.25) {}  // chậm hơn
};

class LevelHard : public Level {
public:
    LevelHard() : Level(0.1) {}   // nhanh hơn
};
// ----------------------------------------

class Snake {
public:
    deque<Vector2> body = {Vector2{6,9}, Vector2{5,9}, Vector2{4,9}};
    Vector2 direction = {1,0};
    bool addSegment = false;

    void Draw() {
        for (unsigned int i = 0; i < body.size(); i++) {
            float x = body[i].x;
            float y = body[i].y;
            Rectangle segment = Rectangle{offset + x * cellSize,offset + y * cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(segment, 0.5, 6, darkGreen);
        }
    }

    void Update() {
        body.push_front(Vector2Add(body[0],direction));
        if(addSegment == true) {
            addSegment = false;
        } else {
            body.pop_back();
        }
    }

    void Reset() {
        body = {Vector2{6,9}, Vector2{5,9},Vector2{4,9}};
        direction = {1, 0};
    }
};

class Food {
public:
    Vector2 position;
    Texture2D texture;

    Food(deque<Vector2> snakeBody) {
        Image image = LoadImage("px_apple_02.png"); 
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
        position = GenerateRandomPos(snakeBody);
    }

    ~Food() {
        UnloadTexture(texture);
    }

    void Draw() {
        DrawTexture(texture,offset + position.x * cellSize,offset + position.y * cellSize, WHITE);
    }

    Vector2 GenerateRandomCell() {
        float x = GetRandomValue(0, cellCount - 1);
        float y = GetRandomValue(0, cellCount - 1);
        return Vector2{x,y};
    }

    Vector2 GenerateRandomPos(deque<Vector2> snakeBody) {
        Vector2 pos = GenerateRandomCell();
        while (ElementInDeque(pos, snakeBody)) {
            pos = GenerateRandomCell();
        }
        return pos;
    }
};

class Game {
public:
    Snake snake;
    Food food;
    bool running = true;
    int score = 0;
    Level* level;

    Game(Level* lvl) : snake(), food(snake.body), level(lvl) {}

    void Draw() {
        food.Draw();
        snake.Draw();
    }

    void Update() {
        if(running) {
            snake.Update();
            CheckCollisionWithFood();
            CheckCollisionWithEdges();
            CheckCollisionWithTail();
        }
    }

    void CheckCollisionWithFood() {
        if (Vector2Equals(snake.body[0], food.position)) {
            food.position = food.GenerateRandomPos(snake.body);
            snake.addSegment = true;
            score++;
        }
    }

    void CheckCollisionWithEdges() {
        if(snake.body[0].x == cellCount || snake.body[0].x == -1) {
            GameOver();
        }
        if(snake.body[0].y == cellCount || snake.body[0].y == -1) {
            GameOver();
        }
    }

    void GameOver() {
        snake.Reset();
        food.position = food.GenerateRandomPos(snake.body);
        running = false;
        score = 0;
    }

    void CheckCollisionWithTail() {
        deque<Vector2> headlessBody = snake.body;
        headlessBody.pop_front();
        if(ElementInDeque(snake.body[0],headlessBody)) {
            GameOver();
        }
    }
};

int main() {
    InitWindow(2*offset + cellSize * cellCount,2 * offset + cellSize * cellCount, "Retro Snake");
    SetTargetFPS(60);

    Level* chosenLevel = nullptr;

    // -------- MENU chọn level --------
    while (!WindowShouldClose() && chosenLevel == nullptr) {
        BeginDrawing();
        ClearBackground(green);
        DrawText("CHOOSE LEVEL", offset, 150, 40, darkGreen);
        DrawText("Press 1 for EASY", offset, 250, 30, darkGreen);
        DrawText("Press 2 for HARD", offset, 300, 30, darkGreen);
        EndDrawing();

        if (IsKeyPressed(KEY_ONE)) chosenLevel = new LevelEasy();
        if (IsKeyPressed(KEY_TWO)) chosenLevel = new LevelHard();
    }

    if (chosenLevel == nullptr) { CloseWindow(); return 0; } // nếu thoát trước khi chọn

    Game game(chosenLevel);

    // -------- GAME LOOP --------
    while (!WindowShouldClose()) {
        BeginDrawing();

        if (eventTriggered(game.level->interval)) {
            game.Update();
        }

        if (IsKeyPressed(KEY_UP) && game.snake.direction.y != 1) {
            game.snake.direction = {0,-1};
            game.running = true;
        }
        if (IsKeyPressed(KEY_DOWN) && game.snake.direction.y != -1) {
            game.snake.direction = {0,1};
            game.running = true;
        }
        if (IsKeyPressed(KEY_LEFT) && game.snake.direction.x != 1) {
            game.snake.direction = {-1,0};
            game.running = true;
        }
        if (IsKeyPressed(KEY_RIGHT) && game.snake.direction.x != -1) {
            game.snake.direction = {1,0};
            game.running = true;
        }

        ClearBackground(green);
        DrawRectangleLinesEx(Rectangle{(float)offset - 5,(float)offset-5,(float)cellSize * cellCount + 10,(float)cellSize * cellCount + 10},5,darkGreen);
        DrawText("OOP-Nhom-2",offset - 5,20,40,darkGreen);
        DrawText(TextFormat("%i",game.score),offset - 5,offset + cellSize*cellCount + 10,40,darkGreen);
        game.Draw();
        EndDrawing();
    }

    CloseWindow();
}






