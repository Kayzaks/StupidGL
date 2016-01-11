
// You will require the SDL2 Library to Compile
// SDL2 is _ONLY_ used to draw the Backbuffer on the Screen.
// Everything "3D" is done in our Software Rasterizer "sGL" below

// NOTE: This is not meant to run at any usable Framerate!

// - Michael Kissner (@Spellwrath)

#include <math.h>

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

// "uniforms"
float fRotation = 0.0f;


in_out_vertex_t vertexShader(in_out_vertex_t inVertex)
{
	in_out_vertex_t tempV;
	in_out_vertex_t out = inVertex;

	// WORLD - Transformation

	// Just a simple rotation of the Vertices
	tempV.x = inVertex.z * sin(fRotation) + inVertex.x * cos(fRotation);
	tempV.y = inVertex.y;
	tempV.z = inVertex.z * cos(fRotation) - inVertex.x * sin(fRotation);



	// VIEW - Transformation

	// Move the "Camera" back a bit so that the Triangles are in full view
	tempV.z = tempV.z + 0.5f;



	// PROJECTION - Transformation
	
	// Quick and dirty "projection" to get a quasi-perspective look.
	float ftan = tan(1.5707f / 2.0f) / (tempV.z);
	out.x = tempV.x * ftan;
	out.y = tempV.y * ftan;
	out.z = tempV.z;
	out.w = 1 / out.z;

	// And blend by depth - because we can.
	out.color.a = 1.0f - out.z;

	return out;
}


in_out_pixel_t pixelShader(in_out_pixel_t inPixel)
{
	in_out_pixel_t out = inPixel;

	// We Point "sample" the Texture
	int texU = (int)(inPixel.u * TEXTURE_SIZE);
	int texV = (int)(inPixel.v * TEXTURE_SIZE);
	out.color = g_TextureSampler[texU][texV];

	// ... and overlay the Vertex Color
	out.color.r = out.color.r * inPixel.color.r;
	out.color.g = out.color.g * inPixel.color.g;
	out.color.b = out.color.b * inPixel.color.b;

	out.color.a = inPixel.color.a;

	return out;
}


/* ----------------------------------------------- The Main Loop------------------------------------------*/


