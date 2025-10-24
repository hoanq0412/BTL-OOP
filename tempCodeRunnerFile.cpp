#include <iostream>
#include "raylib.h"
#include <deque>
#include "raymath.h"
#include <algorithm>
#include <vector>

using namespace std;
Color green = {173,204,96,255};
Color darkGreen = {43,51,24,255};
Color pink = {255,192,203,255};
const int cellSize = 30;
const int cellCount = 25;
const int offset = 75;

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

deque<Vector2> CombineSnakeBodies(const deque<Vector2>& body1, const deque<Vector2>& body2 = {}) {
    deque<Vector2> combinedBody = body1;
    if (!body2.empty()) {
        combinedBody.insert(combinedBody.end(), body2.begin(), body2.end());
    }
    return combinedBody;
}

class BaseFood; 
class PoisonFood;
class GoldFood;
class Snake;
class Game;
class TwoPlayerGame;

class Level {
public:
    double interval;
    Level(double interval = 0.2) { 
        this->interval = interval; 
    }
    virtual ~Level() {} 
};

class LevelEasy : public Level {
public:
    LevelEasy() : Level(0.25) {} 
};

class LevelHard : public Level {
public:
    LevelHard() : Level(0.1) {} 
};

class Snake {
public:
    deque<Vector2> body = {Vector2{6,9}, Vector2{5,9}, Vector2{4,9}}; 
    Vector2 direction = {1,0}; 
    int segmentsToAdd = 0; // Số đốt cần thêm

    virtual void Draw() {
        for (unsigned int i = 0; i < body.size(); i++) {
            float x = body[i].x;
            float y = body[i].y;
            Rectangle segment = Rectangle{offset + x * cellSize,offset + y * cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(segment, 0.5, 6, darkGreen);
        }
    }

    virtual void Update() {
        body.push_front(Vector2Add(body[0],direction)); 
        
        if(segmentsToAdd > 0) {
            segmentsToAdd--;
        } else {
            body.pop_back(); 
        }
    }
    
    void Shrink() {
        if (body.size() > 1) {
            body.pop_back();
            body.pop_back();
        }
        if (body.size() <= 1) {
            body.clear();
        }
    }

    bool CheckCollisionWithFood(const Vector2& foodPos) const {
        return Vector2Equals(body[0], foodPos);
    }

    bool CheckCollisionWithTail() const {
        deque<Vector2> headlessBody = body;
        if (headlessBody.size() > 1) {
            headlessBody.pop_front(); 
            return ElementInDeque(body[0], headlessBody);
        }
        return false;
    }

    bool CheckCollisionWithBody(const deque<Vector2>& otherBody) const {
        return ElementInDeque(body[0], otherBody);
    }

    virtual void Reset() {
        body = {Vector2{6,9}, Vector2{5,9},Vector2{4,9}};
        direction = {1, 0};
        segmentsToAdd = 0;
    }
};

class Player1 : public Snake {
public:
    Player1() {
        body = {Vector2{6, 9}, Vector2{5, 9}, Vector2{4, 9}};
        direction = {1, 0};
    }
    void Reset() override {
        body = {Vector2{6,9}, Vector2{5,9}, Vector2{4,9}};
        direction = {1,0};
        segmentsToAdd = 0;
    }
};

class Player2 : public Snake {
public:
    Player2() {
        body = {Vector2{cellCount - 7, 20}, Vector2{cellCount - 6, 20}, Vector2{cellCount - 5, 20}}; 
        direction = {-1, 0};
    }
    void Draw() override {
        for (auto &p : body) {
            Rectangle seg = {offset + p.x * cellSize, offset + p.y * cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(seg, 0.5, 6, pink);
        }
    }
    void Reset() override {
        body = {Vector2{cellCount - 7, 20}, Vector2{cellCount - 6, 20}, Vector2{cellCount - 5, 20}};
        direction = {-1,0};
        segmentsToAdd = 0;
    }
};

class GameTime {
public:
    double stTime;
    double elapsedTime;
    GameTime() {
        stTime = GetTime();
        elapsedTime = 0.0;
    }

    void Update() {
        elapsedTime = GetTime() - stTime;
    }
};

class BaseFood {
public:
    Vector2 position;
    Texture2D texture;
    
