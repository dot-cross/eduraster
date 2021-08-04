#ifndef __PIPELINE__
#define __PIPELINE__

#include <stdio.h>
#include <stdlib.h>
#include "eduraster.h"

#define TRIANGLES_BATCH_SIZE 32
#define LINES_BATCH_SIZE 48
#define POINTS_BATCH_SIZE 96

typedef struct OutputBufferRegister{
    struct er_VertexOutput vertex;
    unsigned int outcode;
    er_Bool processed;
} OutputBufferRegister;

#include "mat_stack.h"
#include "vertex_array.h"
#include "clipping.h"
#include "texture_mapping.h"
#include "rasterization.h"
#include "program.h"

extern unsigned int window_origin_x;
extern unsigned int window_origin_y;
extern unsigned int window_width;
extern unsigned int window_height;

extern er_PrimitiveEnum current_primitive;
extern struct er_VertexInput *input_buffer;
extern unsigned int input_buffer_size;
extern unsigned int *input_indices;
extern unsigned int input_indices_size;
extern struct OutputBufferRegister *output_buffer;
extern unsigned int output_buffer_size;
extern unsigned int *output_indices;
extern unsigned int output_indices_size;
extern struct er_UniVars global_variables;
extern vec3 current_normal;
extern vec4 current_tex_coord;
extern vec4 current_color;
extern float current_fog_coord;

void process_points();

void process_lines();

void process_triangles();

void reset_buffers_size();

void update_uniform_vars();

#endif
