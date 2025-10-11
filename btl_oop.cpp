#include <iostream>
#include "raylib.h"
#include <deque>
#include "raymath.h"
#include <algorithm> 
using namespace std;
Color green = {173,204,96,255};
Color darkGreen = {43,51,24,255};
Color pink = {255,192,203,255}; // Dung cho Player 2
const int cellSize = 30; // Kich thuoc moi o vuong tren luoi
const int cellCount = 25; // So luong o vuong theo chieu ngang/doc
const int offset = 75; // Khoang cach tu vien man hinh den khu vuc choi
// kiem tra de khong tao food tren than ran
bool ElementInDeque(Vector2 element, deque<Vector2> dq) { 
    for (unsigned int i = 0; i < dq.size(); i++) { 
        if (Vector2Equals(dq[i], element)) { 
            return true;
        }
    }
    return false;
}

double lastUpdateTime = 0; // Thoi diem cap nhat game gan nhat

// Ham kiem tra xem da den luc di chuyen ran chua
bool eventTriggered(double interval) {
    double currentTime = GetTime(); // GetTime() : tra ve gia tri tu luc game bat dau
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

// Khai bao truoc cac lop
class BaseFood; 
class PoisonFood;
class Snake;
class Game;
class TwoPlayerGame;

// ----------------- LOP LEVEL (CAP DO) -----------------
class Level {
public:
    double interval; // Thoi gian delay giua moi lan di chuyen (the hien toc do)
    Level(double interval = 0.2) { 
        this->interval = interval; 
    }
    virtual ~Level() {} 
};

// Cap do De: Chay cham (delay 0.25s)
class LevelEasy : public Level {
public:
    LevelEasy() : Level(0.25) {} 
};

// Cap do Kho: Chay nhanh (delay 0.1s)
class LevelHard : public Level {
public:
    LevelHard() : Level(0.1) {} 
};

// ----------------- LOP SNAKE (RAN) -----------------
class Snake {
public:
    deque<Vector2> body = {Vector2{6,9}, Vector2{5,9}, Vector2{4,9}}; 
    Vector2 direction = {1,0}; 
    bool addSegment = false; 

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
        
        if(addSegment == true) {
            addSegment = false; 
        } else {
            body.pop_back(); 
        }
    }
    
    // Ham thu nho khi an doc
    void Shrink() {
        if (body.size() > 1) {
            body.pop_back();
            body.pop_back(); // Giam 2 dot
        }
        if (body.size() <= 1) {
            body.clear(); // Neu qua ngan, xoa het (dan den game over)
        }
    }

    // Kiem tra va cham voi mot toa do Food
    bool CheckCollisionWithFood(const Vector2& foodPos) const {
        return Vector2Equals(body[0], foodPos);
    }

    // Kiem tra tu can duoi (khong bao gom dau)
    bool CheckCollisionWithTail() const {
        deque<Vector2> headlessBody = body;
        if (headlessBody.size() > 1) {
            headlessBody.pop_front(); 
            return ElementInDeque(body[0], headlessBody);
        }
        return false;
    }

    // Kiem tra va cham dau voi than mot con ran khac
    bool CheckCollisionWithBody(const deque<Vector2>& otherBody) const {
        return ElementInDeque(body[0], otherBody);
    }

    virtual void Reset() {
        body = {Vector2{6,9}, Vector2{5,9},Vector2{4,9}};
        direction = {1, 0};
    }
};

class Player1 : public Snake {
public:
    Player1() {
        // Khoi tao trong body de tranh loi compiler
        body = {Vector2{6, 9}, Vector2{5, 9}, Vector2{4, 9}};
        direction = {1, 0};
    }
    void Reset() override {
        body = {Vector2{6,9}, Vector2{5,9}, Vector2{4,9}};
        direction = {1,0};
    }
};

