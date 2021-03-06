// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include "GlyphExtractor.h"

// specify that we want the OpenGL core profile before including GLFW headers
#ifndef LAB_LINUX
	#include <glad/glad.h>
#else
	#define GLFW_INCLUDE_GLCOREARB
	#define GL_GLEXT_PROTOTYPES
#endif
#include <GLFW/glfw3.h>

static int level = 1; // Level of program
static float rate = 0.01; // rate of movement


/*
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
*/

using namespace std;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint TCSshader, GLuint TESshader, GLuint fragmentShader);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);
// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

struct MyShader
{
	// OpenGL names for vertex and fragment shaders, shader program
	GLuint  vertex;
	GLuint  TCS;
	GLuint  TES;
	GLuint  fragment;
	GLuint  program;
	GLuint  programNoTess;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0),  program(0), programNoTess(0)
	{}
};

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader)
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");
	string TCSSource = LoadSource("tessControl.glsl");
	string TESSource = LoadSource("tessEval.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	shader->TCS = CompileShader(GL_TESS_CONTROL_SHADER, TCSSource);
	shader->TES = CompileShader(GL_TESS_EVALUATION_SHADER, TESSource);

	// link shader program
	shader->program = LinkProgram(shader->vertex, shader->TCS, shader->TES, shader->fragment);
	shader->programNoTess = LinkProgram(shader->vertex, shader->fragment);
	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader->program);
	glDeleteProgram(shader->programNoTess);
	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);
	glDeleteShader(shader->TCS);
	glDeleteShader(shader->TES);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing textures
/*
struct MyTexture
{
	GLuint textureID;
	GLuint target;
	int width;
	int height;

	// initialize object names to zero (OpenGL reserved value)
	MyTexture() : textureID(0), target(0), width(0), height(0)
	{}
};

bool InitializeTexture(MyTexture* texture, const char* filename, GLuint target = GL_TEXTURE_2D)
{
	int numComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename, &texture->width, &texture->height, &numComponents, 0);
	if (data != nullptr)
	{
		texture->target = target;
		glGenTextures(1, &texture->textureID);
		glBindTexture(texture->target, texture->textureID);
		GLuint format = numComponents == 3 ? GL_RGB : GL_RGBA;
		glTexImage2D(texture->target, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);

		// Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
		// GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Clean up
		glBindTexture(texture->target, 0);
		stbi_image_free(data);
		return !CheckGLErrors();
	}
	return true; //error
}

// deallocate texture-related objects
void DestroyTexture(MyTexture *texture)
{
	glBindTexture(texture->target, 0);
	glDeleteTextures(1, &texture->textureID);
}

void SaveImage(const char* filename, int width, int height, unsigned char *data, int numComponents = 3, int stride = 0)
{
	if (!stbi_write_png(filename, width, height, numComponents, data, stride))
		cout << "Unable to save image: " << filename << endl;
}*/

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

void GeneratePoint(MyGeometry *geometry, MyShader *shader, GLfloat (*coordinates)[2], GLfloat (*colour)[3])
{
	GLfloat vertices[][2] = {
		{ coordinates[0][0], coordinates[0][1]}
	};

	GLfloat colours[][3] = {
		{ colour[0][0], colour[0][1], colour[0][2] }
	};

	geometry->elementCount = 1;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	CheckGLErrors();
}

void Generateline(MyGeometry *geometry, MyShader *shader, GLfloat (*coordinates)[2], GLfloat (*colour)[3])
{
	GLfloat vertices[][2] = {
		{ coordinates[0][0], coordinates[0][1]},
		{ coordinates[1][0], coordinates[1][1]}
	};

	GLfloat colours[][3] = {
		{ colour[0][0], colour[0][1], colour[0][2] },
		{ colour[1][0], colour[1][1], colour[1][2] }
	};

	geometry->elementCount = 2;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	CheckGLErrors();
}

void GenerateQuadratic(MyGeometry *geometry, MyShader *shader, GLfloat (*coordinates)[2], GLfloat (*colour)[3])
{
	GLfloat vertices[][2] = {
		{ coordinates[0][0], coordinates[0][1]},
		{ coordinates[1][0], coordinates[1][1]},
		{ coordinates[2][0], coordinates[2][1]}
	};

	GLfloat colours[][3] = {
		{ colour[0][0], colour[0][1], colour[0][2] },
		{ colour[1][0], colour[1][1], colour[1][2] },
		{ colour[2][0], colour[2][1], colour[2][2] }
	};

	geometry->elementCount = 3;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	CheckGLErrors();
}

