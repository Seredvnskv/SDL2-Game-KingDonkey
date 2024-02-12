#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

const int BARREL_SPAWN_INTERVAL = 2000;  // odstep czasowy pomiedzy beczkami
const int MAX_BARRELS = 5; // maksymalna liczba beczkek na ekranie
const int SCREEN_WIDTH = 740; // szerokosc okna gry 
const int SCREEN_HEIGHT = 600; // wysokosc okna gry
const int PLATFORM_HEIGHTS[] = { SCREEN_HEIGHT - 70, SCREEN_HEIGHT - 140, SCREEN_HEIGHT - 210, SCREEN_HEIGHT - 280, SCREEN_HEIGHT - 350, SCREEN_HEIGHT - 420 }; // wysokosci platform, zaczynajac od dolnej czesci ekranu
const int NUMBER_OF_LADDERS = 5; // liczba drabin 
const int LADDER_SEGMENTS = 6; // liczba segmentow drabiny
const int PLATFORM_COUNT = 6; // liczba platform
const int SPACE_WIDTH = 80; // szerokosc przerwy
const int GRAVITY = 1; // grawitacja
const int STAGE_COMPLETION_POINTS = 1000; // liczba punktow za ukonczenie poziomu
const int FPS = 60; // ograniczenie liczby klatek na sekunde

enum Stage {
	STAGE_1,
	STAGE_2,
	STAGE_3
};

typedef struct {
	SDL_Rect rect; // pozycja i rozmiar postaci
	int vx, vy; // predkosc
	int isJumping; // stan skoku 0 - nie, 1 - tak
	int jumpPower; // moc skoku
	int canClimbLadder;
	int ladderClimbCooldown;
	int score;
} Character;

typedef struct {
	SDL_Rect rect; // pozycja i rozmiar
	int vx; // predkosc x
	int vy; // predkosc y
	int isFalling; // spadnie
} Barrel;

typedef struct {
	Barrel barrels[MAX_BARRELS] = {}; //beczki
	SDL_Rect platforms[PLATFORM_COUNT] = {
	{0, PLATFORM_HEIGHTS[0], SCREEN_WIDTH - SPACE_WIDTH, 12}, // Platforma 1 itd...
	{SPACE_WIDTH, PLATFORM_HEIGHTS[1], SCREEN_WIDTH, 12},
	{0, PLATFORM_HEIGHTS[2], SCREEN_WIDTH - SPACE_WIDTH, 12},
	{SPACE_WIDTH, PLATFORM_HEIGHTS[3], SCREEN_WIDTH, 12},
	{0, PLATFORM_HEIGHTS[4], SCREEN_WIDTH - SPACE_WIDTH, 12},
	{SPACE_WIDTH, PLATFORM_HEIGHTS[5], SCREEN_WIDTH, 12},
	};
	SDL_Rect ladders[NUMBER_OF_LADDERS] = {};//drabiny
	Stage currentStage = STAGE_1; //poczatkowy poziom
	SDL_Rect item;
} Game;

