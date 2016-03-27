/*
*   Project: Eduraster
*   Description: Software Rasterizer
*   Author: Eduardo Guerra
*/

#ifndef __EDURASTER__
#define __EDURASTER__

#include "mat_vec.h"

/* Matrix Modes */
#define ER_MODELVIEW 0x0
#define ER_PROJECTION 0x1

/* Error handling */
#define ER_NO_ERROR 0x0
#define ER_OUT_OF_RANGE 0x1
#define ER_INVALID_ENUM 0x2
#define ER_OUT_OF_MEMORY 0x3
#define ER_INVALID_OPERATION 0x4
#define ER_STACK_OVERFLOW 0x5
#define ER_STACK_UNDERFLOW 0x6
#define ER_NULL_POINTER 0x7
#define ER_NO_POWER_OF_TWO 0x8
#define ER_TEXTURE_MIPMAP_NO_COMPLETE 0x9
#define ER_NO_PROGRAM_LOADED 0xA

/* Boolean values */
#define ER_FALSE 0x0
#define ER_TRUE 0x1

/* Primitives */
#define ER_LINES 0x0
#define ER_LINE_STRIP 0x1
#define ER_LINE_LOOP 0x2
#define ER_TRIANGLES 0x3
#define ER_TRIANGLE_STRIP 0x4
#define ER_TRIANGLE_FAN 0x5
#define ER_POINTS 0x6

/* Polygon orientation */
#define ER_FRONT 0x7
#define ER_BACK 0x8
#define ER_FRONT_AND_BACK 0x9
#define ER_CULL_FACE 0xA
#define ER_COUNTER_CLOCK_WISE 0xB
#define ER_CLOCK_WISE 0xC

/* Polygon mode */
#define ER_FILL 0xD
#define ER_LINE 0xE
#define ER_POINT 0xF

/* Texture targets */
#define ER_TEXTURE_1D 0x10
#define ER_TEXTURE_2D 0x11
#define ER_TEXTURE_CUBE_MAP 0x12
#define ER_TEXTURE_CUBE_MAP_POSITIVE_X 0x13
#define ER_TEXTURE_CUBE_MAP_NEGATIVE_X 0x14
#define ER_TEXTURE_CUBE_MAP_POSITIVE_Y 0x15
#define ER_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x16
#define ER_TEXTURE_CUBE_MAP_POSITIVE_Z 0x17
#define ER_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x18

/* Texture filtering */
#define ER_NEAREST 0x19
#define ER_LINEAR 0x1A
#define ER_NEAREST_MIPMAP_NEAREST 0x1B
#define ER_NEAREST_MIPMAP_LINEAR 0x1C
#define ER_LINEAR_MIPMAP_NEAREST 0x1D
#define ER_LINEAR_MIPMAP_LINEAR 0x1E

/* Texture coordinates */
#define ER_TEXTURE_COORD_S 0x1F
#define ER_TEXTURE_COORD_T 0x20
#define ER_TEXTURE_COORD_R 0x21
#define ER_TEXTURE_COORD_Q 0x22

/* Wrapping modes */
#define ER_REPEAT 0x23
#define ER_MIRROR_REPEAT 0x24
#define ER_CLAMP_TO_EDGE 0x25

/* Texture parameters */
#define ER_MAGNIFICATION_FILTER 0x26
#define ER_MINIFICATION_FILTER 0x27
#define ER_LOD_BASE_LEVEL 0x28
#define ER_LOD_MAX_LEVEL 0x29
#define ER_LOD_MIN 0x2A
#define ER_LOD_MAX 0x2B
#define ER_WRAP_S 0x2C
#define ER_WRAP_T 0x2D
#define ER_WRAP_R 0x2E

/* Point sprites */
#define ER_POINT_SPRITES 0x2F
#define ER_POINT_SPRITE_COORD_ORIGIN 0x30
#define ER_POINT_SPRITE_LOWER_LEFT 0x31
#define ER_POINT_SPRITE_UPPER_LEFT 0x32

/* Vertex array */
#define ER_VERTEX_ARRAY 0x33
#define ER_NORMAL_ARRAY 0x34
#define ER_COLOR_ARRAY 0x35
#define ER_FOG_COORD_ARRAY 0x36
#define ER_TEX_COORD_ARRAY 0x37

/* Texture formats */
#define ER_R32F 0x38
#define ER_RG32F 0x39
#define ER_RGB32F 0x3A
#define ER_RGBA32F 0x3B
#define ER_DEPTH32F 0x3C

#define ATTRIBUTES_SIZE 16

struct vertex_input {
    vec4 position;
    vec3 normal;
    vec4 color;
    float fog_coord;
    vec4 tex_coord;
};

struct vertex_output{
    vec4 position;
    float attributes[ATTRIBUTES_SIZE];
    float point_size;
};

struct fragment_input{
    vec4 frag_coord;
    float attributes[ATTRIBUTES_SIZE];
    float ddx[ATTRIBUTES_SIZE];
    float ddy[ATTRIBUTES_SIZE];
    float dz_dx, dz_dy;
    float dw_dx, dw_dy;
    vec2 point_coord; 
    float point_size;
    unsigned int front_facing;
};

struct texture;

struct vertex_array;

struct program;

struct uniform_variables {
    float (*modelview)[4];
    float (*modelview_projection)[4];
    float (*modelview_inverse)[4];
    float (*normal)[3];
    float (*projection)[4];
    float (*projection_inverse)[4];
    unsigned int origin_y, origin_x;
    unsigned int width, height;
    int *uniform_integer;
    float *uniform_float;
    void **uniform_ptr;
    struct texture **uniform_texture;
};

