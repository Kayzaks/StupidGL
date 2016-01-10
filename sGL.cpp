
// You will require the SDL2 Library to Compile
// SDL2 is only used to draw Pixels on the Screen.
// Everything "3D" is done in our Software Rasterizer "sGL" below

// NOTE: This is not meant to run at any usable Framerate!

// - Michael Kissner (@Spellwrath)

#include <SDL.h>
#include <iostream>

#include "sGLHelper.h"

/* --------------------------------------- Our Vertex and Pixel Shader -----------------------------------*/

#define WHITE color_t(1.0f, 1.0f, 1.0f)
#define BLACK color_t(0.0f, 0.0f, 0.0f)
#define TEXTURE_SIZE 6

color_t g_TextureSampler[TEXTURE_SIZE][TEXTURE_SIZE] = {
	{ WHITE, BLACK, WHITE, WHITE, BLACK, WHITE },
	{ WHITE, BLACK, BLACK, BLACK, BLACK, WHITE },
	{ WHITE, BLACK, WHITE, WHITE, BLACK, WHITE },
	{ WHITE, BLACK, WHITE, WHITE, BLACK, WHITE },
	{ WHITE, BLACK, WHITE, WHITE, BLACK, WHITE },
	{ BLACK, BLACK, WHITE, WHITE, BLACK, BLACK },
};

in_out_vertex_t vertexShader(in_out_vertex_t inVertex)
{
	in_out_vertex_t out = inVertex;

	// Just a simple Vertex Shader that slightly Scales the Vertices
	out.x = out.x * 1.5f;
	out.y = out.y * 2.0f;
	out.z = 2.0f;

	return out;
}


in_out_pixel_t pixelShader(in_out_pixel_t inPixel)
{
	in_out_pixel_t out;

	out.x = inPixel.x;
	out.y = inPixel.y;

	int texU = (int)(inPixel.u * TEXTURE_SIZE);
	int texV = (int)(inPixel.v * TEXTURE_SIZE);

	// We "sample" the Texture
	out.color = g_TextureSampler[texU][texV];

	// ... and overlay the Vertex Color
	out.color.r = out.color.r * inPixel.color.r;
	out.color.g = out.color.g * inPixel.color.g;
	out.color.b = out.color.b * inPixel.color.b;

	return out;
}

/* ----------------------------------------------- The Main Loop------------------------------------------*/


int main(int argc, char ** argv)
{
	// This might all look intentionally familiar if you have experience with OpenGL.

	sGLInit(640, 480);

	in_out_vertex_t vertices[6] =
	{
		in_out_vertex_t(-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f),
		in_out_vertex_t(0.5f, -0.4f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f),
		in_out_vertex_t(0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f),


		in_out_vertex_t(-0.5f, 0.7f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f),
		in_out_vertex_t(-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f),
		in_out_vertex_t(0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f),
	};

	int bufferID;

	sGLGenBuffer(&bufferID);
	sGLBindBuffer(bufferID);
	sGLBufferData(6, vertices);

	sGLClearDepth(1.0f);
	sGLEnableDepthTest();

	while (isRunning())
	{
		sGLClear();

		sGLBindBuffer(bufferID);

		sGLUseVertexShader(vertexShader);
		sGLUsePixelShader(pixelShader);

		sGLDrawElements(2);

		sGLSwapBuffers();
	}

	sGLExit();

	return 0;
}


/* --------------------------------------------- Our own stupidGL ----------------------------------------*/

// Yeah. Globals! Because we can.
// For this, assume that all these things are present on the Graphics Card instead
// For ease of Programming, I've made them all globals

Uint32 * g_sGLBackBuffer;
float * g_sGLDepthBuffer;
int g_sGLBackbufferWidth;
int g_sGLBackbufferHeight;

in_out_vertex_t(*g_sGLVertexShader)(in_out_vertex_t);
in_out_pixel_t(*g_sGLPixelShader)(in_out_pixel_t);

// Let's fix the maximum amount of Buffers at some random number: 32
in_out_vertex_t *g_sGLVertexBuffer[32];
int g_sGLNumBuffers = 0;
int g_sGLBoundBuffer = 0;

bool g_sGLDepthTest = false;
float g_sGLDepthClear = 1.0f;



void sGLInit(int windowWidth, int windowHeight)
{
	// Create the SDL things... not of interest
	SDLStart(windowWidth, windowHeight);


	// Let's create our Backbuffer
	g_sGLBackBuffer = new Uint32[windowWidth * windowHeight];
	g_sGLDepthBuffer = new float[windowWidth * windowHeight];

	g_sGLBackbufferWidth = windowWidth;
	g_sGLBackbufferHeight = windowHeight;
}