void GenerateCubic(MyGeometry *geometry, MyShader *shader, GLfloat (*coordinates)[2], GLfloat (*colour)[3])
{
	GLfloat vertices[][2] = {
		{ coordinates[0][0], coordinates[0][1]},
		{ coordinates[1][0], coordinates[1][1]},
		{ coordinates[2][0], coordinates[2][1]},
		{ coordinates[3][0], coordinates[3][1]}
	};

	GLfloat colours[][3] = {
		{ colour[0][0], colour[0][1], colour[0][2] },
		{ colour[1][0], colour[1][1], colour[1][2] },
		{ colour[2][0], colour[2][1], colour[2][2] },
		{ colour[3][0], colour[3][1], colour[3][2] }
	};

	geometry->elementCount = 4;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	CheckGLErrors();
}
// create buffers and fill with geometry data, returning true if successful
/*bool InitializeGeometry(MyGeometry *geometry)
{
	// three vertex positions and assocated colours of a triangle
	const GLfloat vertices[][2] = {
		{ -4.0/50.0, 4.0/50.0 },
		{ 8.0/50.0, -4.0/50.0 },
		{ 0.0/50.0, -4.0/50.0 }
	};

	const GLfloat colours[][3] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f }
	};
	geometry->elementCount = 3;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}
*/

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
	glDeleteBuffers(1, &geometry->colourBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

/*void RenderScene(MyGeometry *geometry, MyShader *shader)
{
	// clear screen to a dark grey colour
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_PATCHES, 0, geometry->elementCount);
	glUseProgram(shader->programNoTess);
	glDrawArrays(GL_LINE_STRIP, 0, geometry->elementCount);
	glDrawArrays(GL_POINTS, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}*/

void RenderBezier(MyGeometry *geometry, MyShader *shader)
{
	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_PATCHES, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

void RenderControlPoints(MyGeometry *geometry, MyShader *shader)
{
	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->programNoTess);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_POINTS, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

void RenderControlLines(MyGeometry *geometry, MyShader *shader)
{
	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->programNoTess);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_LINE_STRIP, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

void ClearScene(MyGeometry *geometry, MyShader *shader)
{
    // clear screen to a dark grey colour
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    CheckGLErrors();
}

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS)
    {
      switch (key)
      {
        case GLFW_KEY_RIGHT :
        if (level < 8)
          level++;
				else
					level = 1;
        break;
        case GLFW_KEY_LEFT :
        if (level > 1)
          level--;
				else
					level = 8;
        break;
				case GLFW_KEY_UP :
        	rate = rate + 0.01;
        break;
				case GLFW_KEY_DOWN :
        	rate = rate - 0.01;
        break;
      }
    }
}

// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(1920, 1080, "CPSC 453 Assignment #3", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwMakeContextCurrent(window);

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	MyShader shader;
	if (!InitializeShaders(&shader)) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}

	// call function to create and fill buffers with geometry data
	MyGeometry geometry;
