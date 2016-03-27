#ifndef __MAT_VEC__
#define __MAT_VEC__

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define deg_to_rad(deg) ((deg) * M_PI / 180.0f)
#define rad_to_deg(rad) ((rad) * 180.0f / M_PI)
#define max(a, b) ( (a) > (b) ? (a) : (b))
#define min(a, b) ( (a) < (b) ? (a) : (b))
#define uiround(x) (int)(x+0.5f)
#define iround(x) ( (x) > 0.0f ? (int)(x+0.5f): (int)(x-0.5f) )
#define lerp(x, y, alpha) ( x + (alpha) * ( y - x ) )
#define clamp(x, min, max) ( (x) < (min) ? (min) :  (x) > (max) ? (max) : (x) )

#define VAR_X 0
#define VAR_Y 1
#define VAR_Z 2
#define VAR_W 3

#define VAR_S 0
#define VAR_T 1
#define VAR_P 2
#define VAR_Q 3

#define VAR_R 0
#define VAR_G 1
#define VAR_B 2
#define VAR_A 3

typedef float vec2[2];
typedef float vec3[3];
typedef float vec4[4];
typedef float mat2[2][2];
typedef float mat3[3][3];
typedef float mat4[4][4];


/*  VECTOR 2  */

#define dot_vec2(vec0, vec1) ( (vec0)[0] * (vec1)[0] + (vec0)[1] * (vec1)[1] )

float length_vec2(vec2 vec);

float angle_vec2(vec2 vec0, vec2 vec1);

void normalize_vec2(vec2 vec);

void reflection_vec2(vec2 incident_vec, vec2 normal_vec, vec2 reflection_vec);

/*  VECTOR 3  */

#define dot_vec3(vec0, vec1 ) ( (vec0)[0] * (vec1)[0] + (vec0)[1] * (vec1)[1] + (vec0)[2] * (vec1)[2] )

float length_vec3(vec3 vec);

float angle_vec3(vec3 vec0, vec3 vec1);

void cross_product(vec3 vec0, vec3 vec1, vec3 normal);

void normalize_vec3(vec3 vec);

void reflection_vec3(vec3 incident_vec, vec3 normal_vec, vec3 reflection_vec);

void normal_vec3(vec3 vec, vec3 normal);

/*  VECTOR 4  */

#define dot_vec4(vec0, vec1 ) ( (vec0)[0] * (vec1)[0] + (vec0)[1] * (vec1)[1] + (vec0)[2] * (vec1)[2] + (vec0)[3] * (vec1)[3] )

float length_vec4(vec4 vec);

void normalize_vec4(vec4 vec);

/*  MATRIX 2x2  */

#define det2(e0, e1, e2, e3) (e0*e3 - e1*e2)
#define det_mat2(m) det2((m)[0][0], (m)[0][1], (m)[1][0], (m)[1][1])

void assign_mat2(mat2 dst, mat2 src);

void assignT_mat2(mat2 dst, mat2 src);

void identity_mat2(mat2 mat);

void mult_mat2_vec2(mat2 mat, vec2 vec, vec2 out);

void multd_mat2_vec2(mat2 mat, vec2 vec, vec2 out);

void mult_mat2_mat2(mat2 mat0, mat2 mat1, mat2 out);

void multd_mat2_mat2(mat2 mat0, mat2 mat1, mat2 out);

void inverse_mat2(mat2 in, mat2 out);

void rotate_mat2(mat2 mat, float angle);

void rotate_align_mat2(mat2 mat, vec2 from_dir, vec2 to_dir);

void scale_mat2(mat2 mat, float sx, float sy);

void scale_dir_mat2(mat2 mat, float k, float x, float y);

/*  MATRIX 3x3  */

