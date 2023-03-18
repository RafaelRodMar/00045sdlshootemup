#include<SDL.h>
#include<SDL_image.h>
#include<SDL_ttf.h>
#include<iostream>
#include<string>
#include<vector>
#include"Vector2D.h"
#include"json.hpp"
#include"AssetsManager.h"
#include"InputHandler.h"
#include"game.h"

//the game class
Game* Game::s_pInstance = 0;

Game::Game() {
	m_pRenderer = nullptr;
	m_pWindow = nullptr;
}

Game::~Game() {}

SDL_Window* g_pWindow = 0;
SDL_Renderer* g_pRenderer = 0;

bool Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
	m_gameWidth = width;
	m_gameHeight = height;

	// initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		int flags = SDL_WINDOW_SHOWN;
		if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN;

		// if succeeded create our window
		std::cout << "SDL init success\n";
		m_pWindow = SDL_CreateWindow(title,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			width, height, flags);
		// if the window creation succeeded create our renderer
		if (m_pWindow != 0)
		{
			std::cout << "window creation success\n";
			m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, 0);
			if (m_pRenderer != 0)
			{
				std::cout << "renderer creation success\n";
				SDL_SetRenderDrawColor(m_pRenderer, 255, 255, 255, 255);
			}
			else
			{
				std::cout << "renderer init fail\n";
				return false;
			}
		}
		else
		{
			std::cout << "window init fail\n";
			return false;
		}
	}
	else
	{
		std::cout << "SDL init fail\n";
		return false; // sdl could not initialize
	}

	if (TTF_Init() == 0)
	{
		std::cout << "sdl font initialization success\n";
	}
	else
	{
		std::cout << "sdl font init fail\n";
		return false;
	}

	std::cout << "init success\n";
	m_bRunning = true;

	//pass the renderer to the assets manager
	AssetsManager::Instance()->renderer = m_pRenderer;
	//load all the assets in the json file
	AssetsManager::Instance()->loadAssetsJson();

	//create player and enemies
	player = new Player();
	player->settings("player", Vector2D(Game::Instance()->getGameWidth()/2, Game::Instance()->getGameHeight()/2), Vector2D(0, 0), 48, 48, 0, 0, 0, 0.0, 1);

	//generate star map
	for (auto& s : aryStars) s = { (float)(rand() % Game::Instance()->getGameWidth()), (float)(rand() % Game::Instance()->getGameHeight()) };

	// MOVEMENT PATTERN FUNCTIONS
	auto Move_None = [&](Enemy& e, float fScrollSpeed)
	{
		e.m_position.m_y += fScrollSpeed;
	};

	auto Move_StraightFast = [&](Enemy& e, float fScrollSpeed)
	{
		e.m_position.m_y += 3.0f * fScrollSpeed;
	};

	auto Move_StraightSlow = [&](Enemy& e, float fScrollSpeed)
	{
		e.m_position.m_y += 0.5f * fScrollSpeed;
	};

	auto Move_SinusoidNarrow = [&](Enemy& e, float fScrollSpeed)
	{
		e.dataMove[0] += 0.05; //increments for the sinusoidal movement
								//calculated with cosf
		e.m_position.m_y += 0.5f * fScrollSpeed;
		e.m_position.m_x += 4.0f * cosf((int)e.dataMove[0]); //4.0f is the wide multiplier
	};

	auto Move_SinusoidWide = [&](Enemy& e, float fScrollSpeed)
	{
		e.dataMove[0] += 0.04;
		e.m_position.m_y += 0.5f * fScrollSpeed;
		e.m_position.m_x += 6.0f * cosf((int)e.dataMove[0]);
	};

	// FIRING PATTERN FUNCTIONS
	auto Fire_None = [&](Enemy& e, float fScrollSpeed, std::list<Bullet>& bullets)
	{

	};

	auto Fire_StraightDelay2 = [&](Enemy& e, float fScrollSpeed, std::list<Bullet>& bullets)
	{
		constexpr float fDelay = 40;
		e.dataFire[0] += 1;
		if (e.dataFire[0] >= fDelay)
		{
			e.dataFire[0] -= fDelay;
			Bullet b;
			b.m_position = e.m_position + Vector2D((float)e.m_width / 2.0f, (float)e.m_height);
			b.m_velocity = { 0.0f, 5.0f };
			bullets.push_back(b);
		}
	};

	auto Fire_CirclePulse2 = [&](Enemy& e, float fScrollSpeed, std::list<Bullet>& bullets)
	{
		constexpr float fDelay = 40;
		constexpr int nBullets = 10;
		constexpr float fTheta = 2.0f * 3.14159f / (float)nBullets;
		e.dataFire[0] += 1;
		if (e.dataFire[0] >= fDelay)
		{
			e.dataFire[0] -= fDelay;
			for (int i = 0; i < nBullets; i++)
			{
				Bullet b;
				b.m_position = e.m_position + Vector2D(e.m_width / 2, e.m_height / 2);
				b.m_velocity = { 5.0f * cosf(fTheta * i), 5.0f * sinf(fTheta * i) };
				bullets.push_back(b);
			}
		}
	};

	auto Fire_DeathSpiral = [&](Enemy& e, float fScrollSpeed, std::list<Bullet>& bullets)
	{
		constexpr float fDelay = 2;
		e.dataFire[0] += 1;
		if (e.dataFire[0] >= fDelay)
		{
			e.dataFire[1] += 0.1f;
			e.dataFire[0] -= fDelay;
			Bullet b;
			b.m_position = e.m_position + Vector2D(e.m_width/2, e.m_height/2);
			b.m_velocity = { 5.0f * cosf(e.dataFire[1]), 5.0f * sinf(e.dataFire[1]) };
			bullets.push_back(b);

		}
	};

	auto Fire_DeathSpiralCircle = [&](Enemy& e, float fScrollSpeed, std::list<Bullet>& bullets)
	{
		constexpr float fDelay = 30;
		constexpr int nBullets = 100;
		constexpr float fTheta = 2.0f * 3.14159f / (float)nBullets;
		e.dataFire[0] += 1;
		if (e.dataFire[0] >= fDelay)
		{
			e.dataFire[0] -= fDelay;
			e.dataFire[1] += 0.1f;
			for (int i = 0; i < nBullets; i++)
			{
				Bullet b;
				b.m_position = e.m_position + Vector2D(e.m_width / 2, e.m_height / 2);
				b.m_velocity = { 5.0f * cosf(fTheta * i + e.dataFire[1]), 5.0f * sinf(fTheta * i + e.dataFire[1]) };
				bullets.push_back(b);
			}
		}
	};

	// Construct level
	listSpawns = 
	{ 
		//triggerTime, spriteID, health, funcMove, funcFire, offset
		{100.0, 0, 3.0f, Move_None,           Fire_CirclePulse2, 0.25f},
		{100.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.75f},
		{120.0, 1, 3.0f, Move_SinusoidNarrow, Fire_CirclePulse2, 0.50f},

		{200.0, 2, 3.0f, Move_SinusoidWide, Fire_CirclePulse2, 0.30f},
		{200.0, 2, 3.0f, Move_SinusoidWide, Fire_CirclePulse2, 0.70f},

		{500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.2f},
		{500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.4f},
		{500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.6f},
		{500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.8f},

		{550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.1f},
		{550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.3f},
		{550.0, 0, 3.0f, Move_StraightSlow,   Fire_DeathSpiralCircle,  0.5f},
		{550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.7f},
		{550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.9f},

		{600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.2f},
		{600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.4f},
		{600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.6f},
		{600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.8f},

		{1100.0, 0, 3.0f, Move_None,           Fire_CirclePulse2, 0.25f},
		{1100.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.75f},
		{1120.0, 1, 3.0f, Move_SinusoidNarrow, Fire_CirclePulse2, 0.50f},

		{1200.0, 2, 3.0f, Move_SinusoidWide, Fire_CirclePulse2, 0.30f},
		{1200.0, 2, 3.0f, Move_SinusoidWide, Fire_CirclePulse2, 0.70f},

		{1500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.2f},
		{1500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.4f},
		{1500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.6f},
		{1500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.8f},

		{1550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.1f},
		{1550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.3f},
		{1550.0, 0, 3.0f, Move_StraightSlow,   Fire_DeathSpiralCircle,  0.5f},
		{1550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.7f},
		{1550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.9f},

		{1600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.2f},
		{1600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.4f},
		{1600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.6f},
		{1600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.8f},
	};

	g_pRenderer = m_pRenderer;
	g_pWindow = m_pWindow;
	return true;
}