int main(int argc, char** argv);
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset);
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y);
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color);
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color);
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor);
void DrawPlatform(SDL_Surface* screen, SDL_Surface* platform, int y, int cutSide);
void DrawLadder(SDL_Surface* screen, SDL_Surface* ladder, int x, int y, int height);
void DrawFirstLevel(SDL_Surface* screen, SDL_Surface* platform, SDL_Surface* ladder, Game* game);
int InitializeSDL(SDL_Window** window, SDL_Renderer** renderer, SDL_Surface** screen, SDL_Surface** charset, SDL_Surface** platform, SDL_Surface** platform2, SDL_Surface** platform3, SDL_Surface** ladder, SDL_Surface** robot, SDL_Surface** barrel, SDL_Surface** stageItem, SDL_Texture** scrtex);
void HandleEvents(int* quit, Character* player, double* worldTime, int* onLadder, int* ladderDirection, int* t1, Game* game);
void Cleanup(SDL_Surface* charset, SDL_Surface* screen, SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer);
int InitializeGame(Character* player, Game* game, SDL_Surface** screen, SDL_Surface** charset, SDL_Surface** platform, SDL_Surface** platform2, SDL_Surface** platform3, SDL_Surface** ladder, SDL_Surface** robot, SDL_Surface** barrel, SDL_Surface** stageItem, SDL_Texture** scrtex, SDL_Window** window, SDL_Renderer** renderer);
void UpdateGameState(Character* player, Game* game, int* onLadder, int ladderDirection);
void RenderGame(SDL_Surface* screen, SDL_Surface* charset, Game* game, SDL_Surface* platform, SDL_Surface* platform2, SDL_Surface* platform3, SDL_Surface* ladder, SDL_Surface* robot, SDL_Surface* barrel, SDL_Surface* stageItem, SDL_Texture* scrtex, SDL_Renderer* renderer, Character* player, double* worldTime, double* fps);
int CheckCollision(SDL_Rect* rect1, SDL_Rect* rect2);
void InitPlayer(Character* player, SDL_Surface* robot);
void InitBarrels(SDL_Surface* barrel, Game* game, int time);
void InitItem(Game* game, SDL_Surface* stageItem);
void UpdatePlayer(Character* player, Game* game, int* onLadder, int ladderDirection);
void isBarrelFalling(Game* game);
void SwitchStage(Game* game, Character* player, int newStage);
void HandleItemCollection(Game* game, Character* player);
void ResetPlayerAndTime(int* quit, Character* player, double* worldTime, int* onLadder, int* ladderDirection);
void ResetBarrels(Game* game, int* t1);
void EndGame(Character* player);

// narysowanie napisu txt na powierzchni screen, zaczynajπc od punktu (x, y)
// charset to bitmapa 128x128 zawierajπca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};
// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt úrodka obrazka sprite na ekranie
// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};
// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};
// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1) 
// bπdü poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};
// rysowanie prostokπta o d≥ugoúci bokÛw l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

void DrawPlatform(SDL_Surface* screen, SDL_Surface* platform, int y, int cutSide) {
	int length = SCREEN_WIDTH / platform->w;
	int cutLength = SPACE_WIDTH / platform->w;
	int start = (cutSide == 0) ? cutLength : 0; 
	int end = (cutSide == 0) ? length : length - cutLength;

	for (int i = start; i <= end; i++) {
		DrawSurface(screen, platform, i * platform->w + platform->w / 2, y);
	}
}

void DrawLadder(SDL_Surface* screen, SDL_Surface* ladder, int x, int y, int height) {
	for (int i = 0; i < height; i++) {
		DrawSurface(screen, ladder, x, y - i * ladder->h);
	}
}

void DrawFirstLevel(SDL_Surface* screen, SDL_Surface* platform, SDL_Surface* ladder, Game* game) {
	int cutSides[] = { 1, 0, 1, 0, 1, 0 }; // wciecie z prawej 1, 0 z lewej 
	
	for (int i = 0; i < PLATFORM_COUNT; i++) {
		DrawPlatform(screen, platform, game->platforms[i].y, cutSides[i]);
	}
	for (int i = 0; i < NUMBER_OF_LADDERS; i++) {
		int y = PLATFORM_HEIGHTS[i];
		int height = ladder->h * LADDER_SEGMENTS;
		int ladderX;
		if (cutSides[i] == 0) {
			ladderX = 100;
		}
		else {
			ladderX = 650;
		}

		DrawLadder(screen, ladder, ladderX, y - ladder->h / 2, LADDER_SEGMENTS);
		game->ladders[i] = { ladderX, y, ladder->w, height };
	}
}

void DrawSecondLevel(SDL_Surface* screen, SDL_Surface* platform2, SDL_Surface* ladder, Game* game) {
	int cutSides[] = { 1, 0, 1, 0, 1, 0 };

	for (int i = 0; i < PLATFORM_COUNT; i++) {
		DrawPlatform(screen, platform2, game->platforms[i].y, cutSides[i]);
	}

	for (int i = 0; i < NUMBER_OF_LADDERS; i++) {
		int y = PLATFORM_HEIGHTS[i];
		int height = ladder->h * LADDER_SEGMENTS;
		int ladderX;
		if (cutSides[i] == 0) {
			ladderX = 400;
		}
		else {
			ladderX = 500;
		}
		DrawLadder(screen, ladder, ladderX, y - ladder->h / 2, LADDER_SEGMENTS);
		game->ladders[i] = { ladderX, y, ladder->w, height };
	}
}