void sGLGenBuffer(int * outBufferIndex)
{
	if (g_sGLNumBuffers < 32)
	{
		(*outBufferIndex) = g_sGLNumBuffers;
		g_sGLNumBuffers++;
	}
	else
	{
		(*outBufferIndex) = -1;
	}
}

void sGLBindBuffer(int bufferID)
{
	g_sGLBoundBuffer = bufferID;
}

void sGLBufferData(int size, in_out_vertex_t * vertexData)
{
	g_sGLVertexBuffer[g_sGLBoundBuffer] = new in_out_vertex_t[size];

	for (int i = 0; i < size; ++i)
	{
		g_sGLVertexBuffer[g_sGLBoundBuffer][i] = vertexData[i];
	}
}

void sGLClearDepth(float zz)
{
	g_sGLDepthClear = zz;
}

void sGLEnableDepthTest()
{
	g_sGLDepthTest = true;
}

void sGLUseVertexShader(in_out_vertex_t(*inVS)(in_out_vertex_t))
{
	g_sGLVertexShader = inVS;
}

void sGLUsePixelShader(in_out_pixel_t(*inPS)(in_out_pixel_t))
{
	g_sGLPixelShader = inPS;
}

void sGLSetPixel(int x, int y, float z, color_t color)
{
	Uint32 iColor = ((uint8_t)(255.0f * color.b) << 16) |
		((uint8_t)(255.0f * color.g) << 8) |
		((uint8_t)(255.0f * color.r)) & 0xffffff;

	// We check if we are actually setting the Pixel inside the Backbuffer
	if (x >= 0 && x < g_sGLBackbufferWidth)
	{
		if (y >= 0 && y < g_sGLBackbufferHeight)
		{
			// Finally, we check if we violate the Depth Buffer
			if (z <= g_sGLDepthBuffer[y * g_sGLBackbufferWidth + x])
			{
				g_sGLBackBuffer[y * g_sGLBackbufferWidth + x] = iColor;
				g_sGLDepthBuffer[y * g_sGLBackbufferWidth + x] = z;
			}
		}
	}
}

void sGLScanLine(int x0, int x1, int y, in_out_pixel_t * tris)
{
	// We determine which Pixels need to be drawn by Scanning each line in the
	// Triangle.

	int sx = x0 < x1 ? x0 : x1;
	int ex = x0 < x1 ? x1 : x0;

	for (int xx = sx; xx <= ex; ++xx)
	{
		in_out_pixel_t currentPixel;
		float u, v, w;

		// For each Pixel we draw, we first Calculate the Barycentric Coordinates
		// https://en.wikipedia.org/wiki/Barycentric_coordinate_system
		// Which are helpful for interpolation inside the Triangle

		calculateBarycentric(xx, y, tris, &u, &v, &w);

		currentPixel.x = xx;
		currentPixel.y = y;

		// We can then interpolate the Color across the Triangle, as well as the 
		// actual u and v texture mapping
		currentPixel.color.r = tris[0].color.r * u + tris[1].color.r * v + tris[2].color.r * w;
		currentPixel.color.g = tris[0].color.g * u + tris[1].color.g * v + tris[2].color.g * w;
		currentPixel.color.b = tris[0].color.b * u + tris[1].color.b * v + tris[2].color.b * w;

		currentPixel.u = tris[0].u * u + tris[1].u * v + tris[2].u * w;
		currentPixel.v = tris[0].v * u + tris[1].v * v + tris[2].v * w;

		// We feed this Pixel information into the Pixel Shader
		currentPixel = g_sGLPixelShader(currentPixel);

		// And finally draw the last Pixel onto the Back Buffer
		// Continue on at - sGLSetPixel -
		sGLSetPixel(currentPixel.x, currentPixel.y, currentPixel.z, currentPixel.color);
	}
}

void sGLFillTopFlatTriangle(int * xx, int * yy, in_out_pixel_t * tris)
{
	// Taken from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
	float invslope1 = (float)(xx[2] - xx[0]) / (float)(yy[2] - yy[0]);
	float invslope2 = (float)(xx[2] - xx[1]) / (float)(yy[2] - yy[1]);

	float curx1 = (float) xx[2];
	float curx2 = (float) xx[2];

	for (int scanlineY = yy[2]; scanlineY > yy[0]; scanlineY--)
	{
		curx1 -= invslope1;
		curx2 -= invslope2;
		sGLScanLine((int)curx1, (int)curx2, scanlineY, tris);
	}
}


