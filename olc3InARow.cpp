#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <time.h>
#include <iostream>
#include <format>

const olc::Pixel cBG = { 255, 255, 255 };	// Background Color
const olc::Pixel cFG = { 0, 0, 0 };			// Foreground Color (grid lines)
const olc::Pixel cP1 = { 255, 0, 0 };		// X Color
const olc::Pixel cP2 = { 0, 0, 255 };		// O Color

const int screenW = 256;
const int screenH = 256;

const int borderL = 20;	// Padding around the edges
const int gridL = (screenW - borderL * 2) / 3;
const int xr = gridL / 2 - 5;

enum GAME_STATE
{
	GS_MENU = 0,
	GS_RESET = 1,
	GS_STARTPLAY,
	GS_PLAYER1,
	GS_PLAYER2,
	GS_PLAYER2_BOT,
	GS_CHKWINNER,
	GS_RESULT
} nGameState, nNextState;

class Program : public olc::PixelGameEngine
{
public:
	Program()
	{
		sAppName = "3 In A Row";
	}

private:
	char grid[3][3] = {0};	// -1 : P1 || 1 : P2 || 0 : Empty
	bool bCurPlayer = 0;	// 0 : P1 || 1 : P2
	bool bP2IsBot = 1;
	int nRound = 0;
	int nWinCase = 0;
	float fTime = 0;

public:
	bool OnUserCreate() override
	{
		srand(time(NULL));
		nGameState = GS_RESET;
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(cBG);
		if (nGameState > 1)
			DrawGame();

		switch (nGameState)
		{
		case GS_MENU:
		{

		}
		break;

		case GS_RESET:
		{
			// reset vars
			memset(grid, 0, sizeof(grid));
			nRound = 0;
			nWinCase = 0;
			nNextState = GS_STARTPLAY;
		}
		break;

		case GS_STARTPLAY:
		{
			// determine player to go first
			if (rand() % 2)
				nNextState = GS_PLAYER1;
			else if (bP2IsBot)
				nNextState = GS_PLAYER2_BOT;
			else
				nNextState = GS_PLAYER2;
		}
		break;

		case GS_PLAYER1:
		{
			// player 1 placement
			bCurPlayer = 0;
			DrawString(2, screenH - 9, "PLAYER 1 GO", cFG);
			DrawString(2, 2, GetGridPos().str(), olc::BLACK);

			int x = gridL * GetGridPos().x + gridL / 2 + borderL;
			int y = gridL * GetGridPos().y + gridL / 2 + borderL;
			if (grid[GetGridPos().y][GetGridPos().x] == 0)
			{
				DrawLine(x - xr, y - xr, x + xr, y + xr, cP1);
				DrawLine(x + xr, y - xr, x - xr, y + xr, cP1);
			}

			if (GetMouse(0).bPressed && grid[GetGridPos().y][GetGridPos().x] == 0)
			{
				grid[GetGridPos().y][GetGridPos().x] = -1;
				nNextState = GS_CHKWINNER;
			}
		}
		break;

		case GS_PLAYER2:
		{
			// player 2 placement
			bCurPlayer = 1;
			DrawString(2, screenH - 9, "PLAYER 2 GO", cFG);

			int x = gridL * GetGridPos().x + gridL / 2 + borderL;
			int y = gridL * GetGridPos().y + gridL / 2 + borderL;
			if (grid[GetGridPos().y][GetGridPos().x] == 0)
				DrawCircle(x, y, xr, cP2);

			if (GetMouse(0).bPressed && grid[GetGridPos().y][GetGridPos().x] == 0)
			{
				grid[GetGridPos().y][GetGridPos().x] = 1;
				nNextState = GS_CHKWINNER;
			}
			//nNextState = GS_CHKWINNER;
		}
		break;

		case GS_PLAYER2_BOT:	// TODO: Make bot smarter
		{
			// bot placement, delay 1 sec before placing
			bCurPlayer = 1;
			DrawString(2, screenH - 9, "THINKING...", cFG);
			fTime += fElapsedTime;
			int x = rand() % 3;
			int y = rand() % 3;

			if (fTime > 1 && grid[x][y] == 0)
			{
				grid[x][y] = 1;
				fTime = 0;
				nNextState = GS_CHKWINNER;
			}
		}
		break;

		case GS_CHKWINNER:
		{
			// check if any wins on board
			nRound++;
			nWinCase = CheckWinCase();
			if (nWinCase || nRound == 9)
				nNextState = GS_RESULT;
			else
			{
				if (bCurPlayer)
					nNextState = GS_PLAYER1;
				else if (bP2IsBot)
					nNextState = GS_PLAYER2_BOT;
				else
					nNextState = GS_PLAYER2;
			}
		}
		break;

		case GS_RESULT:
		{
			// Display result
			if (nWinCase)
				DrawWinCase(nWinCase);
			else
				DrawString(2, screenH - 9, "TIE GAME", cFG);

			DrawString(screenW - 130, screenH - 9, "[LMB] PLAY AGAIN", cFG);
			if (GetMouse(0).bPressed)
				nNextState = GS_RESET;
		}
		break;
		}
	
		if(GetKey(olc::ESCAPE).bPressed)
			nNextState = GS_MENU;

		// DrawString(10, 10, std::to_string(nWinCase), olc::BLACK);

		//DrawWinCase(nWinCase);

		nGameState = nNextState;

		return true;
	}

