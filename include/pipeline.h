#ifndef __PIPELINE__
#define __PIPELINE__

#include <stdio.h>
#include <stdlib.h>
#include "eduraster.h"

#define NO_PRIMITIVE -1

#define TRIANGLES_BATCH_SIZE 32
#define LINES_BATCH_SIZE 48
#define POINTS_BATCH_SIZE 96

struct output_buffer_register{
    struct vertex_output vertex;
    unsigned int outcode;
    int processed;
};

#include "mat_stack.h"
#include "vertex_array.h"
#include "clipping.h"
#include "texture_mapping.h"
#include "rasterization.h"
#include "program.h"

extern unsigned int WINDOW_ORIGIN_X;
extern unsigned int WINDOW_ORIGIN_Y;
extern unsigned int WINDOW_WIDTH;
extern unsigned int WINDOW_HEIGHT;

extern unsigned int current_primitive;
extern struct vertex_input *input_buffer;
extern unsigned int input_buffer_size;
extern unsigned int *input_indices;
extern unsigned int input_indices_size;
extern struct output_buffer_register *output_buffer;
extern unsigned int output_buffer_size;
extern unsigned int *output_indices;
extern unsigned int output_indices_size;
extern struct uniform_variables global_variables;
extern vec3 current_normal;
extern vec4 current_tex_coord;
extern vec4 current_color;
extern float current_fog_coord;

void process_points();

void process_lines();

void process_triangles();

void reset_buffers_size();

void update_uniform_vars();

void set_error(int error);

#endif