//	if (!InitializeGeometry(&geometry))
//		cout << "Program failed to intialize geometry!" << endl;

	glPointSize(5);

	GLint loc1 = glGetUniformLocation(shader.program, "curves");
	GLint loc2 = glGetUniformLocation(shader.programNoTess, "curves");

	GLfloat vertices[4][2];
	GLfloat colours[4][3];
	float scale = 10.0f;

	float move = 1.0;
	bool end = false;
	bool g = false;

	//Load a font file and extract a glyph
	GlyphExtractor extractor;

	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{

		ClearScene(&geometry, &shader);

		switch (level)
		{
			case 1 :
			case 2 :

			glUseProgram(shader.program);
			glUniform2f(loc1, 0.0, 0.0);
			glUseProgram(shader.programNoTess);
			glUniform2f(loc2, 0.0, 0.0);

			glPatchParameteri(GL_PATCH_VERTICES, 3);

		vertices[0][0] = 1.0/scale; vertices[0][1] = 1.0/scale;
		vertices[1][0] = 2.0/scale;  vertices[1][1] = -1.0/scale;
		vertices[2][0] = 0.0/scale;  vertices[2][1] = -1.0/scale;

		if(level == 2){
			for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
			colours[0][0] = 1.0f; colours[0][1] = 1.0f; colours[1][0] = 1.0f; colours[1][1] = 1.0f; colours[2][0] = 1.0f; colours[2][1] = 1.0f;
			GenerateQuadratic(&geometry, &shader, vertices, colours);
			RenderControlLines(&geometry, &shader);
			for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
			colours[0][1] = 1.0f; colours[1][2] = 1.0f; colours[2][1] = 1.0f;
			GenerateQuadratic(&geometry, &shader, vertices, colours);
			RenderControlPoints(&geometry, &shader);
		}

		for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
		colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f;

		GenerateQuadratic(&geometry, &shader, vertices, colours);
		RenderBezier(&geometry, &shader);

		vertices[0][0] = 0.0/scale; vertices[0][1] = -1.0/scale;
		vertices[1][0] = -2.0/scale;  vertices[1][1] = -1.0/scale;
		vertices[2][0] = -1.0/scale;  vertices[2][1] = 1.0/scale;

		if(level == 2){
			for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
			colours[0][0] = 1.0f; colours[0][1] = 1.0f; colours[1][0] = 1.0f; colours[1][1] = 1.0f; colours[2][0] = 1.0f; colours[2][1] = 1.0f;
			GenerateQuadratic(&geometry, &shader, vertices, colours);
			RenderControlLines(&geometry, &shader);
			for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
			colours[0][1] = 1.0f; colours[1][2] = 1.0f; colours[2][1] = 1.0f;
			GenerateQuadratic(&geometry, &shader, vertices, colours);
			RenderControlPoints(&geometry, &shader);
		}

		for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
		colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f;

		GenerateQuadratic(&geometry, &shader, vertices, colours);
		RenderBezier(&geometry, &shader);

		vertices[0][0] = -1.0/scale; vertices[0][1] = 1.0/scale;
		vertices[1][0] = 0.0/scale;  vertices[1][1] = 1.0/scale;
		vertices[2][0] = 1.0/scale;  vertices[2][1] = 1.0/scale;

		if(level == 2){
			for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
			colours[0][0] = 1.0f; colours[0][1] = 1.0f; colours[1][0] = 1.0f; colours[1][1] = 1.0f; colours[2][0] = 1.0f; colours[2][1] = 1.0f;
			GenerateQuadratic(&geometry, &shader, vertices, colours);
			RenderControlLines(&geometry, &shader);
			for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
			colours[0][1] = 1.0f; colours[1][2] = 1.0f; colours[2][1] = 1.0f;
			GenerateQuadratic(&geometry, &shader, vertices, colours);
			RenderControlPoints(&geometry, &shader);
		}

		for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
		colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f;

		GenerateQuadratic(&geometry, &shader, vertices, colours);
		RenderBezier(&geometry, &shader);

		vertices[0][0] = 1.2/scale; vertices[0][1] = 0.5/scale;
		vertices[1][0] = 2.5/scale;  vertices[1][1] = 1.0/scale;
		vertices[2][0] = 1.3/scale;  vertices[2][1] = -0.4/scale;

		if(level == 2){
			for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
			colours[0][0] = 1.0f; colours[0][1] = 1.0f; colours[1][0] = 1.0f; colours[1][1] = 1.0f; colours[2][0] = 1.0f; colours[2][1] = 1.0f;
			GenerateQuadratic(&geometry, &shader, vertices, colours);
			RenderControlLines(&geometry, &shader);
			for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
			colours[0][1] = 1.0f; colours[1][2] = 1.0f; colours[2][1] = 1.0f;
			GenerateQuadratic(&geometry, &shader, vertices, colours);
			RenderControlPoints(&geometry, &shader);
		}

		for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
		colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f;

		GenerateQuadratic(&geometry, &shader, vertices, colours);
		RenderBezier(&geometry, &shader);


			break;
			case 3 :
			case 4 :

			glUseProgram(shader.program);
			glUniform2f(loc1, 1.0, 0.0);
			glUseProgram(shader.programNoTess);
			glUniform2f(loc2, 1.0, 0.0);

			glPatchParameteri(GL_PATCH_VERTICES, 4);

			vertices[0][0] = 1.0/scale;  vertices[0][1] = 1.0/scale;
			vertices[1][0] = 4.0/scale;  vertices[1][1] = 0.0/scale;
			vertices[2][0] = 6.0/scale;  vertices[2][1] = 2.0/scale;
			vertices[3][0] = 9.0/scale;  vertices[3][1] = 1.0/scale;

			if(level == 4){
				for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
				colours[0][0] = 1.0f; colours[0][1] = 1.0f; colours[1][0] = 1.0f; colours[1][1] = 1.0f; colours[2][0] = 1.0f; colours[2][1] = 1.0f; colours[3][0] = 1.0f; colours[3][1] = 1.0f;
				GenerateCubic(&geometry, &shader, vertices, colours);
				RenderControlLines(&geometry, &shader);
				for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
				colours[0][1] = 1.0f; colours[1][2] = 1.0f; colours[2][2] = 1.0f; colours[3][1] = 1.0f;
				GenerateCubic(&geometry, &shader, vertices, colours);
				RenderControlPoints(&geometry, &shader);
			}

			for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
			colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f; colours[3][0] = 1.0f;

			GenerateCubic(&geometry, &shader, vertices, colours);
			RenderBezier(&geometry, &shader);

			vertices[0][0] = 8.0/scale;  vertices[0][1] = 2.0/scale;
			vertices[1][0] = 0.0/scale;  vertices[1][1] = 8.0/scale;
			vertices[2][0] = 0.0/scale;  vertices[2][1] = -2.0/scale;
			vertices[3][0] = 8.0/scale;  vertices[3][1] = 4.0/scale;

			if(level == 4){
				for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
				colours[0][0] = 1.0f; colours[0][1] = 1.0f; colours[1][0] = 1.0f; colours[1][1] = 1.0f; colours[2][0] = 1.0f; colours[2][1] = 1.0f; colours[3][0] = 1.0f; colours[3][1] = 1.0f;
				GenerateCubic(&geometry, &shader, vertices, colours);
				RenderControlLines(&geometry, &shader);
				for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
				colours[0][1] = 1.0f; colours[1][2] = 1.0f; colours[2][2] = 1.0f; colours[3][1] = 1.0f;
				GenerateCubic(&geometry, &shader, vertices, colours);
				RenderControlPoints(&geometry, &shader);
			}

			for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
			colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f; colours[3][0] = 1.0f;

			GenerateCubic(&geometry, &shader, vertices, colours);
			RenderBezier(&geometry, &shader);

			vertices[0][0] = 5.0/scale;  vertices[0][1] = 3.0/scale;
			vertices[1][0] = 3.0/scale;  vertices[1][1] = 2.0/scale;
			vertices[2][0] = 3.0/scale;  vertices[2][1] = 3.0/scale;
			vertices[3][0] = 5.0/scale;  vertices[3][1] = 2.0/scale;

			if(level == 4){
				for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
				colours[0][0] = 1.0f; colours[0][1] = 1.0f; colours[1][0] = 1.0f; colours[1][1] = 1.0f; colours[2][0] = 1.0f; colours[2][1] = 1.0f; colours[3][0] = 1.0f; colours[3][1] = 1.0f;
				GenerateCubic(&geometry, &shader, vertices, colours);
				RenderControlLines(&geometry, &shader);
				for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
				colours[0][1] = 1.0f; colours[1][2] = 1.0f; colours[2][2] = 1.0f; colours[3][1] = 1.0f;
				GenerateCubic(&geometry, &shader, vertices, colours);
				RenderControlPoints(&geometry, &shader);
			}

			for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
			colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f; colours[3][0] = 1.0f;

			GenerateCubic(&geometry, &shader, vertices, colours);
			RenderBezier(&geometry, &shader);

			vertices[0][0] = 3.0/scale;  vertices[0][1] = 2.2/scale;
			vertices[1][0] = 3.5/scale;  vertices[1][1] = 2.7/scale;
			vertices[2][0] = 3.5/scale;  vertices[2][1] = 3.3/scale;
			vertices[3][0] = 3.0/scale;  vertices[3][1] = 3.8/scale;

			if(level == 4){
				for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
				colours[0][0] = 1.0f; colours[0][1] = 1.0f; colours[1][0] = 1.0f; colours[1][1] = 1.0f; colours[2][0] = 1.0f; colours[2][1] = 1.0f; colours[3][0] = 1.0f; colours[3][1] = 1.0f;
				GenerateCubic(&geometry, &shader, vertices, colours);
				RenderControlLines(&geometry, &shader);
				for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
				colours[0][1] = 1.0f; colours[1][2] = 1.0f; colours[2][2] = 1.0f; colours[3][1] = 1.0f;
				GenerateCubic(&geometry, &shader, vertices, colours);
				RenderControlPoints(&geometry, &shader);
			}

			for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
			colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f; colours[3][0] = 1.0f;

			GenerateCubic(&geometry, &shader, vertices, colours);
			RenderBezier(&geometry, &shader);

			vertices[0][0] = 2.8/scale;  vertices[0][1] = 3.5/scale;
			vertices[1][0] = 2.4/scale;  vertices[1][1] = 3.8/scale;
			vertices[2][0] = 2.4/scale;  vertices[2][1] = 3.2/scale;
			vertices[3][0] = 2.8/scale;  vertices[3][1] = 3.5/scale;

			if(level == 4){
				for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
				colours[0][0] = 1.0f; colours[0][1] = 1.0f; colours[1][0] = 1.0f; colours[1][1] = 1.0f; colours[2][0] = 1.0f; colours[2][1] = 1.0f; colours[3][0] = 1.0f; colours[3][1] = 1.0f;
				GenerateCubic(&geometry, &shader, vertices, colours);
				RenderControlLines(&geometry, &shader);
				for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
				colours[0][1] = 1.0f; colours[1][2] = 1.0f; colours[2][2] = 1.0f; colours[3][1] = 1.0f;
				GenerateCubic(&geometry, &shader, vertices, colours);
				RenderControlPoints(&geometry, &shader);
			}

			for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
			colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f; colours[3][0] = 1.0f;

			GenerateCubic(&geometry, &shader, vertices, colours);
			RenderBezier(&geometry, &shader);

			break;
			case 5 :
{
			for (int name = 0; name < 3; name++) {
				float translation;
				if(name == 0){
						extractor.LoadFontFile("fonts/Lora-Regular.ttf");
						translation = 0.75f;}
					else if(name == 1){
						extractor.LoadFontFile("fonts/SourceSansPro-Regular.ttf");
						translation = 0.0f;}
					else {
						extractor.LoadFontFile("fonts/Dattermatter Personal Use.ttf");
						translation = -0.75f;}
				float advance = -1;

			for (int letter = 0; letter < 13; letter++) {
				MyGlyph glyph;
			switch (letter) {
				case 0 :
				glyph = extractor.ExtractGlyph('M'); break;
				case 1 :
				glyph = extractor.ExtractGlyph('a'); break;
				case 2 :
				glyph = extractor.ExtractGlyph('t'); break;
				case 3 :
				glyph = extractor.ExtractGlyph('t'); break;
				case 4 :
				glyph = extractor.ExtractGlyph('h'); break;
				case 5 :
				glyph = extractor.ExtractGlyph('e'); break;
				case 6 :
				glyph = extractor.ExtractGlyph('w'); break;
				case 7 :
				glyph = extractor.ExtractGlyph('H'); break;
				case 8 :
				glyph = extractor.ExtractGlyph('y'); break;
				case 9 :
				glyph = extractor.ExtractGlyph('l'); break;
				case 10 :
				glyph = extractor.ExtractGlyph('t'); break;
				case 11 :
				glyph = extractor.ExtractGlyph('o'); break;
				case 12 :
				glyph = extractor.ExtractGlyph('n'); break;
			}

			for (uint i = 0; i < glyph.contours.size(); i++) {
				for (uint j = 0; j < glyph.contours[i].size(); j++) {
					//cout << j << " " << i << " " << glyph.contours[i][j].degree << " " << glyph.contours[i][j].x[0] << " " << glyph.contours[i][j].x[1] << " " << glyph.contours[i][j].y[0] << " " << glyph.contours[i][j].y[1] << endl;

					uint degree = glyph.contours[i][j].degree;
					switch (degree) {
						case 0:
						vertices[0][0] = (float)(glyph.contours[i][j].x[0]+advance)/scale;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
						for (int i = 0; i < 1; i++){for (int j = 0; j < 1; j++){colours[i][j] = 0.0f;}}
						colours[0][0] = 1.0f;
						GeneratePoint(&geometry, &shader, vertices, colours);
						RenderControlPoints(&geometry, &shader);

						break;
						case 1:
						vertices[0][0] = (float)(glyph.contours[i][j].x[0]+advance)/scale;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
						vertices[1][0] = (float)(glyph.contours[i][j].x[1]+advance)/scale;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;

						for (int i = 0; i < 2; i++){for (int j = 0; j < 2; j++){colours[i][j] = 0.0f;}}
						colours[0][0] = 1.0f; colours[1][0] = 1.0f;

						Generateline(&geometry, &shader, vertices, colours);
						RenderControlLines(&geometry, &shader);
						break;
						case 2:
						glUseProgram(shader.program);
						glUniform2f(loc1, 0.0, 0.0);
						glPatchParameteri(GL_PATCH_VERTICES, 3);

						vertices[0][0] = (float)(glyph.contours[i][j].x[0]+advance)/scale;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
						vertices[1][0] = (float)(glyph.contours[i][j].x[1]+advance)/scale;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;
						vertices[2][0] = (float)(glyph.contours[i][j].x[2]+advance)/scale;  vertices[2][1] = (float)(glyph.contours[i][j].y[2]+(translation*scale))/scale;

						for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
						colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f;

						GenerateQuadratic(&geometry, &shader, vertices, colours);
						RenderBezier(&geometry, &shader);
						break;
						case 3:
						glUseProgram(shader.program);
						glUniform2f(loc1, 1.0, 0.0);
						glPatchParameteri(GL_PATCH_VERTICES, 4);

						vertices[0][0] = (float)(glyph.contours[i][j].x[0]+advance)/scale;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
						vertices[1][0] = (float)(glyph.contours[i][j].x[1]+advance)/scale;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;
						vertices[2][0] = (float)(glyph.contours[i][j].x[2]+advance)/scale;  vertices[2][1] = (float)(glyph.contours[i][j].y[2]+(translation*scale))/scale;
						vertices[3][0] = (float)(glyph.contours[i][j].x[3]+advance)/scale;  vertices[3][1] = (float)(glyph.contours[i][j].y[3]+(translation*scale))/scale;

						for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
						colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f; colours[3][0] = 1.0f;

						GenerateCubic(&geometry, &shader, vertices, colours);
						RenderBezier(&geometry, &shader);
						break;
					}
				}
			}
			advance = advance + glyph.advance;
		}
	}
}
			break;
			case 6:
			if (end){
				move = 1.0;
				end = false;
				g = false;
			}
			else{
					move = move - rate;
				}
{
						extractor.LoadFontFile("fonts/AlexBrush-Regular.ttf");
						float advance = -1;
						float translation = 0.0;

						for (int letter = 0; letter < 36; letter++) {
							MyGlyph glyph;
						switch (letter) {
							case 30 :
							glyph = extractor.ExtractGlyph('a'); break;
							case 9 :
							glyph = extractor.ExtractGlyph('b'); break;
							case 7 :
							glyph = extractor.ExtractGlyph('c'); break;
							case 33 :
							glyph = extractor.ExtractGlyph('d'); break;
							case 3 :
							case 24 :
							case 28 :
							glyph = extractor.ExtractGlyph('e'); break;
							case 14 :
							glyph = extractor.ExtractGlyph('f'); break;
							case 35 :
							glyph = extractor.ExtractGlyph('g'); g = true; break;
							case 2 :
							case 27 :
							glyph = extractor.ExtractGlyph('h'); break;
							case 6 :
							glyph = extractor.ExtractGlyph('i'); break;
							case 17 :
							glyph = extractor.ExtractGlyph('j'); break;
							case 8 :
							glyph = extractor.ExtractGlyph('k'); break;
							case 29 :
							glyph = extractor.ExtractGlyph('l'); break;
							case 19 :
							glyph = extractor.ExtractGlyph('m'); break;
							case 13 :
							glyph = extractor.ExtractGlyph('n'); break;
							case 11 :
							case 15 :
							case 22 :
							case 34 :
							glyph = extractor.ExtractGlyph('o'); break;
							case 20 :
							glyph = extractor.ExtractGlyph('p'); break;
							case 4 :
							glyph = extractor.ExtractGlyph('q'); break;
							case 10 :
							case 25 :
							glyph = extractor.ExtractGlyph('r'); break;
							case 21 :
							glyph = extractor.ExtractGlyph('s'); break;
							case 1 :
							case 26 :
							glyph = extractor.ExtractGlyph('t'); break;
							case 5 :
							case 18 :
							glyph = extractor.ExtractGlyph('u'); break;
							case 23 :
							glyph = extractor.ExtractGlyph('v'); break;
							case 12 :
							glyph = extractor.ExtractGlyph('w'); break;
							case 16 :
							glyph = extractor.ExtractGlyph('x'); break;
							case 32 :
							glyph = extractor.ExtractGlyph('y'); break;
							case 31 :
							glyph = extractor.ExtractGlyph('z'); break;
						}

						for (uint i = 0; i < glyph.contours.size(); i++) {
							for (uint j = 0; j < glyph.contours[i].size(); j++) {
								//cout << j << " " << i << " " << glyph.contours[i][j].degree << " " << glyph.contours[i][j].x[0] << " " << glyph.contours[i][j].x[1] << " " << glyph.contours[i][j].y[0] << " " << glyph.contours[i][j].y[1] << endl;

								uint degree = glyph.contours[i][j].degree;
								switch (degree) {
									case 0:
									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									for (int i = 0; i < 1; i++){for (int j = 0; j < 1; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f;
									GeneratePoint(&geometry, &shader, vertices, colours);
									RenderControlPoints(&geometry, &shader);

									break;
									case 1:
									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									vertices[1][0] = (float)((glyph.contours[i][j].x[1]+advance)/scale)+move;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;

									for (int i = 0; i < 2; i++){for (int j = 0; j < 2; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f; colours[1][0] = 1.0f;

									Generateline(&geometry, &shader, vertices, colours);
									RenderControlLines(&geometry, &shader);
									break;
									case 2:
									glUseProgram(shader.program);
									glUniform2f(loc1, 0.0, 0.0);
									glPatchParameteri(GL_PATCH_VERTICES, 3);

									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									vertices[1][0] = (float)((glyph.contours[i][j].x[1]+advance)/scale)+move;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;
									vertices[2][0] = (float)((glyph.contours[i][j].x[2]+advance)/scale)+move;  vertices[2][1] = (float)(glyph.contours[i][j].y[2]+(translation*scale))/scale;

									for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f;

									GenerateQuadratic(&geometry, &shader, vertices, colours);
									RenderBezier(&geometry, &shader);

									if(g)
										if(vertices[0][0] < -3.0)
											end = true;
									break;
									case 3:
									glUseProgram(shader.program);
									glUniform2f(loc1, 1.0, 0.0);
									glPatchParameteri(GL_PATCH_VERTICES, 4);

									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									vertices[1][0] = (float)((glyph.contours[i][j].x[1]+advance)/scale)+move;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;
									vertices[2][0] = (float)((glyph.contours[i][j].x[2]+advance)/scale)+move;  vertices[2][1] = (float)(glyph.contours[i][j].y[2]+(translation*scale))/scale;
									vertices[3][0] = (float)((glyph.contours[i][j].x[3]+advance)/scale)+move;  vertices[3][1] = (float)(glyph.contours[i][j].y[3]+(translation*scale))/scale;

									for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f; colours[3][0] = 1.0f;

									GenerateCubic(&geometry, &shader, vertices, colours);
									RenderBezier(&geometry, &shader);
									break;
								}
							}
						}
						advance = advance + glyph.advance;
					}
				}
			break;
			case 7:
			if (end){
				move = 1.0;
				end = false;
				g = false;
			}
			else{
					move = move - rate;
				}
{
						extractor.LoadFontFile("fonts/Inconsolata-Regular.ttf");
						float advance = -1;
						float translation = 0.0;

						for (int letter = 0; letter < 36; letter++) {
							MyGlyph glyph;
						switch (letter) {
							case 30 :
							glyph = extractor.ExtractGlyph('a'); break;
							case 9 :
							glyph = extractor.ExtractGlyph('b'); break;
							case 7 :
							glyph = extractor.ExtractGlyph('c'); break;
							case 33 :
							glyph = extractor.ExtractGlyph('d'); break;
							case 3 :
							case 24 :
							case 28 :
							glyph = extractor.ExtractGlyph('e'); break;
							case 14 :
							glyph = extractor.ExtractGlyph('f'); break;
							case 35 :
							glyph = extractor.ExtractGlyph('g'); g = true; break;
							case 2 :
							case 27 :
							glyph = extractor.ExtractGlyph('h'); break;
							case 6 :
							glyph = extractor.ExtractGlyph('i'); break;
							case 17 :
							glyph = extractor.ExtractGlyph('j'); break;
							case 8 :
							glyph = extractor.ExtractGlyph('k'); break;
							case 29 :
							glyph = extractor.ExtractGlyph('l'); break;
							case 19 :
							glyph = extractor.ExtractGlyph('m'); break;
							case 13 :
							glyph = extractor.ExtractGlyph('n'); break;
							case 11 :
							case 15 :
							case 22 :
							case 34 :
							glyph = extractor.ExtractGlyph('o'); break;
							case 20 :
							glyph = extractor.ExtractGlyph('p'); break;
							case 4 :
							glyph = extractor.ExtractGlyph('q'); break;
							case 10 :
							case 25 :
							glyph = extractor.ExtractGlyph('r'); break;
							case 21 :
							glyph = extractor.ExtractGlyph('s'); break;
							case 1 :
							case 26 :
							glyph = extractor.ExtractGlyph('t'); break;
							case 5 :
							case 18 :
							glyph = extractor.ExtractGlyph('u'); break;
							case 23 :
							glyph = extractor.ExtractGlyph('v'); break;
							case 12 :
							glyph = extractor.ExtractGlyph('w'); break;
							case 16 :
							glyph = extractor.ExtractGlyph('x'); break;
							case 32 :
							glyph = extractor.ExtractGlyph('y'); break;
							case 31 :
							glyph = extractor.ExtractGlyph('z'); break;
						}

						for (uint i = 0; i < glyph.contours.size(); i++) {
							for (uint j = 0; j < glyph.contours[i].size(); j++) {
								//cout << j << " " << i << " " << glyph.contours[i][j].degree << " " << glyph.contours[i][j].x[0] << " " << glyph.contours[i][j].x[1] << " " << glyph.contours[i][j].y[0] << " " << glyph.contours[i][j].y[1] << endl;

								uint degree = glyph.contours[i][j].degree;
								switch (degree) {
									case 0:
									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									for (int i = 0; i < 1; i++){for (int j = 0; j < 1; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f;
									GeneratePoint(&geometry, &shader, vertices, colours);
									RenderControlPoints(&geometry, &shader);

									break;
									case 1:
									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									vertices[1][0] = (float)((glyph.contours[i][j].x[1]+advance)/scale)+move;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;

									for (int i = 0; i < 2; i++){for (int j = 0; j < 2; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f; colours[1][0] = 1.0f;

									Generateline(&geometry, &shader, vertices, colours);
									RenderControlLines(&geometry, &shader);
									break;
									case 2:
									glUseProgram(shader.program);
									glUniform2f(loc1, 0.0, 0.0);
									glPatchParameteri(GL_PATCH_VERTICES, 3);

									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									vertices[1][0] = (float)((glyph.contours[i][j].x[1]+advance)/scale)+move;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;
									vertices[2][0] = (float)((glyph.contours[i][j].x[2]+advance)/scale)+move;  vertices[2][1] = (float)(glyph.contours[i][j].y[2]+(translation*scale))/scale;

									for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f;

									GenerateQuadratic(&geometry, &shader, vertices, colours);
									RenderBezier(&geometry, &shader);

									if(g)
										if(vertices[0][0] < -3.0)
											end = true;
									break;
									case 3:
									glUseProgram(shader.program);
									glUniform2f(loc1, 1.0, 0.0);
									glPatchParameteri(GL_PATCH_VERTICES, 4);

									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									vertices[1][0] = (float)((glyph.contours[i][j].x[1]+advance)/scale)+move;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;
									vertices[2][0] = (float)((glyph.contours[i][j].x[2]+advance)/scale)+move;  vertices[2][1] = (float)(glyph.contours[i][j].y[2]+(translation*scale))/scale;
									vertices[3][0] = (float)((glyph.contours[i][j].x[3]+advance)/scale)+move;  vertices[3][1] = (float)(glyph.contours[i][j].y[3]+(translation*scale))/scale;

									for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f; colours[3][0] = 1.0f;

									GenerateCubic(&geometry, &shader, vertices, colours);
									RenderBezier(&geometry, &shader);
									break;
								}
							}
						}
						advance = advance + glyph.advance;
					}
				}
			break;
			case 8:
			if (end){
				move = 1.0;
				end = false;
				g = false;
			}
			else{
					move = move - rate;
				}
{
						extractor.LoadFontFile("fonts/FugazOne-Regular.ttf");
						float advance = -1;
						float translation = 0.0;

						for (int letter = 0; letter < 36; letter++) {
							MyGlyph glyph;
						switch (letter) {
							case 30 :
							glyph = extractor.ExtractGlyph('a'); break;
							case 9 :
							glyph = extractor.ExtractGlyph('b'); break;
							case 7 :
							glyph = extractor.ExtractGlyph('c'); break;
							case 33 :
							glyph = extractor.ExtractGlyph('d'); break;
							case 3 :
							case 24 :
							case 28 :
							glyph = extractor.ExtractGlyph('e'); break;
							case 14 :
							glyph = extractor.ExtractGlyph('f'); break;
							case 35 :
							glyph = extractor.ExtractGlyph('g'); g = true; break;
							case 2 :
							case 27 :
							glyph = extractor.ExtractGlyph('h'); break;
							case 6 :
							glyph = extractor.ExtractGlyph('i'); break;
							case 17 :
							glyph = extractor.ExtractGlyph('j'); break;
							case 8 :
							glyph = extractor.ExtractGlyph('k'); break;
							case 29 :
							glyph = extractor.ExtractGlyph('l'); break;
							case 19 :
							glyph = extractor.ExtractGlyph('m'); break;
							case 13 :
							glyph = extractor.ExtractGlyph('n'); break;
							case 11 :
							case 15 :
							case 22 :
							case 34 :
							glyph = extractor.ExtractGlyph('o'); break;
							case 20 :
							glyph = extractor.ExtractGlyph('p'); break;
							case 4 :
							glyph = extractor.ExtractGlyph('q'); break;
							case 10 :
							case 25 :
							glyph = extractor.ExtractGlyph('r'); break;
							case 21 :
							glyph = extractor.ExtractGlyph('s'); break;
							case 1 :
							case 26 :
							glyph = extractor.ExtractGlyph('t'); break;
							case 5 :
							case 18 :
							glyph = extractor.ExtractGlyph('u'); break;
							case 23 :
							glyph = extractor.ExtractGlyph('v'); break;
							case 12 :
							glyph = extractor.ExtractGlyph('w'); break;
							case 16 :
							glyph = extractor.ExtractGlyph('x'); break;
							case 32 :
							glyph = extractor.ExtractGlyph('y'); break;
							case 31 :
							glyph = extractor.ExtractGlyph('z'); break;
						}

						for (uint i = 0; i < glyph.contours.size(); i++) {
							for (uint j = 0; j < glyph.contours[i].size(); j++) {
								//cout << j << " " << i << " " << glyph.contours[i][j].degree << " " << glyph.contours[i][j].x[0] << " " << glyph.contours[i][j].x[1] << " " << glyph.contours[i][j].y[0] << " " << glyph.contours[i][j].y[1] << endl;

								uint degree = glyph.contours[i][j].degree;
								switch (degree) {
									case 0:
									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									for (int i = 0; i < 1; i++){for (int j = 0; j < 1; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f;
									GeneratePoint(&geometry, &shader, vertices, colours);
									RenderControlPoints(&geometry, &shader);

									break;
									case 1:
									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									vertices[1][0] = (float)((glyph.contours[i][j].x[1]+advance)/scale)+move;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;

									for (int i = 0; i < 2; i++){for (int j = 0; j < 2; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f; colours[1][0] = 1.0f;

									Generateline(&geometry, &shader, vertices, colours);
									RenderControlLines(&geometry, &shader);
									break;
									case 2:
									glUseProgram(shader.program);
									glUniform2f(loc1, 0.0, 0.0);
									glPatchParameteri(GL_PATCH_VERTICES, 3);

									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									vertices[1][0] = (float)((glyph.contours[i][j].x[1]+advance)/scale)+move;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;
									vertices[2][0] = (float)((glyph.contours[i][j].x[2]+advance)/scale)+move;  vertices[2][1] = (float)(glyph.contours[i][j].y[2]+(translation*scale))/scale;

									for (int i = 0; i < 3; i++){for (int j = 0; j < 3; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f;

									GenerateQuadratic(&geometry, &shader, vertices, colours);
									RenderBezier(&geometry, &shader);

									if(g)
										if(vertices[0][0] < -3.0)
											end = true;
									break;
									case 3:
									glUseProgram(shader.program);
									glUniform2f(loc1, 1.0, 0.0);
									glPatchParameteri(GL_PATCH_VERTICES, 4);

									vertices[0][0] = (float)((glyph.contours[i][j].x[0]+advance)/scale)+move;  vertices[0][1] = (float)(glyph.contours[i][j].y[0]+(translation*scale))/scale;
									vertices[1][0] = (float)((glyph.contours[i][j].x[1]+advance)/scale)+move;  vertices[1][1] = (float)(glyph.contours[i][j].y[1]+(translation*scale))/scale;
									vertices[2][0] = (float)((glyph.contours[i][j].x[2]+advance)/scale)+move;  vertices[2][1] = (float)(glyph.contours[i][j].y[2]+(translation*scale))/scale;
									vertices[3][0] = (float)((glyph.contours[i][j].x[3]+advance)/scale)+move;  vertices[3][1] = (float)(glyph.contours[i][j].y[3]+(translation*scale))/scale;

									for (int i = 0; i < 4; i++){for (int j = 0; j < 4; j++){colours[i][j] = 0.0f;}}
									colours[0][0] = 1.0f; colours[1][0] = 1.0f; colours[2][0] = 1.0f; colours[3][0] = 1.0f;

									GenerateCubic(&geometry, &shader, vertices, colours);
									RenderBezier(&geometry, &shader);
									break;
								}
							}
						}
						advance = advance + glyph.advance;
					}
				}
			break;

		}



		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&geometry);
	DestroyShaders(&shader);
	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint TCSshader, GLuint TESshader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (TCSshader) glAttachShader(programObject, TCSshader);
	if (TESshader) glAttachShader(programObject, TESshader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}
