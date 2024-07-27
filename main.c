#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <float.h>
#include <math.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include "simp_GLerror.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "simp_quadtree.h"
#include "fluid.h"
#include "utils.h"

#define WIDTH 900
#define HEIGHT 900
#define PI 3.14159265359

static GLuint build_program(char* vertex_src, char* fragment_src);
static char* read_file(const char* file);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main(void)
{
	//Random number generator seeding
	//srand((unsigned int)time(NULL));

	//GLFW init code
	if(!glfwInit()){ exit(1); }
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Window creation
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ounga Bounga", NULL, NULL);
	glfwSetWindowPos(window, 800, 100);
	glfwSetKeyCallback(window, key_callback);
	if(!window){ glfwTerminate(); exit(1); }
	glfwMakeContextCurrent(window);
	glewInit();

	//OpenGL flags
	glEnable(GL_PROGRAM_POINT_SIZE);

	//Program creation
	GLuint program = build_program(read_file("vertex.glsl"), read_file("fragment.glsl"));

	//Uniform locations
	GLint window_info_loc, rad_loc, max_dens_loc, render_flag_loc;
	window_info_loc = glGetUniformLocation(program, "window_info");
	rad_loc = glGetUniformLocation(program, "rad");
	render_flag_loc = glGetUniformLocation(program, "render_flag");

	//Particle data initialization
	uint32_t grid_size = 30u;
	uint32_t particle_count = grid_size * grid_size;
	float radius = 0.004f;
	float damp_factor = 0.98f;
	float rest_density = 5000.0f;
	float stiffness_constant = 5.0f;
	float surface_coefficient = 50.0f;
	float viscosity_coefficient = 50.0f;
	float* particle_cpos = malloc(particle_count * 2u * sizeof *particle_cpos);
	float* particle_ppos = malloc(particle_count * 2u * sizeof *particle_ppos);
	float* particle_velo = malloc(particle_count * 2u * sizeof *particle_velo);
	float* particle_dens = malloc(particle_count * 1u * sizeof *particle_dens);
	float* particle_pred = malloc(particle_count * 2u * sizeof *particle_pred);
	float* particle_colo = malloc(particle_count * 3u * sizeof *particle_colo);
	for(int i = 0; i < particle_count; i++)
	{
		int i1 = i % grid_size;
		int i2 = (i - i1) / grid_size;
		particle_cpos[2 * i + 0] = particle_ppos[2 * i + 0] = 0.3f + 0.4f * ((float)i1 + 0.5f) / grid_size;
		particle_cpos[2 * i + 1] = particle_ppos[2 * i + 1] = 0.3f + 0.4f * ((float)i2 + 0.5f) / grid_size;
		//particle_cpos[2 * i + 0] = particle_ppos[2 * i + 0] = frand(0.1, 0.9);
        //particle_cpos[2 * i + 1] = particle_ppos[2 * i + 1] = frand(0.1, 0.9);

		particle_velo[2 * i + 0] = 0.0f;
		particle_velo[2 * i + 1] = 0.0f;

		particle_colo[3 * i + 0] = 1.0f;
		particle_colo[3 * i + 1] = 1.0f;
		particle_colo[3 * i + 2] = 1.0f;
	}

	//OpenGL buffer creation
	GLuint VAO, particle_pos_AB, particle_vel_AB, particle_col_AB;
	GL(glGenVertexArrays(1, &VAO));
	GL(glGenBuffers(1, &particle_pos_AB));
	GL(glGenBuffers(1, &particle_vel_AB));
	GL(glGenBuffers(1, &particle_col_AB));

	GL(glBindVertexArray(VAO));

	GL(glBindBuffer(GL_ARRAY_BUFFER, particle_pos_AB));
	GL(glBufferData(GL_ARRAY_BUFFER, particle_count * 2u * sizeof(float), NULL, GL_DYNAMIC_DRAW));
	GL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0));
	GL(glEnableVertexAttribArray(0));

	GL(glBindBuffer(GL_ARRAY_BUFFER, particle_vel_AB));
	GL(glBufferData(GL_ARRAY_BUFFER, particle_count * 2u * sizeof(float), NULL, GL_DYNAMIC_DRAW));
	GL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0));
	GL(glEnableVertexAttribArray(1));

	GL(glBindBuffer(GL_ARRAY_BUFFER, particle_col_AB));
	GL(glBufferData(GL_ARRAY_BUFFER, particle_count * 3u * sizeof(float), NULL, GL_DYNAMIC_DRAW));
	GL(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));
	GL(glEnableVertexAttribArray(2));

	GL(glBindVertexArray(0));

	//Time variables
	double t1, t2, dt = 1e-6, _dt = 1.0f / 220.0f;

	//Simulation settings
	float gravity = -1e1;
	int render_flag = 0;

	//Framerate approximation variables
	char fps_str[32] = { 0 };
	float time_accum = 0.0f;
	size_t frame_count = 1u;

	//Window variables
	int width, height;
	int key_state, key_hold_flag = 0;

	while(!glfwWindowShouldClose(window))
	{
		t1 = glfwGetTime();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		float ratio = (float)width / (float)height;
		double mouse_x, mouse_y;
		glfwGetCursorPos(window, &mouse_x, &mouse_y);
		double screen_x = (mouse_x / width);
		double screen_y = 1.0 - (mouse_y / height);

		dt = _dt;
		simp_quadtree* qtree = simp_quadtree_create(0.0f, 0.0f, 1.0f, 1.0f, 4u);
		for(int i = 0; i < particle_count; i++)
		{
			float fixed_step = 1.1666667f * dt;
			particle_pred[2 * i + 0] = particle_cpos[2 * i + 0] + particle_velo[2 * i + 0] * fixed_step;
			particle_pred[2 * i + 1] = particle_cpos[2 * i + 1] + particle_velo[2 * i + 1] * fixed_step;
			simp_quadtree_insert(qtree, particle_cpos[2 * i + 0], particle_cpos[2 * i + 1], i);
		}

		for(int i = 0; i < particle_count; i++)
			particle_dens[i] = sample_density(i, qtree, particle_pred, 5e-2);

		for(int i = 0; i < particle_count; i++)
		{
			//Fetch position data
			float px = particle_cpos[2 * i + 0];
			float py = particle_cpos[2 * i + 1];
			float vx = particle_velo[2 * i + 0];
			float vy = particle_velo[2 * i + 1];

			//Save previous location
			particle_ppos[2 * i + 0] = px;
			particle_ppos[2 * i + 1] = py;

			//Gravity
			vy += gravity * dt;

			//Fluid acceleration
			float fluid_ax, fluid_ay;
			fluid_accel(i, qtree, particle_pred, particle_velo, particle_dens, particle_colo, 5e-2,
					rest_density, stiffness_constant, surface_coefficient, 
					viscosity_coefficient, &fluid_ax, &fluid_ay);

			vx += fluid_ax * dt;
			vy += fluid_ay * dt;

			key_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
			if(key_state == GLFW_PRESS)
			{
				float dx = screen_x - px;
				float dy = screen_y - py;
				float dd = dot(dx, dy, dx, dy);
				if(dd < 4e-2)
				{
					vx += (dx * 5e2 - 1e1 * vx)* dt;
					vy += (dy * 5e2 - 1e1 * vy)* dt;
				}
			}

			key_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
			if(key_state == GLFW_PRESS)
			{
				float dx = screen_x - px;
				float dy = screen_y - py;
				float dd = dot(dx, dy, dx, dy);
				if(dd < 4e-2)
				{
					vx -= dx * 5e2 * dt;
					vy -= dy * 5e2 * dt;
				}
			}

			//Clamp velocity to 0 if too small
			vx = (fabs(vx) > 1e-6) * vx;
			vy = (fabs(vy) > 1e-6) * vy;

			px += vx * dt;
			py += vy * dt;

			//Boundary collision resolution
			if(px - radius < 0.0f || px + radius > 1.0f)
			{
				px = fclamp(px, radius, 1.0f - radius);
				vx -= 2.0f * damp_factor * vx;
			}
			if(py - radius < 0.0f || py + radius > 1.0f)
			{
				py = fclamp(py, radius, 1.0f - radius);
				vy -= 2.0f * damp_factor * vy;
			}

			particle_cpos[2 * i + 0] = px;
			particle_cpos[2 * i + 1] = py;
			particle_velo[2 * i + 0] = vx;
			particle_velo[2 * i + 1] = vy;
		}
		simp_quadtree_destroy(qtree);

		key_state = glfwGetKey(window, GLFW_KEY_TAB);
		if(key_state == GLFW_PRESS && !key_hold_flag)
		{
			render_flag = 1 - render_flag;
			key_hold_flag = 1;
		}
		if(key_state == GLFW_RELEASE)
		{
			key_hold_flag = 0;
		}


		//Particle rendering
		GL(glUseProgram(program));
		GL(glBindVertexArray(VAO));

		GL(glBindBuffer(GL_ARRAY_BUFFER, particle_pos_AB));
		GL(glBufferSubData(GL_ARRAY_BUFFER, 0, particle_count * 2u * sizeof(float), (void*)particle_cpos));

		GL(glBindBuffer(GL_ARRAY_BUFFER, particle_vel_AB));
		GL(glBufferSubData(GL_ARRAY_BUFFER, 0, particle_count * 2u * sizeof(float), (void*)particle_velo));

		GL(glBindBuffer(GL_ARRAY_BUFFER, particle_col_AB));
		GL(glBufferSubData(GL_ARRAY_BUFFER, 0, particle_count * 3u * sizeof(float), (void*)particle_colo));

		GL(glUniform2f(window_info_loc, width, height));
		GL(glUniform1f(rad_loc, radius));
		GL(glUniform1i(render_flag_loc, render_flag));

		GL(glDrawArrays(GL_POINTS, 0, particle_count));

		glfwPollEvents();
		glfwSwapBuffers(window);

		t2 = glfwGetTime();
		dt = t2 - t1;
		if(time_accum >= 0.1f)
		{
			sprintf(fps_str, "Willy Wonka |  %.0f  fps |", (float)frame_count / time_accum);
			glfwSetWindowTitle(window, fps_str);
			time_accum = 0.0f;
			frame_count = 1u;
		}
		time_accum += dt;
		frame_count++;
	}

	//Cleanup
	free(particle_cpos);
	free(particle_ppos);
	free(particle_velo);
	free(particle_dens);
	free(particle_pred);
	free(particle_colo);

	GL(glDeleteVertexArrays(1, &VAO));
	GL(glDeleteBuffers(1, &particle_pos_AB));
	GL(glDeleteBuffers(1, &particle_vel_AB));
	GL(glDeleteBuffers(1, &particle_col_AB));

	glfwTerminate();
	return 0x45;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	    glfwSetWindowShouldClose(window, true);
}