    // CAP NHAT CÓNTRUCTOR BASE FOOD DE THUC AN XUAT HIEN TRANH CHUONG NGAI VAT
    BaseFood(const deque<Vector2>& body1, const deque<Vector2>& body2 = {}, const vector<Obstacle*>& obstacles = {}) {
        deque<Vector2> combined = CombineSnakeBodies(body1, body2);
        for (const auto& obs : obstacles) {
            combined.push_back(obs->position); //them cac vi tri cnv vao combined
        }
        position = GenerateRandomPos(combined); 
    }

    virtual ~BaseFood() {
        UnloadTexture(texture);
    }

    virtual void Draw() {
        DrawTexture(texture,offset + position.x * cellSize,offset + position.y * cellSize,WHITE);
    }

    virtual void OnConsumed(Game& game, Snake& snake) {
        snake.segmentsToAdd += 1;
    }

public:
    Vector2 GenerateRandomCell() {
        float x = GetRandomValue(0,cellCount - 1);
        float y = GetRandomValue(0,cellCount - 1);
        return Vector2{x,y};
    }

    Vector2 GenerateRandomPos(const deque<Vector2>& combinedBody) {
        Vector2 pos = GenerateRandomCell();
        while(ElementInDeque(pos,combinedBody)) {
            pos = GenerateRandomCell();
        }
        return pos;
    }
};

// UPDATE: TRUYEN VI TRI CHUONG NGAI VAO FOOD
class NormalFood : public BaseFood {
public:
    NormalFood(const deque<Vector2>& body1, const deque<Vector2>& body2 = {}, const vector<Obstacle*>& obstacles = {}) : BaseFood(body1, body2) {
        Image image = LoadImage("px_apple_02.png");
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
    }
    void OnConsumed(Game& game, Snake& snake) override;
};

class PoisonFood : public BaseFood {
public :
    PoisonFood(const deque<Vector2>& body1, const deque<Vector2>& body2 = {}, const vector<Obstacle*>& obstacles = {}) : BaseFood(body1, body2) {
        Image image = LoadImage("px_tao_thoi.png");
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
    }
    void OnConsumed(Game& game, Snake& snake) override;
}; 

class GoldFood : public BaseFood {
public:
    GoldFood(const deque<Vector2>& body1, const deque<Vector2>& body2 = {}, const vector<Obstacle*>& obstacles = {}) : BaseFood(body1, body2) {
        Image image = LoadImage("gold_food.jpg"); // Tên file ảnh GoldFood của bạn
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
    }
    void OnConsumed(Game& game, Snake& snake) override;
};

class Obstacle {
public:
    Vector2 position;

    // Constructor để tạo chướng ngại vật ở một vị trí ngẫu nhiên
    // Cần danh sách các ô đã bị chiếm (rắn, thức ăn) để không tạo đè lên
    Obstacle(const deque<Vector2>& occupiedCells) {
        position = GenerateRandomPos(occupiedCells);
    }

    void Draw() {
        Rectangle rect = {
            offset + position.x * cellSize,
            offset + position.y * cellSize,
            (float)cellSize,
            (float)cellSize
        };
        DrawRectangleRec(rect, GRAY); // Dùng màu xám cho chướng ngại vật
    }

private:
    Vector2 GenerateRandomCell() {
        float x = GetRandomValue(0, cellCount - 1);
        float y = GetRandomValue(0, cellCount - 1);
        return Vector2{x, y};
    }

    Vector2 GenerateRandomPos(const deque<Vector2>& occupiedCells) {
        Vector2 pos = GenerateRandomCell();
        while (ElementInDeque(pos, occupiedCells)) {
            pos = GenerateRandomCell();
        }
        return pos;
    }
};

class Game {
public:
    Snake snake;
    NormalFood* normalFood; 
    PoisonFood* poisonFood; 
    GoldFood* goldFood;
    vector<Obstacle*> obstacles;
    bool isPoisonActive = false; 
    bool isGoldActive = false;
    bool paused = false;
    double poisonStTime = 0.0; 
    double goldStTime = 0.0;
    double lastPoisonSpawnTime = 0.0;
    double lastGoldSpawnTime = 0.0;
    Sound eatSound, eatSound_goldFood, eatSound_poisonFood, crashToSTH;

    GameTime gametime;

    bool running = true;
    int score = 0;
    Level* level; 
    string gameOverMessage = ""; 