	olc::vi2d GetGridPos()
	{
		return olc::vi2d(std::min((GetMouseX() - borderL) / gridL, 2), std::min((GetMouseY() - borderL) / gridL, 2));
	}

	void DrawGame()
	{
		DrawLine(borderL, borderL + gridL, borderL + gridL * 3, borderL + gridL, cFG);
		DrawLine(borderL, borderL + gridL * 2, borderL + gridL * 3, borderL + gridL * 2, cFG);
		DrawLine(borderL + gridL, borderL, borderL + gridL, borderL + gridL * 3, cFG);
		DrawLine(borderL + gridL * 2, borderL, borderL + gridL * 2, borderL + gridL * 3, cFG);

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				int y = gridL * i + gridL / 2 + borderL;
				int x = gridL * j + gridL / 2 + borderL;

				/*
				int dx = gridL * i + borderL;
				int dy = gridL * j + borderL;
				DrawString(dx, dy, std::to_string(i) + std::to_string(j), cFG);
				DrawString(dx, dy + 10, std::to_string(grid[i][j]), cFG);
				*/

				switch (grid[i][j])
				{
				case 0:
					continue;
				case -1:
					DrawLine(x - xr, y - xr, x + xr, y + xr, cP1);
					DrawLine(x + xr, y - xr, x - xr, y + xr, cP1);
					continue;
				case 1:
					DrawCircle(x, y, xr, cP2);
					continue;
				}
			}
		}
	}

	int CheckWinCase()
	{
		bool bWin = 0;
		int nTemp = 0;	
		int nCount = 0;
		int nWin = 0;

		for (int i = 0; i < 3; i++) {
			nWin++;
			for (int j = 0; j < 3; j++) {	// Horizontal
				nCount++;

				nTemp += grid[i][j];
				if (abs(nTemp) == 3)
					return nWin;

				if (nCount == 3)
				{
					nTemp = 0;
					nCount = 0;
				}
			}
		}

		for (int i = 0; i < 3; i++) {
			nWin++;
			for (int j = 0; j < 3; j++) {	// Horizontal
				nCount++;

				nTemp += grid[j][i];
				if (abs(nTemp) == 3)
					return nWin;

				if (nCount == 3)
				{
					nTemp = 0;
					nCount = 0;
				}
			}
		}

		nWin++;
		for (int i = 0; i < 3; i++) // Diagonal 
				nTemp += grid[i][i];
		if (abs(nTemp) == 3)
			return nWin;
		nTemp = 0;

		nWin++;
		for (int i = 0; i < 3; i++) // Diagonal 
			nTemp += grid[i][2 - i];
		if (abs(nTemp) == 3)
			return nWin;

		return 0;
	}

	void DrawWinCase(int n)
	{
		olc::Pixel cWin;
		if (bCurPlayer)
			cWin = cP2;
		else 
			cWin = cP1;

		switch (n)	// Draw Win Line
		{
		case 1:
			DrawLine(borderL, borderL + gridL / 2, borderL + gridL * 3, borderL + gridL / 2, cWin);
			break;
		case 2:
			DrawLine(borderL, borderL + gridL / 2 + gridL, borderL + gridL * 3, borderL + gridL / 2 + gridL, cWin);
			break;
		case 3:
			DrawLine(borderL, borderL + gridL / 2 + gridL * 2, borderL + gridL * 3, borderL + gridL / 2 + gridL * 2, cWin);
			break;
		case 4:
			DrawLine(borderL + gridL / 2, borderL, borderL + gridL / 2, borderL + gridL * 3, cWin);
			break;
		case 5:
			DrawLine(borderL + gridL / 2 + gridL, borderL, borderL + gridL / 2 + gridL, borderL + gridL * 3, cWin);
			break;
		case 6:
			DrawLine(borderL + gridL / 2 + gridL * 2, borderL, borderL + gridL / 2 + gridL * 2, borderL + gridL * 3, cWin);
			break;
		case 7:
			DrawLine(borderL, borderL, borderL + gridL * 3, borderL + gridL * 3, cWin);
			break;
		case 8:
			DrawLine(borderL, borderL + gridL * 3, borderL + gridL * 3, borderL, cWin);
			break;
		}

		DrawString(2, screenH - 9, std::string("PLAYER ") + std::to_string(bCurPlayer+1) + " WINS", cWin);
	}

};

int main()
{
	Program game;
	if (game.Construct(screenW, screenH, 3, 3))
		game.Start();

	return 0;
}