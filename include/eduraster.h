/*
*   Project: Eduraster
*   Description: Software Rasterizer
*   Author: Eduardo Guerra
*/

#ifndef __EDURASTER__
#define __EDURASTER__

#include "mat_vec.h"

/* Boolean values */
typedef enum {
    ER_FALSE = 0,
    ER_TRUE = 1
} er_Bool;

/* Matrix Modes */
typedef enum {
    ER_MODELVIEW = 0x0,
    ER_PROJECTION = 0x1
} er_MatrixModeEnum;

/* Error handling */
typedef enum {
    ER_NO_ERROR = 0x0,
    ER_INVALID_ARGUMENT = 0x1,
    ER_NO_VERTEX_ARRAY_SET = 0x2,
    ER_OUT_OF_MEMORY = 0x3,
    ER_INVALID_OPERATION = 0x4,
    ER_STACK_OVERFLOW = 0x5,
    ER_STACK_UNDERFLOW = 0x6,
    ER_NULL_POINTER = 0x7,
    ER_NO_POWER_OF_TWO = 0x8,
    ER_NO_PROGRAM_SET = 0x9
} er_StatusEnum;

/* Primitives */
typedef enum {
    ER_LINES = 0x0,
    ER_LINE_STRIP = 0x1,
    ER_LINE_LOOP = 0x2,
    ER_TRIANGLES = 0x3,
    ER_TRIANGLE_STRIP = 0x4,
    ER_TRIANGLE_FAN = 0x5,
    ER_POINTS = 0x6
} er_PrimitiveEnum;

/* Polygon Face */
typedef enum {
    ER_FRONT = 0x7,
    ER_BACK = 0x8,
    ER_FRONT_AND_BACK = 0x9
} er_PolygonFaceEnum;

/* Polygon Orientation */
typedef enum {
    ER_COUNTER_CLOCK_WISE = 0xA,
    ER_CLOCK_WISE = 0xB
} er_PolygonOrientationEnum;

/* Polygon mode */
typedef enum {
    ER_FILL = 0xC,
    ER_LINE = 0xD,
    ER_POINT = 0xE
} er_PolygonModeEnum;

/* Texture targets */
typedef enum {
    ER_TEXTURE_1D = 0xF,
    ER_TEXTURE_2D = 0x10,
    ER_TEXTURE_CUBE_MAP = 0x11,
    ER_TEXTURE_CUBE_MAP_POSITIVE_X = 0x12,
    ER_TEXTURE_CUBE_MAP_NEGATIVE_X = 0x13,
    ER_TEXTURE_CUBE_MAP_POSITIVE_Y = 0x14,
    ER_TEXTURE_CUBE_MAP_NEGATIVE_Y = 0x15,
    ER_TEXTURE_CUBE_MAP_POSITIVE_Z = 0x16,
    ER_TEXTURE_CUBE_MAP_NEGATIVE_Z = 0x17
} er_TextureTargetEnum;

/* Texture filtering */
typedef enum {
    ER_NEAREST = 0x18,
    ER_LINEAR = 0x19,
    ER_NEAREST_MIPMAP_NEAREST = 0x1A,
    ER_NEAREST_MIPMAP_LINEAR = 0x1B,
    ER_LINEAR_MIPMAP_NEAREST = 0x1C,
    ER_LINEAR_MIPMAP_LINEAR = 0x1D
} er_TextureFilterEnum;

/* Texture coordinates */
typedef enum {
    ER_TEXTURE_COORD_S = 0x1E,
    ER_TEXTURE_COORD_T = 0x1F,
    ER_TEXTURE_COORD_R = 0x20,
    ER_TEXTURE_COORD_Q = 0x21
} er_TextureCoordEnum;

/* Wrapping modes */
typedef enum {
    ER_REPEAT = 0x22,
    ER_MIRROR_REPEAT = 0x23,
    ER_CLAMP_TO_EDGE = 0x24
} er_TextureWrapModeEnum;

/* Texture parameters */
typedef enum {
    ER_MAGNIFICATION_FILTER = 0x25,
    ER_MINIFICATION_FILTER = 0x26,
    ER_LOD_BASE_LEVEL = 0x27,
    ER_LOD_MAX_LEVEL = 0x28,
    ER_LOD_MIN = 0x29,
    ER_LOD_MAX = 0x2A,
    ER_WRAP_S = 0x2B,
    ER_WRAP_T = 0x2C,
    ER_WRAP_R = 0x2D
} er_TextureParamEnum;