class Player2 : public Snake {
public:
    Player2() {
        // Doi vi tri ban dau cua P2 - Khoi tao trong body de tranh loi compiler
        body = {Vector2{cellCount - 7, 20}, Vector2{cellCount - 6, 20}, Vector2{cellCount - 5, 20}}; 
        direction = {-1, 0}; // Doi huong mac dinh sang trai
    }
    void Draw() override {
        for (auto &p : body) {
            Rectangle seg = {offset + p.x * cellSize, offset + p.y * cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(seg, 0.5, 6, pink); // Dung mau PINK cho P2
        }
    }
    void Reset() override {
        body = {Vector2{cellCount - 7, 20}, Vector2{cellCount - 6, 20}, Vector2{cellCount - 5, 20}};
        direction = {-1,0};
    }
};

// ----------------- LOP GAMETIME -----------------
class GameTime {
public:
    double stTime;
    double elapsedTime;// Tong time troi qua
    GameTime() {
        stTime = GetTime();
        elapsedTime = 0.0;
    }

    void Update() {
        elapsedTime = GetTime() - stTime;
    }
};

// ----------------- LOP BASEFOOD (LOP CO SO CHO THUC AN) -----------------
class BaseFood {
public:
    Vector2 position;
    Texture2D texture;
    
    BaseFood(const deque<Vector2>& body1, const deque<Vector2>& body2 = {}) {
        deque<Vector2> combined = CombineSnakeBodies(body1, body2);
        position = GenerateRandomPos(combined); 
    }

    virtual ~BaseFood() {
        UnloadTexture(texture);
    }

    virtual void Draw() {
        DrawTexture(texture,offset + position.x * cellSize,offset + position.y * cellSize,WHITE);
    }