    Game(Level* lvl);
    virtual ~Game();

    virtual void Draw(); 
    virtual void Update(); 
    void CheckCollisionWithFood(); 
    void CheckCollisionWithEdges(); 
    virtual void GameOver();
    void CheckCollisionWithTail(); 
    void CheckPoisonSpawn();
    void CheckGoldSpawn();
    void CheckCollisionWithObstacles(); // hàm kiểm tra va chạm
    void GenerateObstacles(); // tạo chướng ngại vật
};

Game::Game(Level* lvl) : snake(), level(lvl) { 
    GenerateObstacles(); // tao chuong ngai vat truoc food

    normalFood = new NormalFood(snake.body, {}, obstacles);
    poisonFood = nullptr;
    goldFood = nullptr;
    lastPoisonSpawnTime = 0.0;
    lastGoldSpawnTime = 0.0;
    gameOverMessage = "";


    eatSound = LoadSound("sound effect/Snake Eating Apple Sound Effect.mp3"); // cua normal food
    eatSound_goldFood = LoadSound("sound effect/ngonthi.mp3");
    eatSound_poisonFood = LoadSound("sound effect/boom.mp3");
    crashToSTH = LoadSound("sound effect/metalpipe.mp3");
}

Game::~Game() {
    delete normalFood;
    if(poisonFood != nullptr) {
        delete poisonFood;
    }
    if(goldFood != nullptr) {
        delete goldFood;
    }
    for (auto obs : obstacles) {
        delete obs;
    }
    obstacles.clear();
    delete level;

    
}

void Game::Draw() {
    normalFood->Draw(); 
    if(isPoisonActive && poisonFood != nullptr) {
        poisonFood->Draw();
    }
    if(isGoldActive && goldFood != nullptr) {
        goldFood->Draw();
    }
    snake.Draw();

    for (auto& obs : obstacles) {
        obs->Draw();
    }
}

void Game::Update() {
    if(running && !paused) {
        gametime.Update(); 
        CheckPoisonSpawn();
        CheckGoldSpawn();

        snake.Update();
        CheckCollisionWithFood();
        CheckCollisionWithEdges();
        CheckCollisionWithTail();
        CheckCollisionWithObstacles();
    }
}

void Game::CheckCollisionWithFood() {
    if(Vector2Equals(snake.body[0],normalFood->position)) {
        PlaySound(eatSound); // neu ran an thi bat sound effect

        normalFood->OnConsumed(*this, snake); 
        delete normalFood;
        normalFood = new NormalFood(snake.body, {}, obstacles);
    }
    
    if(isPoisonActive && Vector2Equals(snake.body[0],poisonFood->position)) {
        PlaySound(eatSound_poisonFood);

        poisonFood->OnConsumed(*this, snake); 
        delete poisonFood;
        poisonFood = nullptr;
        isPoisonActive = false; 
    }

    if(isGoldActive && Vector2Equals(snake.body[0],goldFood->position)) {
        PlaySound(eatSound_goldFood);

        goldFood->OnConsumed(*this, snake);
        delete goldFood;
        goldFood = nullptr;
        isGoldActive = false;
    }
}

void Game::CheckPoisonSpawn() {
    LevelHard* hardLevel = dynamic_cast<LevelHard*>(level);
    if (hardLevel != nullptr) {
        if(isPoisonActive && (gametime.elapsedTime - poisonStTime >= 6.0)) {
            delete poisonFood;
            poisonFood = nullptr;
            isPoisonActive = false;
        }
        
        if(!isPoisonActive && (gametime.elapsedTime - lastPoisonSpawnTime >= 6.0)) {
            poisonFood = new PoisonFood(snake.body, {}, obstacles);
            isPoisonActive = true;
            lastPoisonSpawnTime = gametime.elapsedTime;
            poisonStTime = gametime.elapsedTime;
        }
    } 
}

void Game::CheckGoldSpawn() {
    LevelHard* hardLevel = dynamic_cast<LevelHard*>(level);
    if (hardLevel != nullptr) {
        if(isGoldActive && (gametime.elapsedTime - goldStTime >= 8.0)) {
            delete goldFood;
            goldFood = nullptr;
            isGoldActive = false;
        }
        
        if(!isGoldActive && (gametime.elapsedTime - lastGoldSpawnTime >= 10.0)) {
            goldFood = new GoldFood(snake.body, {}, obstacles);
            isGoldActive = true;
            lastGoldSpawnTime = gametime.elapsedTime;
            goldStTime = gametime.elapsedTime;
        }
    }
}

void Game::CheckCollisionWithEdges() {
    if(snake.body[0].x == cellCount || snake.body[0].x == -1 || 
        snake.body[0].y == cellCount || snake.body[0].y == -1) {
        SetSoundVolume(crashToSTH, 0.2f);
        PlaySound(crashToSTH);

        gameOverMessage = "GAME OVER - Va cham canh!"; 
        GameOver();
    }
}

void Game::GameOver() {
    snake.Reset();

    delete normalFood; 
    if(poisonFood != nullptr) delete poisonFood;
    if(goldFood != nullptr) delete goldFood;

    GenerateObstacles(); // tạo lại các chướng ngại vật khác mỗi khi reset

    normalFood = new NormalFood(snake.body, {}, obstacles);
    poisonFood = nullptr; 
    goldFood = nullptr;
    isPoisonActive = false;
    isGoldActive = false;
    
    running = false;
    score = 0; 
}

void Game::CheckCollisionWithTail() {
    if(snake.CheckCollisionWithTail()) {
        SetSoundVolume(crashToSTH, 0.2f);
        PlaySound(crashToSTH);

        gameOverMessage = "GAME OVER - Tu can duoi!"; 
        GameOver();
    }
}

void Game::GenerateObstacles() {
    // dọn dẹp danh sách cũ khi reset game
    for (auto obs : obstacles) {
        delete obs;
    }
    obstacles.clear();
    
    deque<Vector2> occupiedCells = snake.body;
    occupiedCells.push_back(normalFood->position);
    
    // số chướng ngại vật muốn tạo
    int obstacleCount = 10;
    for (int i = 0; i < obstacleCount; i++) {
        Obstacle* newObstacle = new Obstacle(occupiedCells);
        obstacles.push_back(newObstacle);
        // Thêm vị trí của chướng ngại vật vừa tạo vào danh sách đã chiếm
        // để chướng ngại vật tiếp theo không bị tạo trùng
        occupiedCells.push_back(newObstacle->position);
    }
}

void Game::CheckCollisionWithObstacles() {
    for (auto& obs : obstacles) {
        if (Vector2Equals(snake.body[0], obs->position)) {
            SetSoundVolume(crashToSTH, 0.2f);
            PlaySound(crashToSTH);

            gameOverMessage = "GAME OVER - Va cham chuong ngai vat!";
            GameOver();
        }
    }
}

class TwoPlayerGame : public Game {
public:
    Player1 p1;
    Player2 p2;
    int scoreP1 = 0; 
    int scoreP2 = 0; 
    int winner = 0;
    bool paused = false;