/* Point sprites */
typedef enum {
    ER_POINT_SPRITE_COORD_ORIGIN = 0x2E,
    ER_POINT_SPRITE_LOWER_LEFT = 0x2F,
    ER_POINT_SPRITE_UPPER_LEFT = 0x30
} er_PointSpriteEnum;

/* Vertex array */
typedef enum {
    ER_VERTEX_ARRAY = 0x31,
    ER_NORMAL_ARRAY = 0x32,
    ER_COLOR_ARRAY = 0x33,
    ER_FOG_COORD_ARRAY = 0x34,
    ER_TEX_COORD_ARRAY = 0x35
} er_VertexArrayEnum;

/* Texture formats */
typedef enum {
    ER_R32F = 0x36,
    ER_RG32F = 0x37,
    ER_RGB32F = 0x38,
    ER_RGBA32F = 0x39,
    ER_DEPTH32F =0x3A
} er_TextureFormatEnum;

/* Settings */
typedef enum {
    ER_CULL_FACE = 0x3B,
    ER_POINT_SPRITES = 0x3C
} er_EnableSettingEnum;

#define ATTRIBUTES_SIZE 16

typedef struct er_VertexInput {
    vec4 position;
    vec3 normal;
    vec4 color;
    float fog_coord;
    vec4 tex_coord;
} er_VertexInput;

typedef struct er_VertexOutput{
    vec4 position;
    float attributes[ATTRIBUTES_SIZE];
    float point_size;
} er_VertexOutput;

typedef struct er_FragInput{
    vec4 frag_coord;
    float attributes[ATTRIBUTES_SIZE];
    float ddx[ATTRIBUTES_SIZE];
    float ddy[ATTRIBUTES_SIZE];
    float dz_dx, dz_dy;
    float dw_dx, dw_dy;
    vec2 point_coord; 
    float point_size;
    er_Bool front_facing;
} er_FragInput;

typedef struct er_Texture er_Texture;

typedef struct er_VertexArray er_VertexArray;

typedef struct er_Program er_Program;

typedef struct er_UniVars {
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
    struct er_Texture **uniform_texture;
} er_UniVars;

/* Viewport settings */

er_StatusEnum er_viewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

/* Matrix stack utilities */

er_StatusEnum er_push_matrix();

er_StatusEnum er_pop_matrix();

void er_load_identity();

er_StatusEnum er_matrix_mode(er_MatrixModeEnum mode);

er_StatusEnum er_load_matrix(mat4 matrix);

er_StatusEnum er_multiply_matrix(mat4 matrix);

er_StatusEnum er_get_matrix(mat4 matrix);

er_StatusEnum er_load_matrix_transpose(mat4 matrix);

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

er_VertexArray* er_create_vertex_array();

er_StatusEnum er_delete_vertex_array(er_VertexArray *va);

er_StatusEnum er_use_vertex_array(er_VertexArray *va);

er_StatusEnum er_enable_attribute_array(er_VertexArray *va, er_VertexArrayEnum array_enum, er_Bool enable);

er_StatusEnum er_vertex_pointer(er_VertexArray *va, unsigned int stride, unsigned int components, float *pointer);

er_StatusEnum er_color_pointer(er_VertexArray *va, unsigned int stride, unsigned int components, float *pointer);

er_StatusEnum er_normal_pointer(er_VertexArray *va, unsigned int stride, float *pointer);

er_StatusEnum er_fog_coord_pointer(er_VertexArray *va, unsigned int stride, float *pointer);

er_StatusEnum er_tex_coord_pointer(er_VertexArray *va, unsigned int stride, unsigned int components, float *pointer);

/* Drawing routines: Begin/End style */

er_StatusEnum er_begin(er_PrimitiveEnum primitive);

void er_end();

void er_normal3f(float nx, float ny, float nz);

er_StatusEnum er_normal3fv(float *normal);

void er_color3f(float r, float g, float b);

er_StatusEnum er_color3fv(float *color);

void er_color4f(float r, float g, float b, float a);