int main(int argc, char ** argv)
{
	sGLInit(640, 480);

	in_out_vertex_t vertices[6] =
	{
		in_out_vertex_t(-0.3f, -0.3f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f),
		in_out_vertex_t(0.3f, -0.3f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f),
		in_out_vertex_t(0.3f, 0.3f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f),
		
		in_out_vertex_t(-0.3f, 0.3f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f),
		in_out_vertex_t(-0.3f, -0.3f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f),
		in_out_vertex_t(0.3f, 0.3f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f),
	};

	int bufferID;

	sGLGenBuffer(&bufferID);
	sGLBindBuffer(bufferID);
	sGLBufferData(6, vertices);

	sGLClearDepth(1.0f);
	sGLEnableDepthTest();

	float rotateScene = 0.0f;

	while (isRunning())
	{
		sGLClear();


		sGLBindBuffer(bufferID);

		sGLUseVertexShader(vertexShader);
		sGLUsePixelShader(pixelShader);



		sGLDisableAlphaTest();

		sGLUniform1f(&fRotation, 0.0f);

		sGLDrawElements(2);



		sGLEnableAlphaTest();

		rotateScene = rotateScene + 0.01f;
		sGLUniform1f(&fRotation, rotateScene);

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

color_t * g_sGLBackBuffer;
float * g_sGLDepthBuffer;
int g_sGLBackbufferWidth;
int g_sGLBackbufferHeight;

in_out_vertex_t(*g_sGLVertexShader)(in_out_vertex_t);
in_out_pixel_t(*g_sGLPixelShader)(in_out_pixel_t);

// Let's fix the maximum amount of Buffers at some random number: 32
in_out_vertex_t *g_sGLVertexBuffer[32];
int g_sGLNumBuffers = 0;
int g_sGLBoundBuffer = 0;

bool g_sGLAlphaTest = false;

bool g_sGLDepthTest = false;
float g_sGLDepthClear = 1.0f;



void sGLInit(int windowWidth, int windowHeight)
{
	// Create the SDL things... not of interest
	SDLStart(windowWidth, windowHeight);

	// Let's create our Backbuffer
	g_sGLBackBuffer = new color_t[windowWidth * windowHeight];
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

void sGLEnableAlphaTest()
{
	g_sGLAlphaTest = true;
}

void sGLDisableAlphaTest()
{
	g_sGLAlphaTest = false;
}


void sGLUseVertexShader(in_out_vertex_t(*inVS)(in_out_vertex_t))
{
	g_sGLVertexShader = inVS;
}

void sGLUsePixelShader(in_out_pixel_t(*inPS)(in_out_pixel_t))
{
	g_sGLPixelShader = inPS;
}

void sGLUniform1f(float * location, float value)
{
	// Fake setting of uniforms
	(*location) = value;
}

void sGLSetPixel(int x, int y, float z, color_t color)
{
	// We check if we are actually setting the Pixel inside the Backbuffer
	if (x >= 0 && x < g_sGLBackbufferWidth)
	{
		if (y >= 0 && y < g_sGLBackbufferHeight)
		{
			// We check if we violate the Depth Buffer
			if (z > 0 && z <= g_sGLDepthBuffer[y * g_sGLBackbufferWidth + x] || g_sGLDepthTest == false)
			{
				// And finally Alpha blending

				if (g_sGLAlphaTest == true)
				{
					color_t cCol = g_sGLBackBuffer[y * g_sGLBackbufferWidth + x];

					cCol.r = cCol.r * (1 - color.a) + color.r * color.a;
					cCol.g = cCol.g * (1 - color.a) + color.g * color.a;
					cCol.b = cCol.b * (1 - color.a) + color.b * color.a;

					g_sGLBackBuffer[y * g_sGLBackbufferWidth + x] = cCol;
				}
				else
				{
					g_sGLBackBuffer[y * g_sGLBackbufferWidth + x] = color;
				}

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

	// sx = left edge --- ex = right edge of the current Scanline
	for (int x = sx; x <= ex; ++x)
	{
		in_out_pixel_t currentPixel;
		float lambda1, lambda2, lambda3;

		// For each Pixel we draw, we first Calculate the Barycentric Coordinates
		// https://en.wikipedia.org/wiki/Barycentric_coordinate_system
		// Which are helpful for interpolation inside the Triangle
		
		calculateBarycentric(x, y, tris, &lambda1, &lambda2, &lambda3);

		currentPixel.x = x;
		currentPixel.y = y;

		// We can then interpolate the Color, uv and z coordinate across the Triangle
		currentPixel.color.r = tris[0].color.r * lambda1 + tris[1].color.r * lambda2 + tris[2].color.r * lambda3;
		currentPixel.color.g = tris[0].color.g * lambda1 + tris[1].color.g * lambda2 + tris[2].color.g * lambda3;
		currentPixel.color.b = tris[0].color.b * lambda1 + tris[1].color.b * lambda2 + tris[2].color.b * lambda3;
		currentPixel.color.a = tris[0].color.a * lambda1 + tris[1].color.a * lambda2 + tris[2].color.a * lambda3;

		currentPixel.z = tris[0].z * lambda1 + tris[1].z * lambda2 + tris[2].z * lambda3;
		currentPixel.w = tris[0].w * lambda1 + tris[1].w * lambda2 + tris[2].w * lambda3;

		// Moving back to Screen-Affine Space
		currentPixel.u = (tris[0].u * lambda1 + tris[1].u * lambda2 + tris[2].u * lambda3) / currentPixel.w;
		currentPixel.v = (tris[0].v * lambda1 + tris[1].v * lambda2 + tris[2].v * lambda3) / currentPixel.w;


		// You might notice that our Texture is "distorted" once the Triangle rotates. This is due
		// the fact that we are working with an affine Transformation for our Texture coordinates
		// (the typical Playstation 1 look) yet do a perspective Transformation on the triangle itself.
		// I've left it affine for simplicity and the fact that we don't do a real Projection (which is needed).
		// You can read more about it here:
		// https://en.wikipedia.org/wiki/Texture_mapping

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

			// Transform the float coordinates to int's for the screen
			verts[j].x = (int)(((currentVertex.x + 1.0f) / 2.0f) * (float)g_sGLBackbufferWidth);
			verts[j].y = (int)((1.0f - ((currentVertex.y + 1.0f) / 2.0f)) * (float)g_sGLBackbufferHeight);

			// For perspective correct Texture mapping, we need: (i.e. moving to Object-Affine Space)
			verts[j].u = currentVertex.u * currentVertex.w;
			verts[j].v = currentVertex.v * currentVertex.w;

			// We can copy the rest directly
			verts[j].z = currentVertex.z;
			verts[j].w = currentVertex.w;
			verts[j].color = currentVertex.color;

		}
		
		// And then Scanline every Pixel that needs to be drawn
		// See - sGLScanLine - for Continuation

		// Scanline Algorithm taken from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
		// You can check there for a detailed description of what happens below

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
			// Let's clear the Backbuffer to a white Color
			g_sGLBackBuffer[i * g_sGLBackbufferWidth + j] = color_t(1.0f, 1.0f, 1.0f);

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