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
    void Draw() {
        for (unsigned int i = 0; i < body.size(); i++) {
            float x = body[i].x;
            float y = body[i].y;
            // Tính toán vị trí pixel từ tọa độ lưới và vẽ
            Rectangle segment = Rectangle{offset + x * cellSize,offset + y * cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(segment, 0.5, 6, darkGreen);
        }
    }

    // Cập nhật vị trí rắn (di chuyển)
    void Update() {
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
    void Reset() {
        body = {Vector2{6,9}, Vector2{5,9},Vector2{4,9}};
        direction = {1, 0};
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
    void Draw(); 
    void Update(); 
    void CheckCollisionWithFood(); 
    void CheckCollisionWithEdges(); 
    void GameOver(); 
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
    virtual void OnConsumed(Game& game) {
        game.snake.addSegment = true; // Thân rắn dài ra
        game.score++; // Tăng 1 điểm
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
    void OnConsumed(Game& game) override {
        game.score = max(0,game.score - 2); // Giảm 2 điểm (dùng max để không bị âm)
        
        // Giảm độ dài rắn (nếu rắn đủ dài)
        if(game.snake.body.size() > 3) {
            game.snake.body.pop_back(); // Rắn ngắn lại 1 đốt
        } else {
            game.GameOver(); // Nếu rắn quá ngắn, kết thúc game
        }
        game.snake.addSegment = false; // Đảm bảo rắn không dài ra
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
        normalFood->OnConsumed(*this);
        delete normalFood;
        // Tạo Normal Food mới ngay lập tức
        normalFood = new NormalFood(snake.body);
    }
    
    // Check va chạm với Poison Food
    if(isPoisonActive && Vector2Equals(snake.body[0],poisonFood->position)) {
        poisonFood->OnConsumed(*this);
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


// ----------------- HÀM MAIN -----------------
int main() {
    InitWindow(2*offset + cellSize * cellCount,2 * offset + cellSize * cellCount, "BTL_OOP");
    SetTargetFPS(60);

    Level* chosenLevel = nullptr;

    // -------- MENU chọn level --------
    while (!WindowShouldClose() && chosenLevel == nullptr) {
        BeginDrawing();
        ClearBackground(green);
        DrawText("CHOOSE LEVEL", 260, 220, 50, darkGreen);
        DrawText("Press 1 for EASY",280, 350, 35, darkGreen);
        DrawText("Press 2 for HARD", 280, 400, 35, darkGreen);
        EndDrawing();
        if (IsKeyPressed(KEY_ONE)) chosenLevel = new LevelEasy();
        if (IsKeyPressed(KEY_TWO)) chosenLevel = new LevelHard();
    }

    if (chosenLevel == nullptr) { CloseWindow(); return 0; } 
    Game game(chosenLevel); // Khởi tạo Game với cấp độ đã chọn

    // -------- GAME LOOP --------
    while (!WindowShouldClose()) {
        BeginDrawing();

        // Cập nhật game theo tốc độ của level (easy: 0.25s, hard: 0.1s)
        if (eventTriggered(game.level->interval)) {
            game.Update();
        }

        // Xử lý Input (Điều khiển rắn)
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

        // Vẽ màn hình
        ClearBackground(green);
        // Vẽ viền ngoài khu vực chơi
        DrawRectangleLinesEx(Rectangle{(float)offset - 5,(float)offset-5,(float)cellSize * cellCount + 10,(float)cellSize * cellCount + 10},5,darkGreen);
        DrawText("OOP-Nhom-2",offset - 5,20,40,darkGreen);
        // Hiển thị điểm số
        DrawText(TextFormat("%i",game.score),offset - 5,offset + cellSize*cellCount + 10,40,darkGreen);
        game.Draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}



