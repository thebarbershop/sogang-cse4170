#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include <cmath>
#include <iostream>
#include <vector>

#include "Shaders/LoadShaders.h"
GLuint h_ShaderProgram;									  // handle to shader program
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables

// include glm/*.hpp only if necessary
//#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, ortho, etc.
glm::mat4 ModelViewProjectionMatrix;
glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix;

#define TO_RADIAN 0.01745329252f
#define TO_DEGREE 57.295779513f
#define BUFFER_OFFSET(offset) ((GLvoid *)(offset))

#define LOC_VERTEX 0

//// Function headers ////
void prepare_scene(void);
void cleanup(void);
GLfloat current_angle(void);
//////////////////////////

//// CUSTUM CONSTANTS ////

const int background_color[] = {173, 255, 47}; // R,G,B value of background color (0-255)
const GLfloat PI = 3.14159265358979323846;
const unsigned int MIN_SPEED = 10;
const unsigned int MAX_SPEED = 20;
const unsigned int INITIAL_WIDTH = 1280;
const unsigned int INITIAL_HEIGHT = 800;

//////////////////////////

//// CUSTUM VARIABLES ////
unsigned int gameplay_speed = 15; // speed of gameplay
int health = 5;					  // gameover when health == 0
//////////////////////////

//// CUSTUM DATA STRUCTURE ////

struct point
{
	GLfloat x, y;
	point(GLfloat x, GLfloat y) : x(x), y(y) {}
	point() { point(0.0f, 0.0f); }
	point move(GLfloat angle, GLfloat displacement) const
	{
		// move the point by given angle and displacement
		GLfloat x1 = x + displacement * std::cos(angle);
		GLfloat y1 = y + displacement * std::sin(angle);
		return point(x1, y1);
	}

	point operator*(const point &rhs) const
	{
		GLfloat x1 = x * rhs.x;
		GLfloat y1 = y * rhs.y;
		return point(x1, y1);
	}
};

///////////////////////////////

int win_width = 0,
	win_height = 0;

//// DESIGN ROAD ////

const unsigned int ROAD_MAIN = 0;
const unsigned int ROAD_LINE_LEFT = 1;
const unsigned int ROAD_LINE_RIGHT = 2;

GLfloat road_main_shape[6][2] = {
	{-0.5f, 0.0f},
	{(1.0f / 3), 0.5f},
	{0.5f, 0.5f},
	{0.5f, 0.0f},
	{-(1.0f / 3), -0.5f},
	{-0.5f, -0.5f},
};
GLfloat road_line_left_shape[4][2] = {
	{-0.5f, 0.0f},
	{1.0f / 3, 0.5f},
	{23.0f / 60, 0.5f},
	{-0.5f, -1.0f / 20},
};
GLfloat road_line_right_shape[4][2] = {
	{0.5f, 0.0f},
	{-1.0f / 3, -0.5f},
	{-23.0f / 60, -0.5f},
	{0.5f, 1.0f / 20},
};

GLfloat road_color[3][3] = {
	{0x70 / 255.0f, 0x80 / 255.0f, 0x90 / 255.0f},
	{0xFF / 255.0f, 0xFA / 255.0f, 0xFA / 255.0f},
	{0xFF / 255.0f, 0xFA / 255.0f, 0xFA / 255.0f},
};

GLuint VBO_road, VAO_road;

