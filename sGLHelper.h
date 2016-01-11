struct color_t
{
	float r;
	float g;
	float b;
	float a;

	color_t(float rr, float gg, float bb)
	{
		r = rr;
		g = gg;
		b = bb;
		a = 1.0f;
	}

	color_t()
	{
		color_t(0.0f, 0.0f, 0.0f);
	}
};

struct in_out_vertex_t
{
	float x;
	float y;
	float z;
	float w;

	float u;
	float v;

	color_t color;

	in_out_vertex_t(float xx, float yy, float zz, float uu, float vv, float rr, float gg, float bb)
	{
		x = xx;
		y = yy;
		z = zz;
		w = 1.0f;
		u = uu;
		v = vv;
		color.r = rr;
		color.g = gg;
		color.b = bb;
	}

	in_out_vertex_t()
	{
		in_out_vertex_t(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	}
};


struct in_out_pixel_t
{
	int x;
	int y;

	float z;

	float u;
	float v;
	float w;

	color_t color;
};


/* --------------------------- You can safely ignore everything below this line --------------------------*/

#include <SDL.h>


// Just some forward declarations
void sGLInit(int windowWidth, int windowHeight);
void sGLSetPixel(int x, int y, Uint32 color);
void sGLDrawElements(int count);
void sGLGenBuffer(int * outBufferIndex);
void sGLBindBuffer(int bufferID);
void sGLBufferData(int size, in_out_vertex_t * vertexData);
void sGLClearDepth(float zz);
void sGLEnableDepthTest();
void sGLEnableAlphaTest();
void sGLDisableAlphaTest();
void sGLUseVertexShader(in_out_vertex_t(*inVS)(in_out_vertex_t));
void sGLUsePixelShader(in_out_pixel_t(*inPS)(in_out_pixel_t));
void sGLUniform1f(float * location, float value);
void sGLSwapBuffers();
void sGLClear();
void sGLExit();
bool isRunning();


struct point2_t
{
	int x;
	int y;

	float dotProduct(point2_t otherVector)
	{
		return (float)(x * otherVector.x + y * otherVector.y);
	}

	point2_t(int xx, int yy)
	{
		x = xx;
		y = yy;
	}
};


// More Globals, WOHOOO!

SDL_Window * g_SDLWindow;
SDL_Renderer * g_SDLRenderer;
SDL_Texture * g_SDLTexture;
Uint32 * g_SDLBackBuffer;
int g_SDLWidth;
int g_SDLHeight;

void SDLStart(int windowWidth, int windowHeight)
{
	SDL_Init(SDL_INIT_VIDEO);

	g_SDLWidth = windowWidth;
	g_SDLHeight = windowHeight;

	g_SDLWindow = SDL_CreateWindow("stupidGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, 0);

	g_SDLRenderer = SDL_CreateRenderer(g_SDLWindow, -1, 0);
	g_SDLTexture = SDL_CreateTexture(g_SDLRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, windowWidth, windowHeight);

	g_SDLBackBuffer = new Uint32[windowWidth * windowHeight];
}

void SDLSwapBuffers(color_t * backbuffer)
{
	for (int i = 0; i < g_SDLHeight; ++i)
	{
		for (int j = 0; j < g_SDLWidth; ++j)
		{
			// Convert color_t to Uint32
			Uint32 iColor = ((uint8_t)(255.0f * backbuffer[i * g_SDLWidth + j].b) << 16) |
				((uint8_t)(255.0f * backbuffer[i * g_SDLWidth + j].g) << 8) |
				((uint8_t)(255.0f * backbuffer[i * g_SDLWidth + j].r)) & 0xffffff;

			g_SDLBackBuffer[i * g_SDLWidth + j] = iColor;
		}
	}

	SDL_UpdateTexture(g_SDLTexture, NULL, g_SDLBackBuffer, g_SDLWidth * sizeof(Uint32));
	SDL_RenderClear(g_SDLRenderer);
	SDL_RenderCopy(g_SDLRenderer, g_SDLTexture, NULL, NULL);
	SDL_RenderPresent(g_SDLRenderer);
}

void SDLEnd()
{
	delete[] g_SDLBackBuffer;

	SDL_DestroyTexture(g_SDLTexture);
	SDL_DestroyRenderer(g_SDLRenderer);
	SDL_DestroyWindow(g_SDLWindow);
	SDL_Quit();
}

bool isRunning()
{
	SDL_Event sEvent;

	SDL_PollEvent(&sEvent);

	switch (sEvent.type)
	{
	case SDL_QUIT:

		return false;

		break;
	}

	return true;
}

void calculateBarycentric(int pointX, int pointY, in_out_pixel_t * verts, float * outU, float * outV, float * outW)
{
	// Taken from http://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
	point2_t v0 = point2_t(verts[1].x - verts[0].x, verts[1].y - verts[0].y);
	point2_t v1 = point2_t(verts[2].x - verts[0].x, verts[2].y - verts[0].y);
	point2_t v2 = point2_t(pointX - verts[0].x, pointY - verts[0].y);
	float d00 = v0.dotProduct(v0);
	float d01 = v0.dotProduct(v1);
	float d11 = v1.dotProduct(v1);
	float d20 = v2.dotProduct(v0);
	float d21 = v2.dotProduct(v1);
	float invDenom = 1.0f / (d00 * d11 - d01 * d01);
	(*outV) = (d11 * d20 - d01 * d21) * invDenom;
	(*outW) = (d00 * d21 - d01 * d20) * invDenom;
	(*outU) = 1.0f - (*outV) - (*outW);
}

void sortAscendingY(int * xx, int * yy)
{
	int tX, tY;

	if (yy[1] > yy[0])
	{
		if (yy[1] > yy[2])
		{
			tX = xx[0];
			tY = yy[0];

			xx[0] = xx[1];
			yy[0] = yy[1];

			if (tY > yy[2])
			{
				xx[1] = tX;
				yy[1] = tY;
			}
			else
			{
				xx[1] = xx[2];
				yy[1] = yy[2];

				xx[2] = tX;
				yy[2] = tY;
			}
		}
		else
		{
			tX = xx[0];
			tY = yy[0];

			xx[0] = xx[2];
			yy[0] = yy[2];

			xx[2] = tX;
			yy[2] = tY;
		}
	}
	else
	{
		if (yy[0] > yy[2])
		{
			if (yy[1] < yy[2])
			{
				tX = xx[1];
				tY = yy[1];

				xx[1] = xx[2];
				yy[1] = yy[2];

				xx[2] = tX;
				yy[2] = tY;
			}
		}
		else
		{
			tX = xx[0];
			tY = yy[0];

			xx[0] = xx[2];
			yy[0] = yy[2];

			xx[2] = xx[1];
			yy[2] = yy[1];

			xx[1] = tX;
			yy[1] = tY;
		}
	}

	// Accidently sorted it the wrong way around...
	// Too lazy to fix.. So here is the dirty fix:

	tX = xx[0];
	tY = yy[0];

	xx[0] = xx[2];
	yy[0] = yy[2];

	xx[2] = tX;
	yy[2] = tY;
}