void DrawThirdLevel(SDL_Surface* screen, SDL_Surface* platform3, SDL_Surface* ladder, Game* game) {
	int cutSides[] = { 1, 0, 1, 0, 1, 0 };

	for (int i = 0; i < PLATFORM_COUNT; i++) {
		DrawPlatform(screen, platform3, game->platforms[i].y, cutSides[i]);
	}

	for (int i = 0; i < NUMBER_OF_LADDERS; i++) {
		int y = PLATFORM_HEIGHTS[i];
		int height = ladder->h * LADDER_SEGMENTS;
		int ladderX;
		if (cutSides[i] == 0) {
			ladderX = 200;
		}
		else {
			ladderX = 400;
		}
		DrawLadder(screen, ladder, ladderX, y - ladder->h / 2, LADDER_SEGMENTS);
		game->ladders[i] = { ladderX, y, ladder->w, height };
	}
}

int InitializeSDL(SDL_Window** window, SDL_Renderer** renderer, SDL_Surface** screen, SDL_Surface** charset, SDL_Surface** platform, SDL_Surface** platform2, SDL_Surface** platform3, SDL_Surface** ladder, SDL_Surface** robot, SDL_Surface** barrel, SDL_Surface** stageItem, SDL_Texture** scrtex) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, window, renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(*renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(*renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(*window, "KING DONKEY 198035");
	SDL_ShowCursor(SDL_DISABLE);

	*screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	*scrtex = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	*charset = SDL_LoadBMP("./cs8x8.bmp");
	if (*charset == NULL) {
		Cleanup(*charset, *screen, *scrtex, *window, *renderer);
		return 1;
	}
	SDL_SetColorKey(*charset, true, 0x000000);

	*platform = SDL_LoadBMP("./assets/platform.bmp");
	if (*platform == NULL) {
		printf("SDL_LoadBMP(/assets/platform.bmp) error: %s\n", SDL_GetError());
		Cleanup(*charset, *screen, *scrtex, *window, *renderer);
		return 1;
	};

	*platform2 = SDL_LoadBMP("./assets/platform2.bmp");
	if (*platform == NULL) {
		printf("SDL_LoadBMP(/assets/platform2.bmp) error: %s\n", SDL_GetError());
		Cleanup(*charset, *screen, *scrtex, *window, *renderer);
		return 1;
	};

	*platform3 = SDL_LoadBMP("./assets/platform3.bmp");
	if (*platform == NULL) {
		printf("SDL_LoadBMP(/assets/platform3.bmp) error: %s\n", SDL_GetError());
		Cleanup(*charset, *screen, *scrtex, *window, *renderer);
		return 1;
	};

	*ladder = SDL_LoadBMP("./assets/ladder.bmp");
	if (*ladder == NULL) {
		printf("SDL_LoadBMP(/assets/ladder.bmp) error: %s\n", SDL_GetError());
		Cleanup(*charset, *screen, *scrtex, *window, *renderer);
		return 1;
	};

	*robot = SDL_LoadBMP("./assets/robot.bmp");
	if (*robot == NULL) {
		printf("SDL_LoadBMP(/assets/robot.bmp) error: %s\n", SDL_GetError());
		Cleanup(*charset, *screen, *scrtex, *window, *renderer);
		return 1;
	};

	*barrel = SDL_LoadBMP("./assets/barrel.bmp");
	if (*barrel == NULL) {
		printf("SDL_LoadBMP(/assets/barrel.bmp) error: %s\n", SDL_GetError());
		Cleanup(*charset, *screen, *scrtex, *window, *renderer);
		return 1;
	};

	*stageItem = SDL_LoadBMP("./assets/item.bmp");
	if (*stageItem == NULL) {
		printf("SDL_LoadBMP(/assets/item.bmp) error: %s\n", SDL_GetError());
		Cleanup(*charset, *screen, *scrtex, *window, *renderer);
		return 1;
	};

	return 0;
}

void Cleanup(SDL_Surface* charset, SDL_Surface* screen, SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer) {
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void InitPlayer(Character* player, SDL_Surface* robot) {
	// inicjalizacja gracza
	player->rect.w = (robot)->w;
	player->rect.h = (robot)->h;
	player->rect.x = 0;
	player->rect.y = PLATFORM_HEIGHTS[0] - player->rect.h;
	player->vx = 0;
	player->vy = 0;
	player->score = 0;
}

void InitBarrels(SDL_Surface* barrel, Game* game, int time) {
	static int lastBarrelSpawnTime = 0;
	if (time - lastBarrelSpawnTime > BARREL_SPAWN_INTERVAL) { //  sprawdza czy minal odpowiedni czas od ostatniej beczki
		for (int i = 0; i < MAX_BARRELS; i++) {
			if (game->barrels[i].vx == 0) {
				game->barrels[i].rect.w = (barrel)->w;
				game->barrels[i].rect.h = (barrel)->h;
				game->barrels[i].rect.x = SCREEN_WIDTH + 50;
				game->barrels[i].rect.y = PLATFORM_HEIGHTS[PLATFORM_COUNT - 1] - game->barrels[i].rect.h; // pozycja beczki na najwyzszej platformie
				game->barrels[i].vx = -3;
				game->barrels[i].vy = 0;
				game->barrels[i].isFalling = 0; // beczki zaczynajπ niespadajπc
				lastBarrelSpawnTime = time; // akutalizacja czasu 
				break;
			}
		}
	}
}

void InitItem(Game* game, SDL_Surface* stageItem) {
	game->item.w = stageItem->w;
	game->item.h = stageItem->h;
	game->item.x = 600;
	game->item.y = 100;
}

int InitializeGame(Character* player,Game* game, SDL_Surface** screen, SDL_Surface** charset, SDL_Surface** platform, SDL_Surface** platform2, SDL_Surface** platform3, SDL_Surface** ladder, SDL_Surface** robot, SDL_Surface** barrel, SDL_Surface** stageItem, SDL_Texture** scrtex, SDL_Window** window, SDL_Renderer** renderer) {
	if (InitializeSDL(window, renderer, screen, charset, platform, platform2, platform3, ladder, robot, barrel, stageItem, scrtex) != 0) {
		return 1;
	}

	InitPlayer(player, *robot);
	InitItem(game, *barrel);

	return 0;
}

int CheckCollision(SDL_Rect* rect1, SDL_Rect* rect2) {
	if ((rect1->x + rect1->w < rect2->x) || (rect1->x > rect2->x + rect2->w) || (rect1->y + rect1->h < rect2->y) || (rect1->y > rect2->y + rect2->h)) {
		return 0; // brak kolizji
	}
	return 1; // kolizja 
}

void UpdateBarrelState(Barrel* barrel, SDL_Rect* platforms) {
	if (!barrel->isFalling) { //czy beczka spada
		int onPlatform = 0;
		SDL_Rect barrelBottom = { barrel->rect.x, barrel->rect.y + barrel->rect.h, barrel->rect.w, 1 }; // linija sprawdzajaca kolizje z platformami

		for (int i = 0; i < PLATFORM_COUNT; i++) {
			if (CheckCollision(&barrelBottom, &platforms[i])) { // czy dolna czesc beczki koliduje z platformπ
				
				onPlatform = 1;

				if (barrel->rect.x + barrel->rect.w <= platforms[i].x || barrel->rect.x >= platforms[i].x + platforms[i].w) { // czy beczka znajduje siÍ na krawedzi platformy
					
					barrel->isFalling = 1;
					barrel->vy = 2; 
					break;
				}
			}
		}

		if (!onPlatform) { // gdy beczka nie jest w ogole na platformie zaczyna spadac
			barrel->isFalling = 1;
			barrel->vy = 2;
		}
	}
	else {
		barrel->rect.y += barrel->vy; // aktualizacja pozycji 

		for (int i = 0; i < PLATFORM_COUNT; i++) {
			if (CheckCollision(&barrel->rect, &platforms[i])) {
				
				barrel->isFalling = 0;
				barrel->vy = 0;
				barrel->rect.y = platforms[i].y - barrel->rect.h; 
				barrel->vx *= -1; 
				break;
			}
		}
	}
}

void isBarrelFalling(Game* game) {
	for (int i = 0; i < MAX_BARRELS; i++) {
		if (game->barrels[i].vx != 0) { // sprawdzenie czy beczka jest w ruchu
			if (!game->barrels[i].isFalling) { // gdy beczka nie spada sprawdz czy powinna
				int barrelOnPlatform = 0;

				for (int j = 0; j < PLATFORM_COUNT; j++) {
					if (CheckCollision(&game->barrels[i].rect, &game->platforms[j])) {  // sprawdzenie kolizj beczki i platformy
						if (game->barrels[i].rect.y + game->barrels[i].rect.h + game->barrels[i].vy >= game->platforms[j].y) { // gdy beczka jest na krawedzi platformy zacznij spadac
							game->barrels[i].vy = 0; 
							game->barrels[i].rect.y = game->platforms[j].y - game->barrels[i].rect.h; // ustawienie beczki na platformie
							game->barrels[i].isFalling = 0; // beczka juz nie spada
							barrelOnPlatform = 1; // beczka na platformie
							break; 
						}
					}
				}
				
				if (!barrelOnPlatform) { // gdy beczka nie ma na platformie, symulacja grawtiacji
					game->barrels[i].vy += GRAVITY; 
				}
			}

			//  polozenie beczki w oparciu o predkosc
			game->barrels[i].rect.x += game->barrels[i].vx;
			game->barrels[i].rect.y += game->barrels[i].vy;

			if (game->barrels[i].rect.x < 0 - game->barrels[i].rect.w || game->barrels[i].rect.y > SCREEN_HEIGHT) { // gdy beczka wypadnie poza ekran zatrzymujemy jej ruch
				game->barrels[i].vx = 0;
				game->barrels[i].vy = 0;
				game->barrels[i].isFalling = 0;
			}
		}
	}
	for (int i = 0; i < MAX_BARRELS; i++) {
		if (game->barrels[i].vx != 0) {
			UpdateBarrelState(&game->barrels[i], game->platforms); // gdy beczka jest w ruchu zaktualizuj jej stan.
		}
	}
}

void UpdatePlayer(Character* player, Game* game, int* onLadder, int ladderDirection) {
	int onPlatform = 0;
	*onLadder = 0;

	// opoznienie po skoku
	if (player->isJumping) {
		player->canClimbLadder = 0;
		player->ladderClimbCooldown = 30;
	}
	else if (player->ladderClimbCooldown > 0) {
		player->ladderClimbCooldown--;
		if (player->ladderClimbCooldown == 0) {
			player->canClimbLadder = 1;
		}
	}

	// sprawdzanie kolizji z platformami
	for (int i = 0; i < PLATFORM_COUNT; i++) {
		if (player->rect.x + player->rect.w > game->platforms[i].x &&
			player->rect.x < game->platforms[i].x + game->platforms[i].w &&
			player->rect.y + player->rect.h <= game->platforms[i].y &&
			player->rect.y + player->rect.h + player->vy >= game->platforms[i].y) {

			if (!(*onLadder && ladderDirection == 1)) {
				player->vy = 0;
				player->rect.y = game->platforms[i].y - player->rect.h;
				onPlatform = 1;
				break;
			}
		}
	}

	// sprawdzanie czy postac jest na drabinie 
	for (int i = 0; i < NUMBER_OF_LADDERS; i++) {
		if (player->rect.x + player->rect.w > game->ladders[i].x &&
			player->rect.x < game->ladders[i].x + game->ladders[i].w &&
			player->rect.y < game->ladders[i].y &&
			player->rect.y + player->rect.h > game->ladders[i].y - game->ladders[i].h &&
			player->canClimbLadder) {

			*onLadder = 1;
			if (ladderDirection != 0) {
				player->vy = ladderDirection * 5;
			}
			break;
		}
	}

	// grawitacja jesli gracz nie jest na platformie ani na drabinie
	if (!onPlatform && !*onLadder) {
		player->vy += GRAVITY;
	}

	// aktualizacja pozycji gracza
	player->rect.x += player->vx;
	player->rect.y += player->vy;

	// ogranicznie ruchu gracza
	if (player->rect.x < 0) player->rect.x = 0;
	if (player->rect.x > SCREEN_WIDTH - player->rect.w) player->rect.x = SCREEN_WIDTH - player->rect.w;
	if (player->rect.y < 0) player->rect.y = 0;
	if (player->rect.y > SCREEN_HEIGHT - player->rect.h) {
		player->rect.y = SCREEN_HEIGHT - player->rect.h;
		player->vy = 0;
	}
	player->isJumping = 0;
	player->jumpPower = 12;
}

void UpdateGameState(Character* player, Game* game, int* onLadder, int ladderDirection) {
	UpdatePlayer(player, game, onLadder, ladderDirection);
	isBarrelFalling(game);

	if (CheckCollision(&player->rect, &game->item)) { // czy podniesieono przedmiot
		HandleItemCollection(game, player);
	}

	for (int i = 0; i < MAX_BARRELS; i++) { // kolizja gracza i beczki
		if (game->barrels[i].vx != 0) {
			if (CheckCollision(&player->rect, &game->barrels[i].rect)) {
				printf("You Died!, Try Again\n"); 
				EndGame(player); 
			}
		}
	}

}

void RenderGame(SDL_Surface* screen, SDL_Surface* charset, Game* game, SDL_Surface* platform, SDL_Surface* platform2, SDL_Surface* platform3, SDL_Surface* ladder, SDL_Surface* robot, SDL_Surface* barrel, SDL_Surface* stageItem, SDL_Texture* scrtex, SDL_Renderer* renderer, Character* player, double* worldTime, double* fps) {
	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	SDL_FillRect(screen, NULL, czarny);

	DrawSurface(screen, stageItem, game->item.x + game->item.w / 2, game->item.y + game->item.h / 2);

	switch (game->currentStage) {
	case STAGE_1:
		DrawFirstLevel(screen, platform, ladder, game);
		for (int i = 0; i < MAX_BARRELS; i++) {
			DrawSurface(screen, barrel, game->barrels[i].rect.x, game->barrels[i].rect.y);
		}
		DrawSurface(screen, robot, player->rect.x + player->rect.w / 2, player->rect.y + player->rect.h / 2);
		break;
	case STAGE_2:
		DrawSecondLevel(screen, platform2, ladder, game);
		for (int i = 0; i < MAX_BARRELS; i++) {
			DrawSurface(screen, barrel, game->barrels[i].rect.x, game->barrels[i].rect.y);
		}
		DrawSurface(screen, robot, player->rect.x + player->rect.w / 2, player->rect.y + player->rect.h / 2);
		break;
	case STAGE_3:
		DrawThirdLevel(screen, platform3, ladder, game);
		for (int i = 0; i < MAX_BARRELS; i++) {
			DrawSurface(screen, barrel, game->barrels[i].rect.x, game->barrels[i].rect.y);
		}
		DrawSurface(screen, robot, player->rect.x + player->rect.w / 2, player->rect.y + player->rect.h / 2);
		break;
	}

	DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, czerwony, niebieski);
	sprintf(text, "Sambor King Donkey, Czas = %.1lf s  %.0lf klatek / s", *worldTime, *fps);
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
	sprintf(text, "Esc - wyjscie, n - nowa gra, 1 - lvl1, 2 - lvl2, 3 - lvl3");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);

	sprintf(text, "Score: %d", player->score);
	DrawString(screen, 4, 50, text, charset);

	// aktualizacja tekstury i renderowanie
	SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, scrtex, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void SwitchStage(Game* game,Character* player, int newStage) {
	switch (newStage) {
	case 1:
		game->currentStage = STAGE_1;
		player->rect.x = 0; 
		player->rect.y = PLATFORM_HEIGHTS[0] - player->rect.h;
		break;
	case 2:
		game->currentStage = STAGE_2;
		player->rect.x = 0;
		player->rect.y = PLATFORM_HEIGHTS[0] - player->rect.h;
		break;
	case 3:
		game->currentStage = STAGE_3;
		player->rect.x = 0;
		player->rect.y = PLATFORM_HEIGHTS[0] - player->rect.h;
		break;
	}
}

void HandleItemCollection(Game* game, Character* player) {
	switch (game->currentStage) {
	case STAGE_1:
		printf("Item collected on Stage 1! Advancing to Stage 2...\n");
		player->score += STAGE_COMPLETION_POINTS; 
		SwitchStage(game, player, 2);
		break;
	case STAGE_2:
		printf("Item collected on Stage 2! Advancing to Stage 3...\n");
		player->score += STAGE_COMPLETION_POINTS;
		SwitchStage(game, player, 3);
		break;
	case STAGE_3:
		printf("Item collected on Stage 3! You have won!!!!\n");
		player->score += STAGE_COMPLETION_POINTS; 
		EndGame(player);
		break;
	}
}

void HandleEvents(int* quit, Character* player, double* worldTime, int* onLadder, int* ladderDirection, int* t1, Game* game) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE) *quit = 1;
			if (event.key.keysym.sym == SDLK_n) { // resetuje stan gracza i beczek
				ResetPlayerAndTime(quit, player, worldTime, onLadder, ladderDirection);
				ResetBarrels(game, t1);
			}
			if (event.key.keysym.sym == SDLK_LEFT) player->vx = -5; // ruch w lewo
			if (event.key.keysym.sym == SDLK_RIGHT) player->vx = 5; // ruch w prawo
			if (event.key.keysym.sym == SDLK_UP && *onLadder) *ladderDirection = -1; // ruch w gore
			if (event.key.keysym.sym == SDLK_DOWN && *onLadder) *ladderDirection = 1; // ruch w dol
			if (event.key.keysym.sym == SDLK_SPACE && !player->isJumping) { // skok
				player->isJumping = 1;
				player->vy = -player->jumpPower;
			}
			if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_3) { // zmiana poziomu gry
				SwitchStage(game, player, event.key.keysym.sym - SDLK_1 + 1);
			}
			break;
		case SDL_QUIT:
			*quit = 1;
			break;
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT) player->vx = 0;
			if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) *ladderDirection = 0;
			break;
		};
	}
}