void prepare_road()
{
	GLsizeiptr buffer_size = sizeof(road_main_shape) + sizeof(road_line_left_shape) + sizeof(road_line_right_shape);

	glGenBuffers(1, &VBO_road);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_road);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(road_main_shape), road_main_shape);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(road_main_shape), sizeof(road_line_left_shape), road_line_left_shape);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(road_main_shape) + sizeof(road_line_left_shape), sizeof(road_line_right_shape), road_line_right_shape);

	glGenVertexArrays(1, &VAO_road);
	glBindVertexArray(VAO_road);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_road);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_road()
{
	glBindVertexArray(VAO_road);

	glUniform3fv(loc_primitive_color, 1, road_color[ROAD_MAIN]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

	glUniform3fv(loc_primitive_color, 1, road_color[ROAD_LINE_LEFT]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 4);

	glUniform3fv(loc_primitive_color, 1, road_color[ROAD_LINE_RIGHT]);
	glDrawArrays(GL_TRIANGLE_FAN, 10, 4);

	glBindVertexArray(0);
}
//// DESIGN ROAD END ////

//// DESIGN HOUSE ////
#define HOUSE_ROOF 0
#define HOUSE_BODY 1
#define HOUSE_CHIMNEY 2
#define HOUSE_DOOR 3
#define HOUSE_WINDOW 4

GLfloat roof[3][2] = {{-12.0, 0.0}, {0.0, 12.0}, {12.0, 0.0}};
GLfloat house_body[4][2] = {{-12.0, -14.0}, {-12.0, 0.0}, {12.0, 0.0}, {12.0, -14.0}};
GLfloat chimney[4][2] = {{6.0, 6.0}, {6.0, 14.0}, {10.0, 14.0}, {10.0, 2.0}};
GLfloat door[4][2] = {{-8.0, -14.0}, {-8.0, -8.0}, {-4.0, -8.0}, {-4.0, -14.0}};
GLfloat window[4][2] = {{4.0, -6.0}, {4.0, -2.0}, {8.0, -2.0}, {8.0, -6.0}};

GLfloat house_color[5][3] = {
	{200 / 255.0f, 39 / 255.0f, 42 / 255.0f},
	{235 / 255.0f, 225 / 255.0f, 196 / 255.0f},
	{255 / 255.0f, 0 / 255.0f, 0 / 255.0f},
	{233 / 255.0f, 113 / 255.0f, 23 / 255.0f},
	{44 / 255.0f, 180 / 255.0f, 49 / 255.0f},
};

const size_t n_house = 2;
std::vector<point> house_positions;
const std::vector<point> house_initial_positions = {
	point(1.0f / 3, 0.65f),
	point(0.5f, -0.1f),
}; // initial positions, propotional to win_width and win_height
const std::vector<GLfloat> house_initial_displacement = {0, INITIAL_HEIGHT *(-0.5)};

std::vector<point> house_sizes;
const std::vector<point> house_initial_sizes = {point(3.0f, 3.0f), point(3.0f, 3.0f)};

GLuint VBO_house, VAO_house;
void prepare_house()
{
	GLsizeiptr buffer_size = sizeof(roof) + sizeof(house_body) + sizeof(chimney) + sizeof(door) + sizeof(window);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_house);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_house);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(roof), roof);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(roof), sizeof(house_body), house_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(roof) + sizeof(house_body), sizeof(chimney), chimney);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(roof) + sizeof(house_body) + sizeof(chimney), sizeof(door), door);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(roof) + sizeof(house_body) + sizeof(chimney) + sizeof(door),
					sizeof(window), window);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_house);
	glBindVertexArray(VAO_house);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_house);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Initialize house position
	for (size_t i = 0; i < n_house; ++i)
	{
		house_positions.push_back(house_initial_positions[i] * point(win_width, win_height));
		house_sizes.push_back(house_initial_sizes[i]);
	}
}