    virtual void OnConsumed(Game& game, Snake& snake) {
        // Logic mac dinh (dung cho NormalFood)
        snake.addSegment = true; 
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

// ----------------- LOP NORMALFOOD (TAO THUONG) -----------------
class NormalFood : public BaseFood {
public:
    NormalFood(const deque<Vector2>& body1, const deque<Vector2>& body2 = {}) : BaseFood(body1, body2) {
        Image image = LoadImage("px_apple_02.png");
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
    }
    void OnConsumed(Game& game, Snake& snake) override; // Khai bao
};

// ----------------- LOP POISONFOOD (THUC AN DOC) -----------------
class PoisonFood : public BaseFood {
public :
    PoisonFood(const deque<Vector2>& body1, const deque<Vector2>& body2 = {}) : BaseFood(body1, body2) {
        Image image = LoadImage("px_tao_thoi.png");
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
    }
    void OnConsumed(Game& game, Snake& snake) override; // Khai bao
}; 

// ----------------- LOP GAME (QUAN LY TRANG THAI TRO CHOI) -----------------
class Game {
public:
    Snake snake;
    NormalFood* normalFood; 
    PoisonFood* poisonFood; 
    bool isPoisonActive = false; 
    double poisonStTime = 0.0; 
    double lastPoisonSpawnTime = 0.0; 

    GameTime gametime;

    bool running = true;
    int score = 0;
    Level* level; 
    // THEM: Bien thong bao Game Over cho ca 1P va 2P
    string gameOverMessage = ""; 

    // Khai bao constructor/destructor (trien khai o duoi)
    Game(Level* lvl);
    virtual ~Game(); // Destructor ao

    // Khai bao cac phuong thuc (trien khai o duoi)
    virtual void Draw(); 
    virtual void Update(); 
    void CheckCollisionWithFood(); 
    void CheckCollisionWithEdges(); 
    virtual void GameOver(); // Ham GameOver goc (khong tham so)
    void CheckCollisionWithTail(); 
    void CheckPoisonSpawn();
};

// ----------------- TRIEN KHAI PHUONG THUC GAME -----------------
Game::Game(Level* lvl) : snake(), level(lvl) {
    normalFood = new NormalFood(snake.body);
    poisonFood = nullptr; 
    lastPoisonSpawnTime = 0.0;
    gameOverMessage = "";
}

Game::~Game() {
    delete normalFood;
    if(poisonFood != nullptr) {
        delete poisonFood;
    }
    delete level;
}

void Game::Draw() {
    normalFood->Draw(); 
    if(isPoisonActive && poisonFood != nullptr) {
        poisonFood->Draw();
    }
    snake.Draw();
}

void Game::Update() {
    if(running) {
        gametime.Update(); 
        CheckPoisonSpawn(); 

        snake.Update();
        CheckCollisionWithFood();
        CheckCollisionWithEdges();
        CheckCollisionWithTail();
    }
}

void Game::CheckCollisionWithFood() {
    if(Vector2Equals(snake.body[0],normalFood->position)) {
        normalFood->OnConsumed(*this, snake); 
        delete normalFood;
        normalFood = new NormalFood(snake.body); 
    }
    
    if(isPoisonActive && Vector2Equals(snake.body[0],poisonFood->position)) {
        poisonFood->OnConsumed(*this, snake); 
        delete poisonFood;
        poisonFood = nullptr;
        isPoisonActive = false; 
    }
}

void Game::CheckPoisonSpawn() {
    LevelHard* hardLevel = dynamic_cast<LevelHard*>(level);
    if (hardLevel != nullptr) {
        // Xu ly Poison Food bien mat sau 6 giay
        if(isPoisonActive && (gametime.elapsedTime - poisonStTime >= 6.0)) {
            delete poisonFood;
            poisonFood = nullptr;
            isPoisonActive = false;
        }
        
        // Xu ly viec tao moi Poison Food
        if(!isPoisonActive && (gametime.elapsedTime - lastPoisonSpawnTime >= 6.0)) {
            poisonFood = new PoisonFood(snake.body); // 1P - Se duoc override trong TwoPlayerGame

            isPoisonActive = true;
            lastPoisonSpawnTime = gametime.elapsedTime;
            poisonStTime = gametime.elapsedTime;
        }
    } 
}

void Game::CheckCollisionWithEdges() {
    if(snake.body[0].x == cellCount || snake.body[0].x == -1 || 
        snake.body[0].y == cellCount || snake.body[0].y == -1) {
        gameOverMessage = "GAME OVER - Va cham canh!"; 
        GameOver();
    }
}

void Game::GameOver() {
    snake.Reset();
    
    delete normalFood; 
    if(poisonFood != nullptr) delete poisonFood;

    normalFood = new NormalFood(snake.body);
    poisonFood = nullptr; 
    isPoisonActive = false;
    
    running = false;
    score = 0; 
}

void Game::CheckCollisionWithTail() {
    if(snake.CheckCollisionWithTail()) {
        gameOverMessage = "GAME OVER - Tu can duoi!"; 
        GameOver();
    }
}

// ----------------- LOP TWOPLAYERGAME (CHE DO 2 NGUOI) -----------------
class TwoPlayerGame : public Game {
public:
    Player1 p1;
    Player2 p2;
    int scoreP1 = 0; 
    int scoreP2 = 0; 

    // BIEN MOI: Trang thai nguoi thang de giai quyet loi Ambiguous Decode
    // 0: Hoa/Ca hai thua, 1: P1 thang, 2: P2 thang
    int winner = 0; 

    TwoPlayerGame(Level* lvl) : Game(lvl) { 
        delete normalFood;
        deque<Vector2> combinedBody = CombineSnakeBodies(p1.body, p2.body);
        normalFood = new NormalFood(p1.body, p2.body);
        poisonFood = nullptr;
    }
    
    // Ghi de ham GameOver() (KHONG CO THAM SO)
    // LUU Y: Day la ham duy nhat co ten GameOver trong lop nay.
    // No doc bien 'winner' de xac dinh thong bao.
    void GameOver() override {
        p1.Reset();
        p2.Reset();
        running = false;
        
        // Thiet lap thong bao DUA VAO bien winner
        if (winner == 1) {
            gameOverMessage = "GAME OVER: PLAYER 1 WINS!";
        } else if (winner == 2) {
            gameOverMessage = "GAME OVER: PLAYER 2 WINS!";
        } else {
            gameOverMessage = "GAME OVER: DRAW! (Ca hai cung thua)";
        }

        // Reset Food va trang thai
        delete normalFood;
        if (poisonFood != nullptr) delete poisonFood;
        
        normalFood = new NormalFood(p1.body, p2.body);
        poisonFood = nullptr;
        isPoisonActive = false;
        winner = 0; // Reset trang thai winner
    }

    void HandleInput() {
        // Player 1 - WASD
        if (IsKeyPressed(KEY_W) && p1.direction.y != 1) p1.direction = {0, -1};
        if (IsKeyPressed(KEY_S) && p1.direction.y != -1) p1.direction = {0, 1};
        if (IsKeyPressed(KEY_A) && p1.direction.x != 1) p1.direction = {-1, 0};
        if (IsKeyPressed(KEY_D) && p1.direction.x != -1) p1.direction = {1, 0};

        // Player 2 - Mui ten
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
            // Xu ly Poison Food bien mat sau 6 giay
            if(isPoisonActive && (gametime.elapsedTime - poisonStTime >= 6.0)) {
                delete poisonFood;
                poisonFood = nullptr;
                isPoisonActive = false;
            }
            
            // Xu ly viec tao moi Poison Food cho 2 Player
            if(!isPoisonActive && (gametime.elapsedTime - lastPoisonSpawnTime >= 6.0)) {
                poisonFood = new PoisonFood(p1.body, p2.body); // Tranh ca 2 ran
                isPoisonActive = true;
                lastPoisonSpawnTime = gametime.elapsedTime;
                poisonStTime = gametime.elapsedTime;
            }
        } 
    }

    void Update() override {
        if (running) {
            gametime.Update();
            CheckPoisonSpawn(); 

            p1.Update();
            p2.Update();

            deque<Vector2> combinedBody = CombineSnakeBodies(p1.body, p2.body);

            // Xu ly va cham Normal Food
            if (p1.CheckCollisionWithFood(normalFood->position)) {
                p1.addSegment = true;
                scoreP1++; 
                delete normalFood; normalFood = new NormalFood(p1.body, p2.body);
            } else if (p2.CheckCollisionWithFood(normalFood->position)) { 
                p2.addSegment = true;
                scoreP2++; 
                delete normalFood; normalFood = new NormalFood(p1.body, p2.body);
            }

            // Xu ly va cham Poison Food
            if (isPoisonActive && poisonFood != nullptr) {
                if (p1.CheckCollisionWithFood(poisonFood->position)) {
                    scoreP1 = max(0, scoreP1 - 2); 
                    p1.Shrink(); 
                    if(p1.body.empty()) {
                        winner = 2; // P1 thua, P2 thang
                        GameOver(); return;
                    }
                    p1.addSegment = false; 
                    delete poisonFood; poisonFood = nullptr; isPoisonActive = false;
                } else if (p2.CheckCollisionWithFood(poisonFood->position)) {
                    scoreP2 = max(0, scoreP2 - 2); 
                    p2.Shrink();
                    if(p2.body.empty()) {
                        winner = 1; // P2 thua, P1 thang
                        GameOver(); return;
                    }
                    p2.addSegment = false; 
                    delete poisonFood; poisonFood = nullptr; isPoisonActive = false;
                }
            }


            // **********************************************
            // LOGIC QUYET DINH THANG THUA (THIET LAP BIEN WINNER)
            // **********************************************
            bool p1Crashed = false; 
            bool p2Crashed = false; 

            // 1. KIEM TRA VA CHAM DAU-DAU (Uu tien cao nhat)
            if (Vector2Equals(p1.body[0], p2.body[0])) {
                winner = 0; // Hoa
                GameOver(); 
                return; 
            }

            // 2. THIET LAP CO CHO TAT CA CAC VA CHAM KHAC
            
            // A. Va cham Canh
            if (p1.body[0].x == cellCount || p1.body[0].x == -1 || p1.body[0].y == cellCount || p1.body[0].y == -1) p1Crashed = true;
            if (p2.body[0].x == cellCount || p2.body[0].x == -1 || p2.body[0].y == cellCount || p2.body[0].y == -1) p2Crashed = true;

            // B. Va cham Duoi (Tu can minh)
            if (p1.CheckCollisionWithTail()) p1Crashed = true;
            if (p2.CheckCollisionWithTail()) p2Crashed = true;

            // C. Va cham Giua hai ran (Dau dam Than doi phuong)
            if (p1.CheckCollisionWithBody(p2.body)) p1Crashed = true; 
            if (p2.CheckCollisionWithBody(p1.body)) p2Crashed = true; 
            
            // 3. QUYET DINH NGUOI THANG DUA TREN CO
            if (p1Crashed && p2Crashed) {
                winner = 0; // Ca hai cung va cham -> Hoa
                GameOver(); 
            } 
            else if (p1Crashed) {
                winner = 2; // P1 thua, P2 thang
                GameOver(); 
            } 
            else if (p2Crashed) {
                winner = 1; // P2 thua, P1 thang
                GameOver(); 
            }
        }
    }

    void Draw() override {
        normalFood->Draw();
        if (isPoisonActive && poisonFood != nullptr)
            poisonFood->Draw();

        p1.Draw(); 
        p2.Draw(); 
    }
};


// ----------------- TRIEN KHAI PHUONG THUC FOOD (SAU KHI DINH NGHIA TwoPlayerGame) -----------------
void NormalFood::OnConsumed(Game& game, Snake& snake) {
    snake.addSegment = true; 
    // Trong 1P, dung diem chung cua Game
    TwoPlayerGame* tpGame = dynamic_cast<TwoPlayerGame*>(&game);
    if (tpGame == nullptr) {
        game.score++; 
    }
    // Trong 2P, logic diem da duoc chuyen ve Update() cua TwoPlayerGame
}

void PoisonFood::OnConsumed(Game& game, Snake& snake) { 
    // Chi xu ly logic giam do dai/Game Over cho che do 1P
    TwoPlayerGame* tpGame = dynamic_cast<TwoPlayerGame*>(&game);
    if (tpGame == nullptr) {
        game.score = max(0, game.score - 2); 
        snake.Shrink(); 
        
        if(snake.body.empty()) {
            game.gameOverMessage = "GAME OVER - Ran qua ngan do an doc!"; 
            game.GameOver(); 
        }
        snake.addSegment = false; 
    }
    // Trong 2P, logic nay da duoc chuyen ve Update() cua TwoPlayerGame
}


// ----------------- HAM MAIN -----------------
int main() {
    InitWindow(2*offset + cellSize * cellCount,2 * offset + cellSize * cellCount, "BTL_OOP");
    SetTargetFPS(60);

    Level* chosenLevel = nullptr;
    Game* game = nullptr;

    // -------- MENU chon level --------
    while (!WindowShouldClose() && chosenLevel == nullptr) {
        BeginDrawing();
        ClearBackground(green);
        DrawText("CHOOSE LEVEL", 260, 220, 50, darkGreen);
        DrawText("Press 1 for EASY",280, 350, 35, darkGreen);
        DrawText("Press 2 for HARD", 280, 400, 35, darkGreen);
        DrawText("Press 3 for TWO PLAYER MODE", 280, 450, 35, darkGreen);
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
    }

    if (chosenLevel == nullptr) { CloseWindow(); return 0; } 

    // -------- GAME LOOP --------
    while (!WindowShouldClose()) {
        BeginDrawing();

        // Cap nhat game theo toc do cua level (easy: 0.25s, hard: 0.1s)
        if (eventTriggered(game->level->interval)) {
            game->Update();
        }

        // Xu ly Input (Dieu khien ran)
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
        } 
        else {
            // Neu la 2 nguoi, goi ham dieu khien rieng
            tpGame->HandleInput();
        }

        // Ve man hinh
        ClearBackground(green);
        // Ve vien ngoai khu vuc choi
        DrawRectangleLinesEx(Rectangle{(float)offset - 5,(float)offset-5,(float)cellSize * cellCount + 10,(float)cellSize * cellCount + 10},5,darkGreen);
        DrawText("OOP-Nhom-2",offset - 5,20,40,darkGreen);
        
        // Hien thi diem so
        if (tpGame != nullptr) {
            DrawText(TextFormat("P1: %i", tpGame->scoreP1), offset - 5, offset + cellSize*cellCount + 10, 40, darkGreen);
            DrawText(TextFormat("P2: %i", tpGame->scoreP2), offset + cellSize*cellCount - 150, offset + cellSize*cellCount + 10, 40, pink);
        } else {
            DrawText(TextFormat("%i",game->score),offset - 5,offset + cellSize*cellCount + 10,40,darkGreen);
        }

        game->Draw();
        
        // HIEN THI THONG BAO GAME OVER
        if (game->running == false) {
            string msg = game->gameOverMessage;
            if (msg.empty()) {
                msg = "GAME OVER. Press ENTER to continue."; 
            }
            DrawText(msg.c_str(), 150, (2*offset + cellSize*cellCount)/2 - 30, 40, RED);

            // Xu ly restart
            if (IsKeyPressed(KEY_ENTER)) {
                game->running = true; 
                game->gameOverMessage = ""; // Xoa thong bao
                // Tai khoi dong GameTime
                game->gametime = GameTime(); 
            }
        }

        EndDrawing();
    }

    delete game; // Don dep bo nho
    CloseWindow();
    return 0;
}