void Game::handleEvents() {
	InputHandler::Instance()->update();

	//Scroll player object
	player->m_position.m_y += fScrollSpeed;

	// Handle Player Input
	if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_W)) player->m_position.m_y -= (player->fPlayerSpeed + fScrollSpeed);
	if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_S)) player->m_position.m_y += (player->fPlayerSpeed - fScrollSpeed);
	if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_A)) player->m_position.m_x -= player->fPlayerSpeed * 2.0f;
	if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_D)) player->m_position.m_x += player->fPlayerSpeed * 2.0f;

	// Clamp Player to screen
	if (player->m_position.m_x <= 0) player->m_position.m_x = 0;
	if (player->m_position.m_x + (float)player->m_width >= Game::Instance()->getGameWidth()) player->m_position.m_x = (float)Game::Instance()->getGameWidth() - player->m_width;
	if (player->m_position.m_y <= 0) player->m_position.m_y = 0;
	if (player->m_position.m_y + (float)player->m_height >= Game::Instance()->getGameHeight()) player->m_position.m_y = (float)Game::Instance()->getGameHeight() - player->m_height;

	// Player Weapon Fire
	bool bCanFire = false;
	player->fPlayerGunReloadTime -= 1;
	if (player->fPlayerGunReloadTime <= 0.0f)
	{
		bCanFire = true;
	}

	player->fPlayerGunTemp -= 1 * 10.0f;
	if (player->fPlayerGunTemp < 0) player->fPlayerGunTemp = 0;
	if (InputHandler::Instance()->getMouseButtonState(0))
	{
		if (bCanFire && player->fPlayerGunTemp < 80.0f)
		{
			player->fPlayerGunReloadTime = player->fPlayerGunReload;
			player->fPlayerGunTemp += 5.0f;
			if (player->fPlayerGunTemp > 100.0f) player->fPlayerGunTemp = 100.0f;
			Bullet b;
			b.m_position = player->m_position + Vector2D(player->m_width / 2, 0);
			b.m_velocity = Vector2D(0, -5);
			listPlayerBullets.push_back(b);
		}
	}
}