void draw_house()
{
	glBindVertexArray(VAO_house);

	glUniform3fv(loc_primitive_color, 1, house_color[HOUSE_ROOF]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);

	glUniform3fv(loc_primitive_color, 1, house_color[HOUSE_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 3, 4);

	glUniform3fv(loc_primitive_color, 1, house_color[HOUSE_CHIMNEY]);
	glDrawArrays(GL_TRIANGLE_FAN, 7, 4);

	glUniform3fv(loc_primitive_color, 1, house_color[HOUSE_DOOR]);
	glDrawArrays(GL_TRIANGLE_FAN, 11, 4);

	glUniform3fv(loc_primitive_color, 1, house_color[HOUSE_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 4);

	glBindVertexArray(0);
}
//// DESIGN HOUSE END ////

//// DESIGN CAR ////
//draw car2
#define CAR2_BODY 0
#define CAR2_FRONT_WINDOW 1
#define CAR2_BACK_WINDOW 2
#define CAR2_FRONT_WHEEL 3
#define CAR2_BACK_WHEEL 4
#define CAR2_LIGHT1 5
#define CAR2_LIGHT2 6

GLfloat car2_body[8][2] = {{-18.0, -7.0}, {-18.0, 0.0}, {-13.0, 0.0}, {-10.0, 8.0}, {10.0, 8.0}, {13.0, 0.0}, {18.0, 0.0}, {18.0, -7.0}};
GLfloat car2_front_window[4][2] = {{-10.0, 0.0}, {-8.0, 6.0}, {-2.0, 6.0}, {-2.0, 0.0}};
GLfloat car2_back_window[4][2] = {{0.0, 0.0}, {0.0, 6.0}, {8.0, 6.0}, {10.0, 0.0}};
GLfloat car2_front_wheel[8][2] = {{-11.0, -11.0}, {-13.0, -8.0}, {-13.0, -7.0}, {-11.0, -4.0}, {-7.0, -4.0}, {-5.0, -7.0}, {-5.0, -8.0}, {-7.0, -11.0}};
GLfloat car2_back_wheel[8][2] = {{7.0, -11.0}, {5.0, -8.0}, {5.0, -7.0}, {7.0, -4.0}, {11.0, -4.0}, {13.0, -7.0}, {13.0, -8.0}, {11.0, -11.0}};
GLfloat car2_light1[3][2] = {{-18.0, -1.0}, {-17.0, -2.0}, {-18.0, -3.0}};
GLfloat car2_light2[3][2] = {{-18.0, -4.0}, {-17.0, -5.0}, {-18.0, -6.0}};

GLfloat car2_color[7][3] = {
	{100 / 255.0f, 141 / 255.0f, 159 / 255.0f},
	{235 / 255.0f, 219 / 255.0f, 208 / 255.0f},
	{235 / 255.0f, 219 / 255.0f, 208 / 255.0f},
	{0 / 255.0f, 0 / 255.0f, 0 / 255.0f},
	{0 / 255.0f, 0 / 255.0f, 0 / 255.0f},
	{249 / 255.0f, 244 / 255.0f, 0 / 255.0f},
	{249 / 255.0f, 244 / 255.0f, 0 / 255.0f}};

point car2_position;
GLfloat car2_displacement = 0.1f;
point car2_size = point(-4.0f, 4.0f);
int car2_updown = 0;

GLuint VBO_car2, VAO_car2;
void prepare_car2()
{
	GLsizeiptr buffer_size = sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel) + sizeof(car2_back_wheel) + sizeof(car2_light1) + sizeof(car2_light2);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_car2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car2);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(car2_body), car2_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body), sizeof(car2_front_window), car2_front_window);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window), sizeof(car2_back_window), car2_back_window);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window), sizeof(car2_front_wheel), car2_front_wheel);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel),
					sizeof(car2_back_wheel), car2_back_wheel);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel) + sizeof(car2_back_wheel), sizeof(car2_light1), car2_light1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel) + sizeof(car2_back_wheel) + sizeof(car2_light1), sizeof(car2_light2), car2_light2);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_car2);
	glBindVertexArray(VAO_car2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car2);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Initialize car object position
	car2_position = car2_position.move(-current_angle(), car2_displacement * win_width);
}

void draw_car2()
{
	glBindVertexArray(VAO_car2);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 8);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_FRONT_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_BACK_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_FRONT_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 8);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_BACK_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 24, 8);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_LIGHT1]);
	glDrawArrays(GL_TRIANGLE_FAN, 32, 3);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_LIGHT2]);
	glDrawArrays(GL_TRIANGLE_FAN, 35, 3);

	glBindVertexArray(0);
}
//// DESIGN CAR END ////

void display(void)
{
	glm::mat4 ModelMatrix;

	glClear(GL_COLOR_BUFFER_BIT);

	// draw road object
	ModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(win_width, win_height, 1.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_road();

	// draw house objects
	for (size_t i = 0; i < n_house; i++)
	{
		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(house_positions[i].x, house_positions[i].y, 0.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(house_sizes[i].x, house_sizes[i].y, 1.0f));
		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_house();
	}

	// draw car object
	ModelMatrix = glm::mat4(1.0f);
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(car2_size.x, car2_size.y, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(car2_position.x, car2_position.y, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, -current_angle(), glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0, car2_updown, 0.0f));

	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_car2();

	glFlush();
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:				 // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups.
		break;
	}
}

