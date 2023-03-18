#pragma once

#include <vector>
#include <SDL.h>
#include <SDL_image.h>
#include "AssetsManager.h"
#include "InputHandler.h"
#include "entity.h"

class Game {
public:
	static Game* Instance()
	{
		if (s_pInstance == 0)
		{
			s_pInstance = new Game();
			return s_pInstance;
		}
		return s_pInstance;
	}

	SDL_Renderer* getRenderer() const { return m_pRenderer; }

	~Game();

	bool init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
	void render();
	void update();
	void handleEvents();
	//void clean();
	void quit();

	bool running() { return m_bRunning; }

	int getGameWidth() const { return m_gameWidth; }
	int getGameHeight() const { return m_gameHeight; }

	Player* player;
	Enemy* enemy[3];

	float fScrollSpeed = 1.0f;
	double dWorldPos = 0.0f;
	std::array<Vector2D, 1000> aryStars;

private:
	Game();
	static Game* s_pInstance;
	SDL_Window* m_pWindow;
	SDL_Renderer* m_pRenderer;

	std::list<Entity*> entities;

	std::list<EnemyDefinition> listSpawns;
	std::list<Enemy> listEnemies;
	std::list<Bullet> listEnemyBullets;
	std::list<Bullet> listPlayerBullets;
	std::list<Vector2D> listStars;
	std::list<Bullet> listFragments;

	bool m_bRunning;
	int m_gameWidth;
	int m_gameHeight;
};