void Game::update() {
	//AutoScroll world
	dWorldPos += fScrollSpeed; //keeps moving downwards and triggers the listSpawns elements.

	// Update Player Bullets
	for (auto& b : listPlayerBullets)
	{
		// Position Bullet
		b.m_position += (b.m_velocity + Vector2D(0.0f, fScrollSpeed));

		// Check Enemies Vs Player Bullets
		for (auto& e : listEnemies)
		{
			if ((b.m_position - (e.m_position + Vector2D(24.0f, 24.0f))).length2() < player->fPlayerShipRad)
			{
				// Enemy has been hit
				b.remove = true;
				e.def.fHealth -= 1.0f;

				// Trigger explosion
				if (e.def.fHealth <= 0)
				{
					for (int i = 0; i < 500; i++)
					{
						float angle = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f;
						float speed = ((float)rand() / (float)RAND_MAX) * 10.0f + 5;
						Bullet f; //fragments are stored in bullet format.
						f.m_position = e.m_position + Vector2D(e.m_width / 2, e.m_height / 2);
						f.m_velocity = { cosf(angle)*speed, sinf(angle)*speed };
						listFragments.push_back(f);
					}
				}
			}
		}
	}

	// Perform spawn check
	while (!listSpawns.empty() && dWorldPos >= listSpawns.front().dTriggerTime)
	{
		Enemy e;
		e.def = listSpawns.front();
		e.m_position =
		{
			listSpawns.front().offset * (float)(Game::Instance()->getGameWidth() - e.m_width),
			0.0f - e.m_height
		};
		switch (e.def.nSpriteID) {
		case 0:
			e.m_textureID = "enemy1";
			break;
		case 1:
			e.m_textureID = "enemy2";
			break;
		case 2:
			e.m_textureID = "enemy3";
			break;
		default:
			break;
		}
		e.m_width = e.m_height = 48;
		//std::cout << e.m_textureID << " at " << e.m_position.m_x << "," << e.m_position.m_y << std::endl;
		listSpawns.pop_front();
		listEnemies.push_back(e);
	}

	// Update Enemies
	for (auto& e : listEnemies)	e.update(fScrollSpeed, listEnemyBullets);

	// Update Enemy Bullets
	for (auto& b : listEnemyBullets)
	{
		// Position Bullet
		b.m_position += (b.m_velocity + Vector2D(0.0f, fScrollSpeed));

		// Check Player Vs Enemy Bullets
		if ((b.m_position - (player->m_position + Vector2D(24.0f, 24.0f))).length2() < player->fPlayerShipRad)
		{
			b.remove = true;
			player->fPlayerHealth -= 10.0f;
		}
	}

	// Update Fragments
	for (auto& f : listFragments) f.m_position += (f.m_velocity + Vector2D(0.0f, fScrollSpeed));

	// Remove Offscreen Enemies
	listEnemies.remove_if([&](const Enemy& e) {return (e.m_position.m_y >= (float)Game::Instance()->getGameHeight()) || e.def.fHealth <= 0.0f; });

	// Remove finished enemy bullets
	listEnemyBullets.remove_if([&](const Bullet& b) {return b.m_position.m_x<0 || b.m_position.m_x>Game::Instance()->getGameWidth() || b.m_position.m_y <0 || b.m_position.m_y>Game::Instance()->getGameHeight() || b.remove; });

	// Remove finished player bullets
	listPlayerBullets.remove_if([&](const Bullet& b) {return b.m_position.m_x<0 || b.m_position.m_x>Game::Instance()->getGameWidth() || b.m_position.m_y <0 || b.m_position.m_y>Game::Instance()->getGameHeight() || b.remove; });

	// Remove finished fragments
	listFragments.remove_if([&](const Bullet& b) {return b.m_position.m_x<0 || b.m_position.m_x>Game::Instance()->getGameWidth() || b.m_position.m_y <0 || b.m_position.m_y>Game::Instance()->getGameHeight() || b.remove; });
}