void special(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT:
		--gameplay_speed;
		if (gameplay_speed < MIN_SPEED)
			gameplay_speed = MIN_SPEED;
		glutPostRedisplay();
		break;
	case GLUT_KEY_RIGHT:
		++gameplay_speed;
		if (gameplay_speed > MAX_SPEED)
			gameplay_speed = MAX_SPEED;
		glutPostRedisplay();
		break;
	case GLUT_KEY_DOWN:
		car2_updown -= 2;
		glutPostRedisplay();
		break;
	case GLUT_KEY_UP:
		car2_updown += 2;
		glutPostRedisplay();
		break;
	}
}

void timer(int value)
{
	GLfloat angle = current_angle();

	// Move house
	for (size_t i = 0; i < house_positions.size(); ++i)
	{
		auto &house_position = house_positions[i];
		house_sizes[i] = house_sizes[i] * point(1.0005f, 1.0005f);
		house_position = house_position.move(angle, -1);
		if (house_position.x < -win_width * 0.6f || house_position.y < -win_height * 0.6f)
		{
			house_position = house_initial_positions[i] * point(win_width, win_height);
			house_sizes[i] = house_initial_sizes[i];
		}
	}
	glutPostRedisplay();
	glutTimerFunc(100.0f / gameplay_speed, timer, 0);
}

void cleanup(void)
{
	glDeleteVertexArrays(1, &VAO_road);
	glDeleteBuffers(1, &VBO_road);

	glDeleteVertexArrays(1, &VAO_house);
	glDeleteBuffers(1, &VBO_house);
}

void register_callbacks(void)
{
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutTimerFunc(0, timer, 0);
	glutCloseFunc(cleanup);
}

void prepare_shader_program(void)
{
	ShaderInfo shader_info[3] = {
		{GL_VERTEX_SHADER, "Shaders/simple.vert"},
		{GL_FRAGMENT_SHADER, "Shaders/simple.frag"},
		{GL_NONE, NULL}};

	h_ShaderProgram = LoadShaders(shader_info);
	glUseProgram(h_ShaderProgram);

	loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram, "u_ModelViewProjectionMatrix");
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram, "u_primitive_color");
}

void initialize_OpenGL(void)
{
	glEnable(GL_MULTISAMPLE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	int r = background_color[0],
		g = background_color[1],
		b = background_color[2];
	glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
	ViewMatrix = glm::mat4(1.0f);

	glViewport(0, 0, win_width, win_height);
	ProjectionMatrix = glm::ortho(-win_width / 2.0, win_width / 2.0,
								  -win_height / 2.0, win_height / 2.0, -1000.0, 1000.0);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
}

void prepare_scene(void)
{
	prepare_road();
	prepare_house();
	prepare_car2();
}

void initialize_renderer(void)
{
	register_callbacks();
	prepare_shader_program();
	initialize_OpenGL();
	prepare_scene();
}

void initialize_glew(void)
{
	GLenum error;

	glewExperimental = GL_TRUE;

	error = glewInit();
	if (error != GLEW_OK)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

void greetings(char *program_name, char messages[][256], int n_message_lines)
{
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  %s\n\n", program_name);
	fprintf(stdout, " CSE4170 Assignment 1\n");
	fprintf(stdout, "    20120085 EOM, Taegyung\n");
	fprintf(stdout, "      April 17, 2019\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

GLfloat current_angle(void)
{
	return std::atan((win_height * 0.5f) / (win_width * (5.0f / 6)));
}

#define N_MESSAGE_LINES 2
int main(int argc, char *argv[])
{
	char program_name[64] = "Fury Road";
	char messages[N_MESSAGE_LINES][256] = {
		"    - Keys used: 'ESC', four arrows",
		"    - Mouse used: L-click and move"};

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_MULTISAMPLE);
	win_width = INITIAL_WIDTH;
	win_height = INITIAL_HEIGHT;
	glutInitWindowSize(win_width, win_height);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}