/* Viewport settings */

void er_viewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

/* Matrix stack utilities */

void er_push_matrix();

void er_pop_matrix();

void er_load_identity();

void er_matrix_mode(unsigned char mode);

void er_load_matrix(mat4 matrix);

void er_multiply_matrix(mat4 matrix);

void er_get_matrix(mat4 matrix);

void er_load_matrix_transpose(mat4 matrix);

void er_orthographic(float l, float r, float b, float t, float n, float f);

void er_perspective( float angle, float aspect_ratio, float n, float f);

void er_frustum( float l, float r, float b, float t, float n, float f);

void er_translate(float x, float y, float z);

void er_scale(float x, float y, float z);

void er_scale_direction(float k, float x, float y, float z);

void er_rotate(float angle, float x, float y, float z);

void er_rotate_x(float angle);

void er_rotate_y(float angle);

void er_rotate_z(float angle);

void er_look_at(float eye_x, float eye_y, float eye_z, float at_x, float at_y, float at_z, float up_x, float up_y, float up_z);

/* Vertex Array routines */

struct vertex_array* er_create_vertex_array();

void er_delete_vertex_array(struct vertex_array *va);

void er_use_vertex_array(struct vertex_array *va);

void er_enable_attribute_array(struct vertex_array *va, int array_enum, int enable);

void er_vertex_pointer(struct vertex_array *va, unsigned int stride, unsigned int components, float *pointer);

void er_color_pointer(struct vertex_array *va, unsigned int stride, unsigned int components, float *pointer);

void er_normal_pointer(struct vertex_array *va, unsigned int stride, float *pointer);

void er_fog_coord_pointer(struct vertex_array *va, unsigned int stride, float *pointer);

void er_tex_coord_pointer(struct vertex_array *va, unsigned int stride, unsigned int components, float *pointer);

/* Drawing routines: Begin/End style */

void er_begin(int primitive);

void er_end();

void er_normal3f(float nx, float ny, float nz);

void er_normal3fv(float *normal);

void er_color3f(float r, float g, float b);

void er_color3fv(float *color);

void er_color4f(float r, float g, float b, float a);

void er_color4fv(float *color);

void er_vertex2f(float x, float y);

void er_vertex2fv(float *point);

void er_vertex3f(float x, float y, float z);

void er_vertex3fv(float *point);

void er_vertex4f(float x, float y, float z, float w);

void er_vertex4fv(float *point);

void er_tex_coord1f(float s);

void er_tex_coord1fv(float *tex_coord);

void er_tex_coord2f(float s, float t);

void er_tex_coord2fv(float *tex_coord);

void er_tex_coord3f(float s, float t, float r);

void er_tex_coord3fv(float *tex_coord);

void er_tex_coord4f(float s, float t, float r, float q);

void er_tex_coord4fv(float *tex_coord);

void er_fog_coordf(float fog_coord);

void er_fog_coordfv(float *fog_coord);

/* Drawing routines (indexed and non indexed) */

void er_draw_elements(unsigned int primitive, unsigned int size, unsigned int *index);

void er_draw_arrays(unsigned int primitive, unsigned int first, unsigned int count);

/* Init routines */

int er_init();

void er_quit();

/* Error handling */

int er_get_error();

const char* er_get_error_string(int error);

/* State settings */

void er_enable(unsigned int param, unsigned int enable);

void er_cull_face(unsigned int face);

void er_front_face(unsigned int orientation);

void er_polygon_mode(int face, int mode);

void er_point_parameteri(int param, int value);

/* Texture mapping setup */

struct texture* er_create_texture1D(int width, int internal_format);

struct texture* er_create_texture2D(int width, int height, int internal_format);

struct texture* er_create_texture_cubemap(int dimension, int internal_format);

void er_delete_texture(struct texture *tex);

void* er_texture_ptr(struct texture *tex, int texture_target, int level);

void er_texture_filtering(struct texture *tex, int parameter, int value);

void er_texture_wrap_mode(struct texture *tex, int parameter, int value);

void er_generate_mipmaps(struct texture *tex);

/* Texture functions */

void texture_size(struct texture *tex, int lod, int *dimension);

void texel_fetch(struct texture *tex, int *coord, int lod, float *color);

void texture_lod(struct texture *tex, float *coord, float lod, float *color);

void texture_grad(struct texture *tex, float *coord, float *ddx, float *ddy, float *color);

void write_texture(struct texture *tex, int *coord, int lod, float *color);

/* Program settings */

struct program* er_create_program();

void er_delete_program(struct program *p);

void er_use_program(struct program *p);

void er_uniformi(struct program *p, int index, int value);

void er_uniformf(struct program *p, int index, float value);

void er_uniform_ptr(struct program *p, int index, void *pointer);

void er_uniform_texture_ptr(struct program *p, int index, struct texture *tex_ptr);

void er_varying_attributes(struct program *p, int number);

void er_load_fragment_shader(struct program *p, void (*fragment_shader)(int, int, struct fragment_input*, struct uniform_variables*));

void er_load_vertex_shader(struct program *p, void (*vertex_shader)(struct vertex_input*, struct vertex_output*, struct uniform_variables*) );

void er_load_homogeneous_division(struct program *p, void (*homogeneous_division)(struct vertex_output*) );

#endif