    TwoPlayerGame(Level* lvl) : Game(lvl) { 
        delete normalFood;
        deque<Vector2> combinedBody = CombineSnakeBodies(p1.body, p2.body);
        normalFood = new NormalFood(p1.body, p2.body);
        poisonFood = nullptr;
        goldFood = nullptr;

        GenerateObstacles();
    }
    
    void GameOver() override {
        p1.Reset();
        p2.Reset();
        running = false;
        
        if (winner == 1) {
            gameOverMessage = "GAME OVER: PLAYER 1 WINS!";
        } else if (winner == 2) {
            gameOverMessage = "GAME OVER: PLAYER 2 WINS!";
        } else {
            gameOverMessage = "GAME OVER: DRAW! (Ca hai cung thua)";
        }

        delete normalFood;
        if (poisonFood != nullptr) delete poisonFood;
        if (goldFood != nullptr) delete goldFood;
        
        normalFood = new NormalFood(p1.body, p2.body);
        poisonFood = nullptr;
        goldFood = nullptr;
        isPoisonActive = false;
        isGoldActive = false;
        winner = 0;
    }

    void HandleInput() {
        if (IsKeyPressed(KEY_W) && p1.direction.y != 1) p1.direction = {0, -1};
        if (IsKeyPressed(KEY_S) && p1.direction.y != -1) p1.direction = {0, 1};
        if (IsKeyPressed(KEY_A) && p1.direction.x != 1) p1.direction = {-1, 0};
        if (IsKeyPressed(KEY_D) && p1.direction.x != -1) p1.direction = {1, 0};

        if (IsKeyPressed(KEY_UP) && p2.direction.y != 1) p2.direction = {0, -1};
        if (IsKeyPressed(KEY_DOWN) && p2.direction.y != -1) p2.direction = {0, 1};
        if (IsKeyPressed(KEY_LEFT) && p2.direction.x != 1) p2.direction = {-1, 0};
        if (IsKeyPressed(KEY_RIGHT) && p2.direction.x != -1) p2.direction = {1, 0};

        if(IsKeyDown(KEY_ENTER)) {
            running = true;
        }
    }

