#include <windows.h>
#include <map>
#include <utility>
#include <vector>
#include <functional>
#include <string>
#include <iostream>
HWND hwnd;
std::map<int, std::function<void(WPARAM)>> messages;
class window{
	PAINTSTRUCT ps;
	int squareSize = 15;
	public:
	int getSquareSize(){
		return squareSize;
	}
	PAINTSTRUCT* getPaintStruct(){
		return &ps;
	}
	std::pair<int,int> getSize2(){
		RECT rect;
		GetClientRect(hwnd, &rect);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		return std::make_pair(width/2,height/2);
	}
};
struct tetromino{
	HBRUSH color;
	std::vector<std::vector<bool>> shape;
	tetromino(HBRUSH colorBrush, const std::vector<std::vector<bool>>& shape)
        : color(colorBrush), shape(shape) {}
	virtual void rotate(){
		std::vector<std::vector<bool>> rotated(shape[0].size(), std::vector<bool>(shape.size()));
        for (int i = 0; i < shape.size(); ++i) {
            for (int j = 0; j < shape[i].size(); ++j) {
                rotated[j][shape.size() - 1 - i] = shape[i][j];
            }
        }
        shape = std::move(rotated);
	}
	virtual std::pair<int,int> getSize(){
		return std::make_pair(shape.size(),shape[0].size());
	}
};
struct IShape : tetromino {
    IShape() : tetromino(CreateSolidBrush(RGB(0, 100, 100)), {{1, 1, 1, 1}}) {}
};
struct LShape : tetromino {
    LShape() : tetromino(CreateSolidBrush(RGB(0, 0, 255)), {{1, 0, 0}, {1, 1, 1}}) {}
};
struct JShape : tetromino {
    JShape() : tetromino(CreateSolidBrush(RGB(255, 165, 0)), {{0, 0, 1}, {1, 1, 1}}) {}
};
struct OShape : tetromino {
    OShape() : tetromino(CreateSolidBrush(RGB(255, 255, 0)), {{1, 1}, {1, 1}}) {}
	void rotate() override {}
};
struct ZShape : tetromino {
    ZShape() : tetromino(CreateSolidBrush(RGB(0, 255, 0)), {{0, 1, 1}, {1, 1, 0}}) {}
};
struct TShape : tetromino {
    TShape() : tetromino(CreateSolidBrush(RGB(128, 0, 128)), {{0, 1, 0}, {1, 1, 1}}) {}
};
struct SShape : tetromino {
    SShape() : tetromino(CreateSolidBrush(RGB(255, 0, 0)), {{1, 1, 0}, {0, 1, 1}}) {}
};
struct tetrominoList{
	std::vector<std::function<tetromino*()>> tetrominos;
    tetrominoList(){
		tetrominos.push_back([]() { return new IShape(); });
		tetrominos.push_back([]() { return new LShape(); });
		tetrominos.push_back([]() { return new JShape(); });
		tetrominos.push_back([]() { return new OShape(); });
		tetrominos.push_back([]() { return new ZShape(); });
		tetrominos.push_back([]() { return new TShape(); });
		tetrominos.push_back([]() { return new SShape(); });
	}
	tetromino* pick(){
		return tetrominos[rand() % tetrominos.size()]();
	}
};
class gameState;
class slowGame;
class fastGame;
class game{
	gameState* state;
	int score = 0;
	std::pair<int,int> gridStart;
	tetromino* activeTetromino = nullptr;
	std::pair<int,int> tetrominoPos = std::make_pair(4,0);
	tetrominoList TetrominoList;
	public:
	HBRUSH background = CreateSolidBrush(RGB(200, 200, 200));
	std::vector<std::vector<HBRUSH>> grid;
	std::pair<int,int> getSize(int squareSize = 1,int divisor = 1){
		return std::make_pair((grid.size()/divisor)*squareSize,(grid[0].size()/divisor)*squareSize);
	}
	HBRUSH getBackground(){
		return background;
	}
	void setState(gameState*);
	game();
	void toggleState();
	void tetrominoRespawned();
	void updateGridStart(window Window){
		auto [windowWidth, windowHeight] = Window.getSize2();
		auto [gridWidth, gridHeight] = getSize(Window.getSquareSize(),2);
		int startX = (windowWidth-gridWidth);
		int startY = (windowHeight-gridHeight);
		gridStart = std::make_pair(startX,startY);
	}
	void tetrominoDebris(){
		auto [Width, Height] = activeTetromino->getSize();
		auto [X, Y] = tetrominoPos;
		Y-=1;
		for(int x = 0; x<Width; x++) {
			for(int y = 0; y<Height; y++) {
				if(activeTetromino->shape[x][y]==0)continue;
				grid[x+X][y+Y] = activeTetromino->color;
			}
		}
	}
	bool checkTile(int x, int y){
		auto [gridWidth, gridHeight] = getSize();
		if(x<0||x>=gridWidth)return 1;
		if(y<0||y>=gridHeight)return 1;
		return (grid[x][y]!=getBackground());
	}
	bool checkOverlap(){
		auto [Width, Height] = activeTetromino->getSize();
		auto [X, Y] = tetrominoPos;
		for(int x = 0; x<Width; x++) {
			for(int y = 0; y<Height; y++) {
				if(activeTetromino->shape[x][y]==0)continue;
				if(checkTile(x+X,y+Y))return 1;
			}
		}return 0;
	}
	void rotateTetramino(){
		if(activeTetromino == nullptr)return;
		activeTetromino->rotate();
		if(checkOverlap()){
			for(int i = 0;i<3;i++)activeTetromino->rotate();
		}
	}
	void moveTetramino(int x,int y){
		auto [X, Y] = tetrominoPos;
		auto oldPos = tetrominoPos;
		tetrominoPos = std::make_pair(X+x,Y+y);
		if(checkOverlap()){
			tetrominoPos = oldPos;
		}
	}
	void spawnTetromino(bool debris = 0){
		if(activeTetromino!=nullptr){
			if(debris){
				tetrominoDebris();
			}
			tetrominoPos = std::make_pair(4,0);
			delete(activeTetromino);
		}
		activeTetromino = TetrominoList.pick();
		tetrominoRespawned();
		if(checkOverlap()){
			MessageBox(NULL, ("Jus pralaimejote, jusu taskai: "+std::to_string(score)).c_str(),"Zaidimas baigtas",MB_ICONEXCLAMATION|MB_OK);
			exit(0);
		}
	}
	void drawBackground(HDC hdc,window Window){
		updateGridStart(Window);
		auto [windowWidth, windowHeight] = Window.getSize2();
		auto [gridWidth, gridHeight] = getSize(Window.getSquareSize(),2);
		auto [startX, startY] = gridStart;
		int endX = (windowWidth+gridWidth);
		int endY = (windowHeight+gridHeight);
		SelectObject(hdc, getBackground());
		Rectangle(hdc,startX,startY,endX,endY);
		SetTextColor(hdc, RGB(0, 0, 0));
		std::string scoreText = ("Score: "+std::to_string(score));
        TextOut(hdc, 0, 0, scoreText.c_str(), scoreText.size());
	}
	void drawTiles(HDC hdc,window Window){
		auto [gridWidth, gridHeight] = getSize();
		auto [startX, startY] = gridStart;
		int squareSize = Window.getSquareSize();
		for(int x = 0; x<gridWidth; x++) {
			for(int y = 0; y<gridHeight; y++) {
				if(grid[x][y] == background)continue;
				SelectObject(hdc, grid[x][y]);
				Rectangle(hdc, startX+((x)*squareSize), startY+((y)*squareSize),
				          startX+((x+1)*squareSize), startY+((y+1)*squareSize));
			}
		}
	}
	void drawTetromino(HDC hdc,window Window){
		if(activeTetromino == nullptr)return;
		auto [Width, Height] = activeTetromino->getSize();
		auto [startX, startY] = gridStart;
		auto [X, Y] = tetrominoPos;
		int squareSize = Window.getSquareSize();
		SelectObject(hdc, activeTetromino->color);
		for(int x = 0; x<Width; x++) {
			for(int y = 0; y<Height; y++) {
				if(activeTetromino->shape[x][y]==0)continue;
				Rectangle(hdc, startX+((x+X)*squareSize), startY+((y+Y)*squareSize),
				          startX+((x+X+1)*squareSize), startY+((y+Y+1)*squareSize));
			}
		}
	}
	void clearRows(){
		auto [Width, Height] = getSize();
		for(int y = Height-1; y>=0; y--) {
			bool fullRow = true;
			for(int x = 0; x<Width; x++)if(grid[x][y]==getBackground())fullRow = false;
			if(fullRow==false)continue;
			for(int yy = y;yy>0;yy--){
				for(int xx = 0;xx<Width;xx++){
					grid[xx][yy] = grid[xx][yy-1];
				}
			}
			score+=100;
		}
	}
	void update(int);
};
class gameState{
	public:
	virtual bool dropTick(int tick) = 0;
	virtual void changeState(game* Game) = 0;
	virtual void slow(game* Game) = 0;
};
class fastGame;
class slowGame : public gameState{
	bool dropTick(int tick) override {
		if(tick%7 == 0)return true;
		return false;
	}
	void changeState(game* Game) override;
	void slow(game* Game) override {}
};
class fastGame : public gameState{
	bool dropTick(int tick) override {
		return true;
	}
	void changeState(game* Game) override;
	void slow(game* Game) override {
		changeState(Game);
	}
};
game::game(){
	state = new slowGame();
}
void game::toggleState(){
	state->changeState(this);
}
void game::tetrominoRespawned(){
	state->slow(this);
}
void game::setState(gameState* newState){
	delete state;
	state = newState;
}
void game::update(int tick){
	if(activeTetromino == nullptr)spawnTetromino();
	auto [X, Y] = tetrominoPos;
	if(state->dropTick(tick))tetrominoPos = std::make_pair(X,Y+1);
	if(checkOverlap()){
		spawnTetromino(1);
	}
	clearRows();
}
void fastGame::changeState(game* Game){
	Game->setState(new slowGame());
}
void slowGame::changeState(game* Game){
	Game->setState(new fastGame());
}
class gameBuilder{
	game Game;
	public:
	gameBuilder gridSize(int x,int y){
		Game.grid = std::vector<std::vector<HBRUSH>>(x, std::vector<HBRUSH>(y,Game.background));
		return *this;
	}
	gameBuilder backgroundColor(int r,int g,int b){
		auto [width, height] = Game.getSize();
		Game.background = CreateSolidBrush(RGB(r, g, b));
		Game.grid = std::vector<std::vector<HBRUSH>>(width, std::vector<HBRUSH>(height,Game.background));
		return *this;
	}
	game build(){
		return Game;
	}
};
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	int response = MessageBox(NULL, "Ar norite pradeti zaidima?", "Tetris", MB_YESNO | MB_ICONQUESTION);
    if (response == IDNO)exit(0);
	
	WNDCLASS wc = {0};
	MSG msg;
    wc.lpfnWndProc = [](HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) -> LRESULT CALLBACK {
		if(messages.find(Message)!=messages.end())messages[Message](wParam);
		return DefWindowProc(hwnd, Message, wParam, lParam);
	};
    wc.hInstance = hInstance;
    wc.lpszClassName = "WindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	RegisterClass(&wc);
	hwnd = CreateWindowEx(0, "WindowClass", "Tetris", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,640,480, 0, 0, hInstance, 0);
	messages[WM_SIZE] = [](WPARAM wParam){InvalidateRect(hwnd, NULL, TRUE);};
	messages[WM_DESTROY] = [](WPARAM wParam){exit(0);};
	window Window;
	gameBuilder GameBuilder;
	game Game = GameBuilder.gridSize(10,20).backgroundColor(200,200,200).build();
	std::srand(GetTickCount());
	messages[WM_PAINT] = [&Window,&Game](WPARAM wParam){HDC hdc = BeginPaint(hwnd, Window.getPaintStruct());
	
		Game.drawBackground(hdc,Window);
		Game.drawTiles(hdc,Window);
		Game.drawTetromino(hdc,Window);
		
		EndPaint(hwnd, Window.getPaintStruct());};
	messages[WM_KEYDOWN] = [&Game](WPARAM wParam){
		if(wParam == VK_UP)Game.rotateTetramino();
		else if(wParam == VK_UP)Game.rotateTetramino();
		else if(wParam == VK_LEFT)Game.moveTetramino(-1,0);
		else if(wParam == VK_RIGHT)Game.moveTetramino(1,0);
		else if(wParam == VK_DOWN)Game.toggleState();
	};
	int ticks = GetTickCount();
	while(1){Sleep(35);
		MSG Message;
		ticks++;
        while(PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
		InvalidateRect(hwnd, NULL, TRUE);
		Game.update(ticks);
	}
	return msg.wParam;
}