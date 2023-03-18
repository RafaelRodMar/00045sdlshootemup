#pragma once
#include <sdl.h>
#include <string>
#include "Vector2D.h"
#include "AssetsManager.h"

class Game;

class Entity
{
public:
	//movement variables
	Vector2D m_position;
	Vector2D m_velocity;
	Vector2D m_acceleration;

	float moveDelay = 0.0;

	//size variables
	int m_width = 0, m_height = 0;

	//animation variables
	int m_currentRow = 0;
	int m_currentFrame = 0;
	int m_numFrames = 0;
	std::string m_textureID;

	//common boolean variables
	bool m_bUpdating = false;
	bool m_bDead = false;
	bool m_bDying = false;

	//rotation
	double m_angle = 0;

	//blending
	int m_alpha = 255;

	//game variables
	int m_radius;
	bool m_life;
	bool m_shield;
	int m_shieldTime;
	int m_waitTime = 0;
	std::string m_name;

	Entity()
	{
		m_life = true;
		m_shield = false;
	}

	void settings(const string &Texture, Vector2D pos, Vector2D vel, int Width, int Height, int nFrames, int row, int cframe, double Angle = 0, int radius = 1)
	{
		m_textureID = Texture;
		m_position = pos;
		m_velocity = vel;
		m_width = Width; m_height = Height;
		m_angle = Angle;
		m_radius = radius;
		m_numFrames = nFrames;
		m_currentRow = row;
		m_currentFrame = cframe;
	}

	virtual void update();

	virtual void draw();

	virtual ~Entity() {};
};

class car : public Entity
{
public:
	car()
	{
		m_name = "car";
	}

	void update();
};


class asteroid : public Entity
{
public:
	asteroid()
	{
		m_velocity.m_x = rand() % 8 - 4;
		m_velocity.m_y = rand() % 8 - 4;
		m_name = "asteroid";
	}

	void  update();

};

class bullet : public Entity
{
public:
	bullet()
	{
		m_name = "bullet";
	}

	void update();
	void draw();
};


class playerold : public Entity
{
public:
	bool m_isMoving = false;
	bool m_onGround = false;
	bool m_heading = true; //true = right, false = left
	bool m_isJumping = false;
	int m_jumpHigh = 0;

	playerold()
	{
		m_name = "playerold";
	}

	void update();
	void draw();

};

class Player : public Entity
{
public:
	float fPlayerSpeed = 5.0f;
	float fPlayerShipRad = 24 * 24;
	float fPlayerHealth = 1000.0f;
	float fPlayerGunTemp = 0.0f;
	float fPlayerGunReload = 0.2f;
	float fPlayerGunReloadTime = 0.0f;

	Player()
	{
		m_name = "Player";
	}

	void update();
};

class Bullet : public Entity
{
public:
	bool remove = false;

	Bullet() {}

	void update();
	void draw();
};

class Enemy; //so i can use this in EnemyDefinition before defining Enemy.

struct EnemyDefinition
{
	double dTriggerTime;
	uint32_t nSpriteID = 0;
	float fHealth = 0.0f;
	std::function<void(Enemy&, float)> funcMove;
	std::function<void(Enemy&, float, std::list<Bullet>& bullets)> funcFire;
	float offset = 0.0f;
};

class Enemy : public Entity
{
public:
	EnemyDefinition def;

	std::array<float, 4> dataMove{ 0 };
	std::array<float, 4> dataFire{ 0 };

	Enemy()
	{
		m_name = "Enemy";
	}

	void update(float fScrollSpeed, std::list<Bullet>& bullets);
};



