#include <iostream>
#include "raylib.h"
#include <deque>
#include "raymath.h"
#include <algorithm> 
using namespace std;
Color green = {173,204,96,255};
Color darkGreen = {43,51,24,255};
int cellSize = 30; // Kích thước mỗi ô vuông trên lưới
int cellCount = 25; // Số lượng ô vuông theo chiều ngang/dọc
int offset = 75; // Khoảng cách từ viền màn hình đến khu vực chơi
// kiem tra de khong tao food tren than ran
bool ElementInDeque(Vector2 element, deque<Vector2> dq) { 
    for (unsigned int i = 0; i < dq.size(); i++) { 
        if (Vector2Equals(dq[i], element)) { 
            return true;
        }
    }
    return false;
}

double lastUpdateTime = 0; // Thời điểm cập nhật game gần nhất

// Hàm kiểm tra xem đã đến lúc di chuyển rắn chưa
bool eventTriggered(double interval) {
    double currentTime = GetTime(); // GetTime() : trả về giá trị từ lúc game bắt đầu
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

// ----------------- LỚP LEVEL (CẤP ĐỘ) -----------------
class Level {
public:
    double interval; // Thời gian delay giữa mỗi lần di chuyển (thể hiện tốc độ)
    Level(double interval = 0.2) { 
        this->interval = interval; 
    }
    virtual ~Level() {} // Destructor ảo để xóa đúng đối tượng con
};

// Cấp độ Dễ: Chạy chậm (delay 0.25s)
class LevelEasy : public Level {
public:
    LevelEasy() : Level(0.25) {} 
};

// Cấp độ Khó: Chạy nhanh (delay 0.1s)
class LevelHard : public Level {
public:
    LevelHard() : Level(0.1) {} 
};

// ----------------- LỚP SNAKE (RẮN) -----------------
class Snake {
public:
    // Thân rắn: danh sách các ô vuông (tọa độ)
    deque<Vector2> body = {Vector2{6,9}, Vector2{5,9}, Vector2{4,9}}; 
    Vector2 direction = {1,0}; // Hướng di chuyển mặc định: sang phải (x=1, y=0)
    bool addSegment = false; // Cờ báo hiệu có nên thêm đốt sau khi ăn mồi không

    // Vẽ thân rắn lên màn hình
    virtual void Draw() {
        for (unsigned int i = 0; i < body.size(); i++) {
            float x = body[i].x;
            float y = body[i].y;
            // Tính toán vị trí pixel từ tọa độ lưới và vẽ
            Rectangle segment = Rectangle{offset + x * cellSize,offset + y * cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(segment, 0.5, 6, darkGreen);
        }
    }

    // Cập nhật vị trí rắn (di chuyển)
    virtual void Update() {
        // 1. Thêm đầu mới vào phía trước theo hướng di chuyển hiện tại
        body.push_front(Vector2Add(body[0],direction)); 
        
        // 2. Xử lý logic dài ra/ngắn lại
        if(addSegment == true) {
            addSegment = false; // Đã thêm đốt, reset cờ
        } else {
            body.pop_back(); // Không ăn mồi hoặc ăn mồi độc -> Xóa đuôi cũ (di chuyển)
        }
    }

    // Thiết lập lại trạng thái ban đầu khi GameOver
    virtual void Reset() {
        body = {Vector2{6,9}, Vector2{5,9},Vector2{4,9}};
        direction = {1, 0};
    }
};

class Player1 : public Snake {
public:
    Player1() {
        body = {Vector2{6,9}, Vector2{5,9}, Vector2{4,9}};
        direction = {1,0};
    }
    void Reset() override {
        body = {Vector2{6,9}, Vector2{5,9}, Vector2{4,9}};
        direction = {1,0};
    }
};

class Player2 : public Snake {
public:
    Player2() {
        body = {Vector2{6,20}, Vector2{5,20}, Vector2{4,20}};
        direction = {1,0};
    }
    void Draw() override {
        for (auto &p : body) {
            Rectangle seg = {offset + p.x * cellSize, offset + p.y * cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(seg, 0.5, 6, PINK);
        }
    }
    void Reset() override {
        body = {Vector2{6,20}, Vector2{5,20}, Vector2{4,20}};
        direction = {1,0};
    }
};

// ----------------- LỚP GAMETIME -----------------
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

    // Hàm TimeMod không cần thiết nữa với logic CheckPoisonSpawn mới
    bool TimeMod(int mod) {
        return ((int)elapsedTime % mod == 0); // check dk chia het
    } 
};

// Khai báo trước các lớp Food
class BaseFood; 
class NormalFood;
class PoisonFood;

// ----------------- LỚP GAME (QUẢN LÝ TRẠNG THÁI TRÒ CHƠI) -----------------
class Game {
public:
    Snake snake;
    BaseFood* normalFood; // Food thường
    PoisonFood* poisonFood; // Food độc
    bool isPoisonActive = false; // Cờ báo Poison Food đang hiển thị
    double poisonStTime = 0.0; // Thời điểm Poison Food xuất hiện (để tính thời gian tồn tại)
    double lastPoisonSpawnTime = 0.0; // Thời điểm Poison Food gần nhất được tạo

    GameTime gametime;

    bool running = true;
    int score = 0;
    Level* level; // Con trỏ tới cấp độ 

    // Khai báo constructor/destructor (triển khai ở dưới)
    Game(Level* lvl);
    ~Game();

    // Khai báo các phương thức (triển khai ở dưới để giải quyết dependency)
    virtual void Draw(); 
    virtual void Update(); 
    void CheckCollisionWithFood(); 
    void CheckCollisionWithEdges(); 
    virtual void GameOver(); 
    void CheckCollisionWithTail(); 
    void CheckPoisonSpawn();
};

// ----------------- LỚP BASEFOOD (LỚP CƠ SỞ CHO THỨC ĂN) -----------------
class BaseFood {
public:
    Vector2 position;
    Texture2D texture;
    
    BaseFood(deque<Vector2> snakeBody) {
        position = GenerateRandomPos(snakeBody); // Tạo vị trí ngẫu nhiên khi khởi tạo
    }

    virtual ~BaseFood() {
        UnloadTexture(texture);
    }

    virtual void Draw() {
        DrawTexture(texture,offset + position.x * cellSize,offset + position.y * cellSize,WHITE);
    }

    // Phương thức ảo xử lý hiệu ứng khi bị ăn. Game đã được định nghĩa.
    virtual void OnConsumed(Game& game, Snake& snake) {
        snake.addSegment = true; // Thân rắn dài ra
        game.score++; // Tăng 1 điểm (dù là 1P hay 2P, điểm vẫn nằm trong Game object)
    }

public:
    // Tạo tọa độ ô vuông ngẫu nhiên trên lưới
    Vector2 GenerateRandomCell() {
        float x = GetRandomValue(0,cellCount - 1);
        float y = GetRandomValue(0,cellCount - 1);
        return Vector2{x,y};
    }

    // Tạo tọa độ ngẫu nhiên sao cho không trùng với thân rắn
    Vector2 GenerateRandomPos(deque<Vector2> snakeBody) {
        Vector2 pos = GenerateRandomCell();
        while(ElementInDeque(pos,snakeBody)) {
            pos = GenerateRandomCell();
        }
        return pos;
    }
};

// ----------------- LỚP NORMALFOOD (TÁO THƯỜNG) -----------------
class NormalFood : public BaseFood {
public:
    NormalFood(deque<Vector2> snakeBody) : BaseFood(snakeBody) {
        Image image = LoadImage("px_apple_02.png");
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
    }
};

// ----------------- LỚP POISONFOOD (THỨC ĂN ĐỘC) -----------------
class PoisonFood : public BaseFood {
public :
    PoisonFood(deque<Vector2> snakeBody) : BaseFood(snakeBody) {
        Image image = LoadImage("px_tao_thoi.png");
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
    }

    // Ghi đè hiệu ứng tiêu cực
    void OnConsumed(Game& game, Snake& snake) override { 
        game.score = max(0, game.score - 2); // Giảm 2 điểm
        
    // Giảm độ dài rắn (nếu rắn đủ dài)
        if(snake.body.size() > 3) { // Dùng tham số snake
            snake.body.pop_back(); // Rắn ngắn lại 1 đốt
        } else {
            game.GameOver(); // Nếu rắn quá ngắn, kết thúc game
        }
            snake.addSegment = false; // Đảm bảo rắn không dài ra (ngay cả khi ngắn lại)
    }
}; 

// ----------------- TRIỂN KHAI CÁC PHƯƠNG THỨC CỦA GAME -----------------

// Constructor: Khởi tạo Game với level được chọn và tạo NormalFood đầu tiên
Game::Game(Level* lvl) : snake(), level(lvl) {
    normalFood = new NormalFood(snake.body);
    poisonFood = nullptr; // Bắt đầu không có Poison Food
    lastPoisonSpawnTime = 0.0;
}

// Destructor: Dọn dẹp bộ nhớ đã cấp phát
Game::~Game() {
    delete normalFood;
    if(poisonFood != nullptr) {
        delete poisonFood;
    }
    delete level;
}

// Vẽ thức ăn và rắn
void Game::Draw() {
    normalFood->Draw(); // luon ve
    
    // Chỉ vẽ Poison Food nếu nó đang hoạt động (CHỈ XẢY RA TRONG HARD MODE)
    if(isPoisonActive && poisonFood != nullptr) {
        poisonFood->Draw();
    }

    snake.Draw();
}

// Cập nhật trạng thái game mỗi tick
void Game::Update() {
    if(running) {
        gametime.Update(); // Cập nhật thời gian game
        CheckPoisonSpawn(); // Kiểm tra tạo/xóa Poison Food (chỉ Hard Mode)

        snake.Update();
        CheckCollisionWithFood();
        CheckCollisionWithEdges();
        CheckCollisionWithTail();
    }
}

// Kiểm tra va chạm với thức ăn
void Game::CheckCollisionWithFood() {
    // Check va chạm với Normal Food
    if(Vector2Equals(snake.body[0],normalFood->position)) {
        normalFood->OnConsumed(*this, snake); // **truyền cả con rắn**
        delete normalFood;
        normalFood = new NormalFood(snake.body); // Tạo Normal Food mới
    }
    
    // Check va chạm với Poison Food
    if(isPoisonActive && Vector2Equals(snake.body[0],poisonFood->position)) {
        poisonFood->OnConsumed(*this, snake); // **truyền cả con rắn**
        delete poisonFood;
        poisonFood = nullptr;
        isPoisonActive = false; // Tắt cờ Poison Food
    }
}

// Kiem tra viec tao poison food, CHỈ TRONG HARD MODE
void Game::CheckPoisonSpawn() {
    // 1. Kiểm tra cấp độ
    LevelHard* hardLevel = dynamic_cast<LevelHard*>(level);

    // CHỈ CHẠY LOGIC NÀY NẾU LÀ HARD MODE
    if (hardLevel != nullptr) {
        
        // Xử lý Poison Food biến mất sau 6 giây
        if(isPoisonActive && (gametime.elapsedTime - poisonStTime >= 6.0)) {
            delete poisonFood;
            poisonFood = nullptr;
            isPoisonActive = false;
        }
        
        // Xử lý việc tạo mới Poison Food (Tạo mới nếu chưa có và đủ 6 giây trôi qua)
        if(!isPoisonActive && (gametime.elapsedTime - lastPoisonSpawnTime >= 6.0)) {
            // Đảm bảo không tạo khi Normal Food đang bị lỗi hoặc không tồn tại (chỉ là biện pháp phòng ngừa)
            if(normalFood != nullptr) { 
                poisonFood = new PoisonFood(snake.body);
                isPoisonActive = true;
                // Cập nhật thời điểm tạo Poison Food gần nhất
                lastPoisonSpawnTime = gametime.elapsedTime;
                // Cập nhật thời điểm Poison Food xuất hiện để tính thời gian tồn tại 6s
                poisonStTime = gametime.elapsedTime;
            }
        }
    } 
    // Nếu là Level Easy, hàm này sẽ không làm gì cả.
}

// Kiểm tra va chạm với các cạnh 
void Game::CheckCollisionWithEdges() {
    if(snake.body[0].x == cellCount || snake.body[0].x == -1) {
        GameOver();
    }
    if(snake.body[0].y == cellCount || snake.body[0].y == -1) {
        GameOver();
    }
}

// Xử lý khi Game Over
void Game::GameOver() {
    snake.Reset();
    
    // Dọn dẹp và reset cả hai loại Food
    delete normalFood; 
    if(poisonFood != nullptr) {
        delete poisonFood;
    }

    normalFood = new NormalFood(snake.body);
    poisonFood = nullptr; // Đảm bảo reset cả Poison Food
    isPoisonActive = false;
    
    running = false;
    score = 0;
}

// Kiểm tra va chạm với đuôi rắn 
void Game::CheckCollisionWithTail() {
    deque<Vector2> headlessBody = snake.body;
    headlessBody.pop_front(); 
    if(ElementInDeque(snake.body[0],headlessBody)) {
        GameOver();
    }
}

class TwoPlayerGame : public Game {
public:
    Player1 p1;
    Player2 p2;

    TwoPlayerGame(Level* lvl) : Game(lvl) { 
        // Khi tạo TwoPlayerGame, cần dọn dẹp Food cũ của lớp Game (dùng cho 1P)
        delete normalFood;
        // Tạo Food mới, phải tránh body của cả P1 và P2
        deque<Vector2> combinedBody = CombineSnakeBodies(p1.body, p2.body);
        normalFood = new NormalFood(combinedBody);
        poisonFood = nullptr;
    }

    void HandleInput() {
        // Player 1 - WASD
        if (IsKeyPressed(KEY_W) && p1.direction.y != 1) p1.direction = {0, -1};
        if (IsKeyPressed(KEY_S) && p1.direction.y != -1) p1.direction = {0, 1};
        if (IsKeyPressed(KEY_A) && p1.direction.x != 1) p1.direction = {-1, 0};
        if (IsKeyPressed(KEY_D) && p1.direction.x != -1) p1.direction = {1, 0};

        // Player 2 - Mũi tên
        if (IsKeyPressed(KEY_UP) && p2.direction.y != 1) p2.direction = {0, -1};
        if (IsKeyPressed(KEY_DOWN) && p2.direction.y != -1) p2.direction = {0, 1};
        if (IsKeyPressed(KEY_LEFT) && p2.direction.x != 1) p2.direction = {-1, 0};
        if (IsKeyPressed(KEY_RIGHT) && p2.direction.x != -1) p2.direction = {1, 0};
    }

    void Update() override {
        if (running) {
            gametime.Update();
            CheckPoisonSpawn(); // Kiểm tra tạo/xóa Poison Food (vẫn dùng logic của Game)

            p1.Update();
            p2.Update();

            // Lấy combined body để tạo Food mới
            deque<Vector2> combinedBody = CombineSnakeBodies(p1.body, p2.body);

            // Kiểm tra va chạm với Normal Food cho cả hai
            if (Vector2Equals(p1.body[0], normalFood->position)) {
                normalFood->OnConsumed(*this, p1); // **Sửa: Dùng P1**
                delete normalFood;
                normalFood = new NormalFood(combinedBody); // Tạo mới, tránh cả 2 rắn
            } else if (Vector2Equals(p2.body[0], normalFood->position)) { // Dùng else if để tránh 2 con ăn 1 lúc
                normalFood->OnConsumed(*this, p2); // **Sửa: Dùng P2**
                delete normalFood;
                normalFood = new NormalFood(combinedBody); // Tạo mới, tránh cả 2 rắn
            }

            // Kiểm tra va chạm với Poison Food cho cả hai
            if (isPoisonActive) {
                if (Vector2Equals(p1.body[0], poisonFood->position)) {
                    poisonFood->OnConsumed(*this, p1); // **Sửa: Dùng P1**
                    delete poisonFood;
                    poisonFood = nullptr;
                    isPoisonActive = false;
                } else if (Vector2Equals(p2.body[0], poisonFood->position)) {
                    poisonFood->OnConsumed(*this, p2); // **Sửa: Dùng P2**
                    delete poisonFood;
                    poisonFood = nullptr;
                    isPoisonActive = false;
                }
            }

            // KIỂM TRA VA CHẠM CẠNH
            CheckSnakeCollisionWithEdges(p1);
            CheckSnakeCollisionWithEdges(p2);

            // KIỂM TRA VA CHẠM ĐUÔI
            CheckSnakeCollisionWithTail(p1);
            CheckSnakeCollisionWithTail(p2);

            // KIỂM TRA VA CHẠM GIỮA HAI RẮN (LOGIC MỚI QUAN TRỌNG)
            CheckSnakeVsSnakeCollision(p1, p2);
            CheckSnakeVsSnakeCollision(p2, p1);
        }
    }

    // Helper: Kiểm tra va chạm với cạnh cho một con rắn
    void CheckSnakeCollisionWithEdges(const Snake& s) {
        if (s.body[0].x == cellCount || s.body[0].x == -1 || 
            s.body[0].y == cellCount || s.body[0].y == -1) {
            GameOver(); // Nếu 1 trong 2 đụng, Game Over
        }
    }

    // Helper: Kiểm tra va chạm với đuôi cho một con rắn
    void CheckSnakeCollisionWithTail(const Snake& s) {
        deque<Vector2> headlessBody = s.body;
        if (headlessBody.size() > 1) { // Đảm bảo có đuôi để kiểm tra
             headlessBody.pop_front(); 
            if(ElementInDeque(s.body[0],headlessBody)) {
                GameOver();
            }
        }
    }

    // Helper: Kiểm tra rắn A va chạm với rắn B (đầu A va chạm với thân hoặc đầu B)
    void CheckSnakeVsSnakeCollision(const Snake& snakeA, const Snake& snakeB) {
        if (ElementInDeque(snakeA.body[0], snakeB.body)) {
            GameOver(); // Nếu đầu A va chạm với bất kỳ phần nào của B
        }
    }

    void Draw() override {
        normalFood->Draw();
        if (isPoisonActive && poisonFood != nullptr)
            poisonFood->Draw();

        p1.Draw(); // Vẽ rắn P1
        p2.Draw(); // Vẽ rắn P2
    }

    void GameOver() override {
        p1.Reset();
        p2.Reset();
        running = false;
        score = 0;

        // Reset Food mới phải tránh body của cả P1 và P2
        delete normalFood;
        if (poisonFood != nullptr) delete poisonFood;
        deque<Vector2> combinedBody = CombineSnakeBodies(p1.body, p2.body);
        normalFood = new NormalFood(combinedBody);
        poisonFood = nullptr;
        isPoisonActive = false;
    }
};


// ----------------- HÀM MAIN -----------------
int main() {
    InitWindow(2*offset + cellSize * cellCount,2 * offset + cellSize * cellCount, "BTL_OOP");
    SetTargetFPS(60);

    Level* chosenLevel = nullptr;
    Game* game = nullptr;

    // -------- MENU chọn level --------
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
            chosenLevel = new LevelEasy();     // Two player mode mặc định là easy
            game = new TwoPlayerGame(chosenLevel);
        }
    }

    if (chosenLevel == nullptr) { CloseWindow(); return 0; } 

    // -------- GAME LOOP --------
    while (!WindowShouldClose()) {
        BeginDrawing();

        // Cập nhật game theo tốc độ của level (easy: 0.25s, hard: 0.1s)
        if (eventTriggered(game->level->interval)) {
            game->Update();
        }

        // Xử lý Input (Điều khiển rắn)
        if (dynamic_cast<TwoPlayerGame*>(game) == nullptr) {
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
            // Nếu là 2 người, gọi hàm điều khiển riêng
            ((TwoPlayerGame*)game)->HandleInput();
        }

        // Vẽ màn hình
        ClearBackground(green);
        // Vẽ viền ngoài khu vực chơi
        DrawRectangleLinesEx(Rectangle{(float)offset - 5,(float)offset-5,(float)cellSize * cellCount + 10,(float)cellSize * cellCount + 10},5,darkGreen);
        DrawText("OOP-Nhom-2",offset - 5,20,40,darkGreen);
        // Hiển thị điểm số
        DrawText(TextFormat("%i",game->score),offset - 5,offset + cellSize*cellCount + 10,40,darkGreen);
        game->Draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}