static GLuint build_program(char* vertex_src, char* fragment_src)
{
	GLuint program;
	if(!vertex_src || !fragment_src)
	{
		fprintf(stderr, "Pointer to vertex or fragment shader source was NULL\n");
		return -1;
	}

	GLuint vertex_shader, fragment_shader;
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GL(glShaderSource(vertex_shader, 1, (const GLchar**)&vertex_src, NULL));
	GL(glCompileShader(vertex_shader));

	GLint vertex_compiled;
	GL(glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_compiled));
	if (vertex_compiled != GL_TRUE)
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		GL(glGetShaderInfoLog(vertex_shader, 1024, &log_length, message));
		fprintf(stderr, "%s\n", message);
	}

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	GL(glShaderSource(fragment_shader, 1, (const GLchar**)&fragment_src, NULL));
	GL(glCompileShader(fragment_shader));

	GLint fragment_compiled;
	GL(glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_compiled));
	if (fragment_compiled != GL_TRUE)
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		GL(glGetShaderInfoLog(fragment_shader, 1024, &log_length, message));
		fprintf(stderr, "%s\n", message);
	}

	program = glCreateProgram();
	GL(glAttachShader(program, vertex_shader));
	GL(glAttachShader(program, fragment_shader));
	GL(glLinkProgram(program));

	GLint program_linked;
	GL(glGetProgramiv(program, GL_LINK_STATUS, &program_linked));
	if (program_linked != GL_TRUE)
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		GL(glGetProgramInfoLog(program, 1024, &log_length, message));
		fprintf(stderr, "%s\n", message);
	}
	GL(glDeleteShader(vertex_shader));
	GL(glDeleteShader(fragment_shader));
	free(vertex_src);
	free(fragment_src);
	return program;
}

static char* read_file(const char* file)
{
	FILE* f = fopen(file, "r");
	if(!f)
		return NULL;
	unsigned long file_size;
	fseek(f, 0, SEEK_END);
	file_size = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	char* res = calloc(file_size + 1, sizeof * res);
	if(!res)
	{
		fclose(f);
		return NULL;
	}
	
	char* curr = res;
	char c;
	while((c = fgetc(f)) != EOF)
	{
		*curr = c;
		curr++;
	}
	*curr = '\0';
	fclose(f);
	return res;
}