void Game::render() {
	// set to black // This function expects Red, Green, Blue and
	// Alpha as color values
	SDL_SetRenderDrawColor(g_pRenderer, 0, 0, 0, 255);
	// clear the window to black
	SDL_RenderClear(g_pRenderer);

	// Update & Draw Stars		
	for (size_t i = 0; i < aryStars.size(); i++)
	{
		auto& s = aryStars[i];
		s.m_y += fScrollSpeed * ((i < aryStars.size() >> 2) ? 0.8f : 1.0f);
		if (s.m_y >= (float)Game::Instance()->getGameHeight())
			s = { (float)(rand() % Game::Instance()->getGameWidth()), 0.0f };

		//color white
		int r = 255, g = 255, b = 255;
		if (i < aryStars.size() >> 2)
		{
			//color dark grey
			r = 169; g = 169; b = 169;
		}
		SDL_SetRenderDrawColor(g_pRenderer, r, g, b, 255);

		SDL_RenderDrawPoint(g_pRenderer, s.m_x, s.m_y);
	}
	SDL_SetRenderDrawColor(g_pRenderer, 0, 0, 0, 255);

	//Draw enemies
	for (auto& e : listEnemies) e.draw();

	//Draw player
	player->draw();

	// Draw Enemy Bullets
	SDL_SetRenderDrawColor(g_pRenderer, 255, 0, 0, 255);
	for (auto& b : listEnemyBullets) {
		SDL_Rect r;
		r.x = b.m_position.m_x;
		r.y = b.m_position.m_y;
		r.h = r.w = 3;
		SDL_RenderDrawRect(g_pRenderer, &r);
	}

	// Draw Player Bullets
	SDL_SetRenderDrawColor(g_pRenderer, 0, 0, 255, 255);
	for (auto& b : listPlayerBullets) {
		SDL_Rect r;
		r.x = b.m_position.m_x;
		r.y = b.m_position.m_y;
		r.h = r.w = 3;
		SDL_RenderDrawRect(g_pRenderer, &r);
	}

	// Draw Fragments
	SDL_SetRenderDrawColor(g_pRenderer, 255, 255, 0, 255);
	for (auto& b : listFragments) {
		SDL_Rect r;
		r.x = b.m_position.m_x;
		r.y = b.m_position.m_y;
		r.h = r.w = 1;
		SDL_RenderDrawRect(g_pRenderer, &r);
	}

	//Draw Player Health bar
	AssetsManager::Instance()->Text("HEALTH:", "font", 4, 4, { 0, 255, 0, 255 }, g_pRenderer);
	SDL_Rect r = { 100, 10, player->fPlayerHealth * 100 / 640, 8 };
	SDL_RenderFillRect(g_pRenderer, &r);

	/*AssetsManager::Instance()->Text("WEAPON:", "font", 4, 25, { 0, 255, 0, 255 }, g_pRenderer);
	r = { 150, 10, (int)player->fPlayerGunTemp * 100 / 640, 8 };
	SDL_RenderFillRect(g_pRenderer, &r);*/

	// show the window
	SDL_RenderPresent(g_pRenderer);
}

void Game::quit() {
	m_bRunning = false;
}

const int FPS = 60;
const int DELAY_TIME = 1000.0f / FPS;

int main(int argc, char* args[])
{
	srand(time(nullptr));

	Uint32 frameStart, frameTime;

	if (Game::Instance()->init("Shoot'em up", 100, 100, 640, 480,
		false))
	{
		while (Game::Instance()->running()) {
			frameStart = SDL_GetTicks();

			Game::Instance()->handleEvents();
			Game::Instance()->update();
			Game::Instance()->render();

			frameTime = SDL_GetTicks() - frameStart;

			if (frameTime < DELAY_TIME)
			{
				SDL_Delay((int)(DELAY_TIME - frameTime));
			}
		}
	}
	else
	{
		std::cout << "init failure - " << SDL_GetError() << "\n";
		return -1;
	}

	std::cout << "closing...\n";

	// clean up SDL
	SDL_DestroyWindow(g_pWindow);
	SDL_DestroyRenderer(g_pRenderer);
	AssetsManager::Instance()->clearAllTextures();
	InputHandler::Instance()->clean();
	SDL_Quit();
	return 0;
}