void ResetPlayerAndTime(int* quit, Character* player, double* worldTime, int* onLadder, int* ladderDirection) {
	*worldTime = 0; // resetowanie stanu gry i gracza
	*onLadder = 0;
	*ladderDirection = 0;
	player->vx = 0;
	player->vy = 0;
	player->rect.x = 0;
	player->rect.y = PLATFORM_HEIGHTS[0] - player->rect.h;
	player->score = 0;
}

void ResetBarrels(Game* game, int* t1) {
	static int lastBarrelSpawnTime = 0;
	lastBarrelSpawnTime = *t1; // resetownie beczek
	for (int i = 0; i < MAX_BARRELS; i++) {
		game->barrels[i].rect.x = SCREEN_WIDTH + 50;
		game->barrels[i].rect.y = PLATFORM_HEIGHTS[PLATFORM_COUNT - 1] - game->barrels[i].rect.h;
		game->barrels[i].vx = 0;
		game->barrels[i].isFalling = 0;
	}
}

void EndGame(Character* player) {
	printf("Game Ends. You scored %d points!\n", player->score);
	SDL_Quit();
	exit(0); 
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
	SDL_Surface* screen, * charset, * platform, * platform2, * platform3, * ladder, * robot, * barrel, * stageItem;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
	Character player;
	Game game;
	int t1, t2, quit, frames, frameTime;
	double delta, worldTime, fpsTimer, fps;

	int onLadder = 0; // flaga czy postaÊ jest na drabinie
	int ladderDirection = 0; // flaga ruchu na drabince (0 - brak, 1 - dol, -1 - gora)

	printf("Starting game...\n");

	if (InitializeGame(&player,&game, &screen, &charset, &platform, &platform2, &platform3, &ladder, &robot, &barrel, &stageItem, &scrtex, &window, &renderer) != 0) {
		printf("Initialization failed!\n");
		return 1;
	}

	t1 = SDL_GetTicks();
	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;

	while (!quit) {
		t2 = SDL_GetTicks();
		delta = (t2 - t1) * 0.001;
		t1 = t2;

		fpsTimer += delta;
		if (fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		};

		frameTime = SDL_GetTicks() - t1; // czas trwania klatki

		if (frameTime < 1000 / FPS) {
			SDL_Delay(1000 / FPS - frameTime); // opoznienie do 60 FPS
		}
		InitBarrels(barrel, &game, t1);
		HandleEvents(&quit, &player, &worldTime, &onLadder, &ladderDirection, &t1, &game);
		UpdateGameState(&player, &game, &onLadder, ladderDirection);
		worldTime += delta;
		frames++;
		RenderGame(screen, charset, &game, platform, platform2, platform3, ladder, robot, barrel, stageItem, scrtex, renderer, &player, &worldTime, &fps);
	}

	Cleanup(charset, screen, scrtex, window, renderer);
	printf("Game ended.\n");
	return 0;
}