er_StatusEnum er_color4fv(float *color);

void er_vertex2f(float x, float y);

er_StatusEnum er_vertex2fv(float *point);

void er_vertex3f(float x, float y, float z);

er_StatusEnum er_vertex3fv(float *point);

void er_vertex4f(float x, float y, float z, float w);

er_StatusEnum er_vertex4fv(float *point);

void er_tex_coord1f(float s);

er_StatusEnum er_tex_coord1fv(float *tex_coord);

void er_tex_coord2f(float s, float t);

er_StatusEnum er_tex_coord2fv(float *tex_coord);

void er_tex_coord3f(float s, float t, float r);

er_StatusEnum er_tex_coord3fv(float *tex_coord);

void er_tex_coord4f(float s, float t, float r, float q);

er_StatusEnum er_tex_coord4fv(float *tex_coord);

void er_fog_coordf(float fog_coord);

er_StatusEnum er_fog_coordfv(float *fog_coord);

/* Drawing routines (indexed and non indexed) */

er_StatusEnum er_draw_elements(er_PrimitiveEnum primitive, unsigned int size, unsigned int *index);

er_StatusEnum er_draw_arrays(er_PrimitiveEnum primitive, unsigned int first, unsigned int count);

/* Init routines */

er_StatusEnum er_init();

void er_quit();

/* Status Strings */

const char* er_status_string(er_StatusEnum status);

/* State settings */

er_StatusEnum er_enable(er_EnableSettingEnum enumValue, er_Bool enable);

er_StatusEnum er_cull_face(er_PolygonFaceEnum face);

er_StatusEnum er_front_face(er_PolygonOrientationEnum orientation);

er_StatusEnum er_polygon_mode(er_PolygonFaceEnum face, er_PolygonModeEnum mode);

er_StatusEnum er_point_parameteri(er_PointSpriteEnum param, er_PointSpriteEnum value);

/* Texture mapping setup */

er_StatusEnum er_create_texture1D(er_Texture** tex, int width, er_TextureFormatEnum internal_format);

er_StatusEnum er_create_texture2D(er_Texture** tex, int width, int height, er_TextureFormatEnum internal_format);

er_StatusEnum er_create_texture_cubemap(er_Texture **tex, int size, er_TextureFormatEnum internal_format);

er_StatusEnum er_delete_texture(er_Texture *tex);

er_StatusEnum er_texture_ptr(er_Texture *tex, er_TextureTargetEnum texture_target, int level, float **data);

er_StatusEnum er_texture_filtering(er_Texture *tex, er_TextureParamEnum parameter, er_TextureFilterEnum value);

er_StatusEnum er_texture_wrap_mode(er_Texture *tex, er_TextureParamEnum, er_TextureWrapModeEnum value);

er_StatusEnum er_generate_mipmaps(er_Texture *tex);

/* Texture functions */

void er_texture_size(er_Texture *tex, int lod, int *size);

void er_texel_fetch(er_Texture *tex, int *coord, int lod, float *color);

void er_texture_lod(er_Texture *tex, float *coord, float lod, float *color);

void er_texture_grad(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color);

void er_write_texture(er_Texture *tex, int *coord, int lod, float *color);

/* Program settings */

er_Program* er_create_program();

er_StatusEnum er_delete_program(er_Program *p);

er_StatusEnum er_use_program(er_Program *p);

er_StatusEnum er_uniformi(er_Program *p, unsigned int index, int value);

er_StatusEnum er_uniformf(er_Program *p, unsigned int index, float value);

er_StatusEnum er_uniform_ptr(er_Program *p, unsigned int index, void *pointer);

er_StatusEnum er_uniform_texture_ptr(er_Program *p, unsigned int index, er_Texture *tex_ptr);

er_StatusEnum er_varying_attributes(er_Program *p, int number);

er_StatusEnum er_load_fragment_shader(er_Program *p, void (*fragment_shader)(int, int, er_FragInput*, er_UniVars*));

er_StatusEnum er_load_vertex_shader(er_Program *p, void (*vertex_shader)(er_VertexInput*, er_VertexOutput*, er_UniVars*) );

er_StatusEnum er_load_homogeneous_division(er_Program *p, void (*homogeneous_division)(er_VertexOutput*) );

#endif