    void CheckPoisonSpawn() {
        LevelHard* hardLevel = dynamic_cast<LevelHard*>(level);
        if (hardLevel != nullptr) {
            if(isPoisonActive && (gametime.elapsedTime - poisonStTime >= 6.0)) {
                delete poisonFood;
                poisonFood = nullptr;
                isPoisonActive = false;
            }
            
            if(!isPoisonActive && (gametime.elapsedTime - lastPoisonSpawnTime >= 6.0)) {
                poisonFood = new PoisonFood(p1.body, p2.body);
                isPoisonActive = true;
                lastPoisonSpawnTime = gametime.elapsedTime;
                poisonStTime = gametime.elapsedTime;
            }
        } 
    }

    void CheckGoldSpawn() {
        LevelHard* hardLevel = dynamic_cast<LevelHard*>(level);
        if (hardLevel != nullptr) {
            if(isGoldActive && (gametime.elapsedTime - goldStTime >= 8.0)) {
                delete goldFood;
                goldFood = nullptr;
                isGoldActive = false;
            }
            
            if(!isGoldActive && (gametime.elapsedTime - lastGoldSpawnTime >= 10.0)) {
                goldFood = new GoldFood(p1.body, p2.body);
                isGoldActive = true;
                lastGoldSpawnTime = gametime.elapsedTime;
                goldStTime = gametime.elapsedTime;
            }
        }
    }

    void Update() override {
        if (running && !paused) {
            gametime.Update();
            CheckPoisonSpawn();
            CheckGoldSpawn();

            p1.Update();
            p2.Update();

            deque<Vector2> combinedBody = CombineSnakeBodies(p1.body, p2.body);

            if (p1.CheckCollisionWithFood(normalFood->position)) {
                p1.segmentsToAdd += 1;
                scoreP1++; 
                delete normalFood; normalFood = new NormalFood(p1.body, p2.body);
            } else if (p2.CheckCollisionWithFood(normalFood->position)) { 
                p2.segmentsToAdd += 1;
                scoreP2++; 
                delete normalFood; normalFood = new NormalFood(p1.body, p2.body);
            }

            if (isPoisonActive && poisonFood != nullptr) {
                if (p1.CheckCollisionWithFood(poisonFood->position)) {
                    scoreP1 = max(0, scoreP1 - 2); 
                    p1.Shrink(); 
                    if(p1.body.empty()) {
                        winner = 2;
                        GameOver(); return;
                    }
                    p1.segmentsToAdd = 0;
                    delete poisonFood; poisonFood = nullptr; isPoisonActive = false;
                } else if (p2.CheckCollisionWithFood(poisonFood->position)) {
                    scoreP2 = max(0, scoreP2 - 2); 
                    p2.Shrink();
                    if(p2.body.empty()) {
                        winner = 1;
                        GameOver(); return;
                    }
                    p2.segmentsToAdd = 0;
                    delete poisonFood; poisonFood = nullptr; isPoisonActive = false;
                }
            }

            if (isGoldActive && goldFood != nullptr) {
                if (p1.CheckCollisionWithFood(goldFood->position)) {
                    p1.segmentsToAdd += 2;
                    scoreP1 += 3;
                    delete goldFood; goldFood = nullptr; isGoldActive = false;
                } else if (p2.CheckCollisionWithFood(goldFood->position)) {
                    p2.segmentsToAdd += 2;
                    scoreP2 += 3;
                    delete goldFood; goldFood = nullptr; isGoldActive = false;
                }
            }

            bool p1Crashed = false;
            bool p2Crashed = false;

            for (auto& obs : obstacles) {
                if (Vector2Equals(p1.body[0], obs->position)) {
                    p1Crashed = true;
                }
                if (Vector2Equals(p2.body[0], obs->position)) {
                    p2Crashed = true;
                }
            }

            if (Vector2Equals(p1.body[0], p2.body[0])) {
                winner = 0;
                GameOver(); 
                return; 
            }

            if (p1.body[0].x == cellCount || p1.body[0].x == -1 || p1.body[0].y == cellCount || p1.body[0].y == -1) p1Crashed = true;
            if (p2.body[0].x == cellCount || p2.body[0].x == -1 || p2.body[0].y == cellCount || p2.body[0].y == -1) p2Crashed = true;

            if (p1.CheckCollisionWithTail()) p1Crashed = true;
            if (p2.CheckCollisionWithTail()) p2Crashed = true;

            if (p1.CheckCollisionWithBody(p2.body)) p1Crashed = true; 
            if (p2.CheckCollisionWithBody(p1.body)) p2Crashed = true; 
            
            if (p1Crashed && p2Crashed) {
                winner = 0;
                GameOver(); 
            } 
            else if (p1Crashed) {
                winner = 2;
                GameOver(); 
            } 
            else if (p2Crashed) {
                winner = 1;
                GameOver(); 
            }
        }
    }