#define det3(e0, e1, e2, e3, e4, e5, e6, e7, e8) (e0*e4*e8 + e1*e5*e6 + e2*e3*e7 - e2*e4*e6 - e0*e5*e7 - e1*e3*e8)
#define det_mat3(m) det3( m[0][0], (m)[0][1], (m)[0][2], (m)[1][0], (m)[1][1], (m)[1][2], (m)[2][0], (m)[2][1], (m)[2][2] )

void assign_mat3(mat3 dst, mat3 src);

void assignT_mat3(mat3 dst, mat3 src);

void identity_mat3(mat3 mat);

void mult_mat3_vec3(mat3 mat, vec3 vec, vec3 out);

void multd_mat3_vec3(mat3 mat, vec3 vec, vec3 out);

void mult_mat3_mat3(mat3 mat0, mat3 mat1, mat3 out);

void multd_mat3_mat3(mat3 mat0, mat3 mat1, mat3 out);

void inverse_mat3(mat3 mat, mat3 inv);

void rotate_mat3(mat3 mat, float angle, float x, float y, float z);

void rotate_align_mat3(mat3 mat, vec3 from_dir, vec3 to_dir);

void rotatex_mat3(mat3 mat, float angle);

void rotatey_mat3(mat3 mat, float angle);

void rotatez_mat3(mat3 mat, float angle);

void scale_mat3(mat3 mat, float sx, float sy, float sz);

void scale_dir_mat3(mat3 mat, float k, float x, float y, float z);

/*  MATRIX 4x4  */

#define det_mat4(m) (  (m)[0][0] * det3( (m)[1][1], (m)[1][2], (m)[1][3], (m)[2][1], (m)[2][2], (m)[2][3], (m)[3][1], (m)[3][2], (m)[3][3] ) \
                     - (m)[0][1] * det3( (m)[1][0], (m)[1][2], (m)[1][3], (m)[2][0], (m)[2][2], (m)[2][3], (m)[3][0], (m)[3][2], (m)[3][3] ) \
                     + (m)[0][2] * det3( (m)[1][0], (m)[1][1], (m)[1][3], (m)[2][0], (m)[2][1], (m)[2][3], (m)[3][0], (m)[3][1], (m)[3][3] ) \
                     - (m)[0][3] * det3( (m)[1][0], (m)[1][1], (m)[1][2], (m)[2][0], (m)[2][1], (m)[2][2], (m)[3][0], (m)[3][1], (m)[3][2] ) )

void assign_mat4(mat4 dst, mat4 src);

void assignT_mat4(mat4 dst, mat4 src);

void identity_mat4(mat4 mat);

void mult_mat4_vec4(mat4 mat, vec4 vec, vec4 out);

void multd_mat4_vec4(mat4 mat, vec4 vec, vec4 out);

void mult_mat4_mat4(mat4 mat0, mat4 mat1, mat4 out);

void multd_mat4_mat4(mat4 mat0, mat4 mat1, mat4 out);

void inverse_mat4(mat4 mat, mat4 inv);

void inverse_modeling_mat4(mat4 mat, mat4 inv);

void rotate_mat4(mat4 mat, float angle, float x, float y, float z);

void rotate_align_mat4(mat4 mat, vec3 from_dir, vec3 to_dir);

void rotatex_mat4(mat4 mat, float angle);

void rotatey_mat4(mat4 mat, float angle);

void rotatez_mat4(mat4 mat, float angle);

void translate_mat4(mat4 mat, float x, float y, float z);

void scale_mat4(mat4 mat, float sx, float sy, float sz);

void scale_dir_mat4(mat4 mat, float k, float x, float y, float z);

void scale_dir_mat4(mat4 mat, float k, float x, float y, float z);

void frustum_mat4(mat4 mat, float l, float r, float b, float t, float n, float f);

void perspective_mat4(mat4 mat, float angle, float aspect_ratio, float n, float f);

void orthographic_mat4(mat4 mat, float l, float r, float b, float t, float n, float f);

void lookat_mat4(mat4 mat, vec3 eye_pos, vec3 at_pos, vec3 up_vector);

#endif