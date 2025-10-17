// Console First Person Stroller.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <algorithm>
#include <iostream>
#include <chrono>
#include <Windows.h>

int nScreenWidth = 120;
int nScreenHeight = 40;

float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerA = 0.0f; // Angle of player

int nMapWidth = 16;
int nMapHeight = 16;

float fFOV = 3.14159f / 4.0f; // Field of View
float fDepth = 16.0f; // Maximum rendering distance

int main()
{
    // Creating the screen buffer

	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer
	(
		GENERIC_READ | GENERIC_WRITE, 
		0, 
		NULL, 
		CONSOLE_TEXTMODE_BUFFER, 
		NULL
	);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	std::wstring map;
	
	map += L"################";
	map += L"#..#...........#";
	map += L"#..#...........#";		
	map += L"#..#...........#";
	map += L"#..#...........#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";

	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();

	// Game Loop
	while (1)
	{
		// Game loop code would go here

		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();


		// Controls
		// Handles CCW Rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
		{
			fPlayerA -= (0.6f) * fElapsedTime; // Rotate left
		}

		if(GetAsyncKeyState((unsigned short)'D') & 0x8000)
		{
			fPlayerA += (0.6f) * fElapsedTime; // Rotate right
		}

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += sin(fPlayerA) * (3.0f) * fElapsedTime; // Move Forward
			fPlayerY += cos(fPlayerA) * (3.0f) * fElapsedTime; // Move Forward

			// Collision Detection
			if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX -= sin(fPlayerA) * (3.0f) * fElapsedTime; // Undo Move Forward
				fPlayerY -= cos(fPlayerA) * (3.0f) * fElapsedTime; // Undo Move Forward
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= sin(fPlayerA) * (3.0f) * fElapsedTime; // Move Back
			fPlayerY -= cos(fPlayerA) * (3.0f) * fElapsedTime; // Move Back

			// Collision Detection
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX += sin(fPlayerA) * (3.0f) * fElapsedTime; // Undo Move Back
				fPlayerY += cos(fPlayerA) * (3.0f) * fElapsedTime; // Undo Move Back
			}
		}



		for (int x = 0; x < nScreenWidth; x++)
		{
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;
			
			float fDistanceToWall = 0.0f;
			bool bHitWall = false;
			bool bBoundary = false;

			// Unit vector for ray in player space
			float fEyeX = sinf(fRayAngle);
			float fEyeY = cosf(fRayAngle);

			while (!bHitWall && fDistanceToWall < fDepth)
			{
				fDistanceToWall += 0.1f;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				if(nTestX < 0 || nTestX >=nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
				{
					bHitWall = true;
					fDistanceToWall = fDepth; // Just set distance to maximum depth
				}
				else
				{
					// Ray is inbounds so test to see if the ray cell is a wall block
					if (map[nTestY * nMapWidth + nTestX] == '#')
					{
						bHitWall = true;

						// To highlight tile boundaries, cast a ray from each corner
						// of the tile, to the player. The more coincident this ray
						// is to the rendering ray, the closer we are to a tile 
						// boundary, which we'll shade to add detail to the walls
						std::vector<std::pair<float, float>> p;

						// Test each corner of hit tile, storing the distance from
						// the player, and the calculated dot product of the two rays
						for (int tx = 0; tx < 2; tx++)
							for (int ty = 0; ty < 2; ty++)
							{
								// Angle of corner to eye
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float d = sqrt(vx * vx + vy * vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(std::make_pair(d, dot));
							}

						// Sort Pairs from closest to farthest
						sort(p.begin(), p.end(), [](const std::pair<float, float>& left, const std::pair<float, float>& right) {return left.first < right.first; });

						// First two/three are closest (we will never see all four)
						float fBound = 0.005f;
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;
						//if (acos(p.at(2).second) < fBound) bBoundary = true;
					}
				}
			}

			// Calculate distance to ceiling and floor
			int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			// Shading walls based on distance
			short nShade = ' ';

			if(fDistanceToWall <= fDepth/4.0f)       nShade = 0x2588; // Very close
			else if(fDistanceToWall < fDepth / 3.0f)   nShade = 0x2593;
			else if(fDistanceToWall < fDepth / 2.0f)   nShade = 0x2592;
			else if(fDistanceToWall < fDepth)          nShade = 0x2591;
			else                                       nShade = ' ';    // Too far away

			if (bBoundary) nShade = ' '; // Black it out

			for (int y = 0; y < nScreenHeight; y++)
			{
				if (y < nCeiling) 
				{
					screen[y * nScreenWidth + x] = ' ';
				}

				else if (y > nCeiling && y <= nFloor) 
				{
					screen[y * nScreenWidth + x] = nShade;
				}
				else 
				{
					//Shade Floor based on distance
					float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					short nShade = ' ';
					if (b < 0.25)       
						nShade = '#';
					else if (b < 0.5)   
						nShade = 'x';
					else if (b < 0.75)  
						nShade = '.';
					else if (b < 0.9)   
						nShade = '-';
					else
						nShade = ' ';
					
					screen[y * nScreenWidth + x] = nShade;
				}
			}
		}
		
		// Display Stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

		// Display Map
		for (int nx = 0; nx < nMapWidth; nx++)
		{
			for (int ny = 0; ny < nMapWidth; ny++)
			{
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}
		}
			
		screen[((int)fPlayerX + 1) * nScreenWidth + (int)fPlayerY] = 'P';


		// Display Frame
		screen[nScreenWidth * nScreenHeight - 1] = '\0';

		WriteConsoleOutputCharacter
		(
			hConsole, 
			screen,
			nScreenWidth * nScreenHeight, 
			{ 0,0 }, 
			&dwBytesWritten
		);

		

	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