    void Draw() override {
        normalFood->Draw();
        if (isPoisonActive && poisonFood != nullptr)
            poisonFood->Draw();
        if (isGoldActive && goldFood != nullptr)
            goldFood->Draw();

        p1.Draw(); 
        p2.Draw(); 

        for (auto& obs : obstacles) {
            obs->Draw();
        }
    }
};

void NormalFood::OnConsumed(Game& game, Snake& snake) {
    snake.segmentsToAdd += 1;
    TwoPlayerGame* tpGame = dynamic_cast<TwoPlayerGame*>(&game);
    if (tpGame == nullptr) {
        game.score++; 
    }
}

void PoisonFood::OnConsumed(Game& game, Snake& snake) { 
    TwoPlayerGame* tpGame = dynamic_cast<TwoPlayerGame*>(&game);
    if (tpGame == nullptr) {
        game.score = max(0, game.score - 2); 
        snake.Shrink(); 
        
        if(snake.body.empty()) {
            game.gameOverMessage = "GAME OVER - Ran qua ngan do an doc!"; 
            game.GameOver(); 
        }
        snake.segmentsToAdd = 0;
    }
}

void GoldFood::OnConsumed(Game& game, Snake& snake) {
    snake.segmentsToAdd += 2; // Thêm 2 đốt
    TwoPlayerGame* tpGame = dynamic_cast<TwoPlayerGame*>(&game);
    if (tpGame == nullptr) {
        game.score += 3; // Thêm 3 điểm
    }
}

int main() {
    InitWindow(2 * offset + cellSize * cellCount, 2 * offset + cellSize * cellCount, "BTL_OOP");
    SetTargetFPS(60);

    Level* chosenLevel = nullptr;
    Game* game = nullptr;

    //THEM AM THANH
    InitAudioDevice(); // ham them am thanh      
    Music menuMusic = LoadMusicStream("sound effect/Menu Theme.mp3"); // gan bien co kieu struct Music de them nhac
    Music bgmMusic = LoadMusicStream("sound effect/Main Theme.mp3");
    SetMusicVolume(menuMusic, 0.5f); // dat gia tri do lon am thanh la 50%
    // mot vai sound effect la thuoc tinh va tao trong constructor cua class Game de chay trong cac phuong thuc

    while (!WindowShouldClose()) {
        PlayMusicStream(menuMusic); // phat nhac khi vao menu
        // ================= MENU CHỌN LEVEL =================
        while (!WindowShouldClose() && chosenLevel == nullptr) {  
            UpdateMusicStream(menuMusic); // bat nhac

            BeginDrawing();
            ClearBackground(green);
            DrawText("CHOOSE LEVEL", 260, 220, 50, darkGreen);
            DrawText("Press 1 for EASY", 280, 350, 35, darkGreen);
            DrawText("Press 2 for HARD", 280, 400, 35, darkGreen);
            DrawText("Press 3 for TWO PLAYER MODE", 280, 450, 35, darkGreen);
            DrawText("Press ESC to EXIT", 280, 500, 35, RED);
            EndDrawing();

            if (IsKeyPressed(KEY_ONE)) {
                chosenLevel = new LevelEasy();
                game = new Game(chosenLevel);
            }
            if (IsKeyPressed(KEY_TWO)) {
                chosenLevel = new LevelHard();
                game = new Game(chosenLevel);
            }
            if (IsKeyPressed(KEY_THREE)) {
                chosenLevel = new LevelHard();
                game = new TwoPlayerGame(chosenLevel);
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                CloseWindow();
                return 0;
            }
        }

        StopMusicStream(menuMusic);

        PlayMusicStream(bgmMusic); // de phat nhac khi trong game

        // ================= VÒNG LẶP CHƠI GAME =================
        while (!WindowShouldClose() && chosenLevel != nullptr) {
            BeginDrawing();
            UpdateMusicStream(bgmMusic); // bat nhac
            // ---- PAUSE GAME ----
            if (IsKeyPressed(KEY_P)) {
                game->paused = !game->paused;
            }

            // ---- THOÁT VỀ MENU CHỌN LEVEL ----
            if (IsKeyPressed(KEY_Z)) {
                StopMusicStream(bgmMusic); // dung nhac khi thoat game
                delete game;
                chosenLevel = nullptr;
                game = nullptr;
                break; // ← THOÁT KHỎI VÒNG LẶP GAME, QUAY LẠI MENU
            }

            // ---- CẬP NHẬT GAME ----
            if (!game->paused && eventTriggered(game->level->interval)) {
                game->Update();
            }

            TwoPlayerGame* tpGame = dynamic_cast<TwoPlayerGame*>(game);
            if (tpGame == nullptr) {
                if (IsKeyPressed(KEY_UP) && game->snake.direction.y != 1) {
                    game->snake.direction = {0, -1};
                    game->running = true;
                }
                if (IsKeyPressed(KEY_DOWN) && game->snake.direction.y != -1) {
                    game->snake.direction = {0, 1};
                    game->running = true;
                }
                if (IsKeyPressed(KEY_LEFT) && game->snake.direction.x != 1) {
                    game->snake.direction = {-1, 0};
                    game->running = true;
                }
                if (IsKeyPressed(KEY_RIGHT) && game->snake.direction.x != -1) {
                    game->snake.direction = {1, 0};
                    game->running = true;
                }
            } else {
                tpGame->HandleInput();
            }

            // ---- VẼ GAME ----
            ClearBackground(green);
            DrawRectangleLinesEx(Rectangle{(float)offset - 5, (float)offset - 5,
                (float)cellSize * cellCount + 10, (float)cellSize * cellCount + 10}, 5, darkGreen);
            DrawText("OOP-Nhom-2", offset - 5, 20, 40, darkGreen);

            if (tpGame != nullptr) {
                DrawText(TextFormat("P1: %i", tpGame->scoreP1),
                         offset - 5, offset + cellSize * cellCount + 10, 40, darkGreen);
                DrawText(TextFormat("P2: %i", tpGame->scoreP2),
                         offset + cellSize * cellCount - 150,
                         offset + cellSize * cellCount + 10, 40, pink);
            } else {
                DrawText(TextFormat("%i", game->score),
                         offset - 5, offset + cellSize * cellCount + 10, 40, darkGreen);
            }

            game->Draw();

            // ---- HIỂN THỊ PAUSE ----
            if (game->paused) {
                DrawText("PAUSED", 320, 320, 50, RED);
            }

            // ---- HIỂN THỊ GAME OVER ----
            if (!game->running) {
                string msg = game->gameOverMessage;
                if (msg.empty()) msg = "GAME OVER. Press ENTER to continue.";
                DrawText(msg.c_str(), 150, (2 * offset + cellSize * cellCount) / 2 - 30, 40, RED);

                if (IsKeyPressed(KEY_ENTER)) {
                    game->running = true;
                    game->gameOverMessage = "";
                    game->gametime = GameTime();
                }
            }

            EndDrawing();
        }
    }

    delete game;

    UnloadMusicStream(menuMusic);
    UnloadMusicStream(bgmMusic); // dung han nhac khi thoat tro choi
    CloseAudioDevice(); // tat he thong am thanh
    CloseWindow();
    return 0;
}