void sGLFillBottomFlatTriangle(int * xx, int * yy, in_out_pixel_t * tris)
{
	// Taken from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
	float invslope1 = (float)(xx[1] - xx[0]) / (float)(yy[1] - yy[0]);
	float invslope2 = (float)(xx[2] - xx[0]) / (float)(yy[2] - yy[0]);

	float curx1 = (float) xx[0];
	float curx2 = (float) xx[0];

	for (int scanlineY = yy[0]; scanlineY <= yy[1]; scanlineY++)
	{
		sGLScanLine((int)curx1, (int)curx2, scanlineY, tris);
		curx1 += invslope1;
		curx2 += invslope2;
	}
}

void sGLDrawElements(int count)
{
	// We loop each Triangle we need to draw
	for (int i = 0; i < count; ++i)
	{
		in_out_pixel_t verts[3];

		// First, we Apply the Vertex Shader to each of the 3 Vertices
		for (int j = 0; j < 3; ++j)
		{
			in_out_vertex_t currentVertex;

			currentVertex = g_sGLVertexShader(g_sGLVertexBuffer[g_sGLBoundBuffer][i * 3 + j]);

			verts[j].x = (int)(((currentVertex.x + 1.0f) / 2.0f) * (float)g_sGLBackbufferWidth);
			verts[j].y = (int)((1.0f - ((currentVertex.y + 1.0f) / 2.0f)) * (float)g_sGLBackbufferHeight);
			verts[j].z = currentVertex.z;
			verts[j].color = currentVertex.color;
			verts[j].u = currentVertex.u;
			verts[j].v = currentVertex.v;
		}
		
		// And then Scanline every Pixel that needs to be drawn
		// See - sGLScanLine - for Continuation

		// Scanline Algorithm taken from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
		// You can go there for a detailed description of what happens below

		int xx[3];
		int yy[3];

		xx[0] = verts[0].x;
		yy[0] = verts[0].y;
		xx[1] = verts[1].x;
		yy[1] = verts[1].y;
		xx[2] = verts[2].x;
		yy[2] = verts[2].y;

		sortAscendingY(xx, yy);

		if (yy[1] == yy[2])
		{
			sGLFillBottomFlatTriangle(xx, yy, verts);
		}
		else if (yy[0] == yy[1])
		{
			sGLFillTopFlatTriangle(xx, yy, verts);
		}
		else
		{
			int xxTemp[3];
			int yyTemp[3];

			xxTemp[0] = xx[0];
			yyTemp[0] = yy[0];

			xxTemp[1] = xx[1];
			yyTemp[1] = yy[1];

			xxTemp[2] = (int)(xx[0] + ((float)(yy[1] - yy[0]) / (float)(yy[2] - yy[0])) * (xx[2] - xx[0]));
			yyTemp[2] = yy[1];

			sGLFillBottomFlatTriangle(xxTemp, yyTemp, verts);

			xxTemp[0] = xx[1];
			yyTemp[0] = yy[1];

			xxTemp[1] = xxTemp[2];
			yyTemp[1] = yyTemp[2];

			xxTemp[2] = xx[2];
			yyTemp[2] = yy[2];

			sGLFillTopFlatTriangle(xxTemp, yyTemp, verts);
		}
	}
}


void sGLSwapBuffers()
{
	// Swap our Backbuffer with the Frontbuffer
	SDLSwapBuffers(g_sGLBackBuffer);
}



void sGLClear()
{
	for (int i = 0; i < g_sGLBackbufferHeight; ++i)
	{
		for (int j = 0; j < g_sGLBackbufferWidth; ++j)
		{	
			// Let's clear the Backbuffer to a white Color: 255, 255, 255(, 255)
			g_sGLBackBuffer[i * g_sGLBackbufferWidth + j] = 0xffffff;

			// And the Depth Buffer to the Set value
			g_sGLDepthBuffer[i * g_sGLBackbufferWidth + j] = g_sGLDepthClear;
		}
	}
}




void sGLExit()
{
	delete[] g_sGLBackBuffer;
	delete[] g_sGLDepthBuffer;

	for (int i = 0; i < g_sGLNumBuffers; ++i)
	{
		delete[] g_sGLVertexBuffer[i];
	}

	SDLEnd();
}