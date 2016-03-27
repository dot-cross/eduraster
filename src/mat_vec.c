#include "mat_vec.h"

/*  VECTOR 2  */

float length_vec2(vec2 vec){
    return sqrt(vec[0]*vec[0]+vec[1]*vec[1]);
}

float angle_vec2(vec2 vec0, vec2 vec1){
    float length0 = length_vec2(vec0);
    float length1 = length_vec2(vec1);
    float dot = dot_vec2(vec0, vec1);
    float value = clamp(dot / (length0 * length1), -1.0f, 1.0f);
    return acos(value);
}

void normalize_vec2(vec2 vec){
    float length = length_vec2(vec);
    vec[0] /= length;
    vec[1] /= length;
}

void reflection_vec2(vec2 incident_vec, vec2 normal_vec, vec2 reflection_vec){
    float normal_dot_incident = dot_vec2(incident_vec, normal_vec);
    reflection_vec[0] = incident_vec[0] - 2.0f * normal_dot_incident * normal_vec[0];
    reflection_vec[1] = incident_vec[1] - 2.0f * normal_dot_incident * normal_vec[1];
}

/*  VECTOR 3  */

float length_vec3(vec3 vec){
    return sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
}

float angle_vec3(vec3 vec0, vec3 vec1){
    float length0 = length_vec3(vec0);
    float length1 = length_vec3(vec1);
    float dot = dot_vec3(vec0, vec1);
    float value = clamp(dot / (length0*length1), -1.0f, 1.0f);
    return acos(value);
}

void cross_product(vec3 vec0, vec3 vec1, vec3 normal){
    normal[0] = vec0[1] * vec1[2] - vec1[1] * vec0[2];
    normal[1] = vec1[0] * vec0[2] - vec0[0] * vec1[2];
    normal[2] = vec0[0] * vec1[1] - vec1[0] * vec0[1];
}

void normalize_vec3(vec3 vec){
    float length = length_vec3(vec);
    vec[0] /= length;
    vec[1] /= length;
    vec[2] /= length;
}

void reflection_vec3(vec3 incident_vec, vec3 normal_vec, vec3 reflection_vec){
    float normal_dot_incident = dot_vec3(incident_vec, normal_vec);
    reflection_vec[0] = incident_vec[0] - 2.0f * normal_dot_incident * normal_vec[0]; 
    reflection_vec[1] = incident_vec[1] - 2.0f * normal_dot_incident * normal_vec[1];
    reflection_vec[2] = incident_vec[2] - 2.0f * normal_dot_incident * normal_vec[2];
}

void normal_vec3(vec3 vec, vec3 normal){
    int mi1 = 1, mi2 = 2, ma = 0;
    if(fabs(vec[1]) > fabs(vec[ma])){
        mi1 = 0;
        ma = 1;
        mi2 = 2;
    }
    if(fabs(vec[2]) > fabs(vec[ma])){
        mi1 = 0;
        mi2 = 1;
        ma = 2;
    }
    normal[mi1] = 1.0f;
    normal[mi2] = 1.0f;
    normal[ma] = (-vec[mi1]-vec[mi2]) / vec[ma];
    normalize_vec3(normal);
}

/* VECTOR 4 */

float length_vec4(vec4 vec){
    return sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2] + vec[3]*vec[3]);
}

void normalize_vec4(vec4 vec){
    float length = length_vec4(vec);
    vec[0] /= length;
    vec[1] /= length;
    vec[2] /= length;
    vec[3] /= length;
}

/* MATRIX 2x2 */

void assign_mat2(mat2 dst, mat2 src){
    dst[0][0] = src[0][0];  dst[0][1] = src[0][1];
    dst[1][0] = src[1][0];  dst[1][1] = src[1][1];
}

void assignT_mat2(mat2 dst, mat2 src){
    dst[0][0] = src[0][0];  dst[0][1] = src[1][0];
    dst[1][0] = src[0][1];  dst[1][1] = src[1][1];
}

void identity_mat2(mat2 mat){
    mat[0][0] = 1.0f;  mat[0][1] = 0.0f;
    mat[1][0] = 0.0f;  mat[1][1] = 1.0f;
}

void mult_mat2_vec2(mat2 mat, vec2 vec, vec2 out){
    vec2 aux;
    aux[0] = mat[0][0] * vec[0] + mat[0][1] * vec[1];
    aux[1] = mat[1][0] * vec[0] + mat[1][1] * vec[1];
    out[0] = aux[0];
    out[1] = aux[1];
}

void multd_mat2_vec2(mat2 mat, vec2 vec, vec2 out){
    out[0] = mat[0][0] * vec[0] + mat[0][1] * vec[1];
    out[1] = mat[1][0] * vec[0] + mat[1][1] * vec[1];
}

void mult_mat2_mat2(mat2 mat0, mat2 mat1, mat2 out){
    mat2 aux;
    aux[0][0] = mat0[0][0] * mat1[0][0] +
                mat0[0][1] * mat1[1][0];
    aux[0][1] = mat0[0][0] * mat1[0][1] +
                mat0[0][1] * mat1[1][1];
    aux[1][0] = mat0[1][0] * mat1[0][0] +
                mat0[1][1] * mat1[1][0];
    aux[1][1] = mat0[1][0] * mat1[0][1] +
                mat0[1][1] * mat1[1][1];
    assign_mat2(out, aux);
}

void multd_mat2_mat2(mat2 mat0, mat2 mat1, mat2 out){
    out[0][0] = mat0[0][0] * mat1[0][0] +
                mat0[0][1] * mat1[1][0];
    out[0][1] = mat0[0][0] * mat1[0][1] +
                mat0[0][1] * mat1[1][1];
    out[1][0] = mat0[1][0] * mat1[0][0] +
                mat0[1][1] * mat1[1][0];
    out[1][1] = mat0[1][0] * mat1[0][1] +
                mat0[1][1] * mat1[1][1];
}

void inverse_mat2(mat2 mat, mat2 inv){
	float det = det_mat2(mat);
	inv[0][0] = mat[1][1] / det;   inv[0][1] = -mat[0][1] / det;
    inv[1][0] = -mat[1][0] / det;  inv[1][1] = mat[0][0] / det;
}

void rotate_mat2(mat2 mat, float angle){
    float c = cos(angle);
    float s = sin(angle);
    mat[0][0] = c;  mat[0][1] = -s;
    mat[1][0] = s;  mat[1][1] = c;
}

void rotate_align_mat2(mat2 mat, vec2 from_dir, vec2 to_dir){
    float c = dot_vec2(from_dir, to_dir);
    float s = from_dir[0]*to_dir[1]-to_dir[0]*from_dir[1];
    mat[0][0] = c;  mat[0][1] = -s;
    mat[1][0] = s;  mat[1][1] = c;
}

void scale_mat2(mat2 mat, float sx, float sy){
    mat[0][0] = sx  ;  mat[0][1] = 0.0f;
    mat[1][0] = 0.0f;  mat[1][1] = sy;
}

void scale_dir_mat2(mat2 mat, float k, float x, float y){
  float k_minus_1 = k - 1.0f;
  mat[0][0] = k_minus_1 * x * x + 1.0f;  mat[0][1] = k_minus_1 * x * y;
  mat[1][0] = k_minus_1 * x * y;         mat[1][1] = k_minus_1 * y * y + 1.0f;
}

/* MATRIX 3x3 */

void assign_mat3(mat3 dst, mat3 src){
    dst[0][0] = src[0][0];  dst[0][1] = src[0][1];  dst[0][2] = src[0][2];
    dst[1][0] = src[1][0];  dst[1][1] = src[1][1];  dst[1][2] = src[1][2];
    dst[2][0] = src[2][0];  dst[2][1] = src[2][1];  dst[2][2] = src[2][2];
}

void assignT_mat3(mat3 dst, mat3 src){
    dst[0][0] = src[0][0];  dst[0][1] = src[1][0];  dst[0][2] = src[2][0];
    dst[1][0] = src[0][1];  dst[1][1] = src[1][1];  dst[1][2] = src[2][1];
    dst[2][0] = src[0][2];  dst[2][1] = src[1][2];  dst[2][2] = src[2][2];
}

void identity_mat3(mat3 mat){
    mat[0][0] = 1.0f;  mat[0][1] = 0.0f;  mat[0][2] = 0.0f;
    mat[1][0] = 0.0f;  mat[1][1] = 1.0f;  mat[1][2] = 0.0f;
    mat[2][0] = 0.0f;  mat[2][1] = 0.0f;  mat[2][2] = 1.0f;
}

void mult_mat3_vec3(mat3 mat, vec3 vec, vec3 out){
    vec3 aux;
    aux[0] = mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2];
    aux[1] = mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2];
    aux[2] = mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2];
    out[0] = aux[0];
    out[1] = aux[1];
    out[2] = aux[2];
}

void multd_mat3_vec3(mat3 mat, vec3 vec, vec3 out){
    out[0] = mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2];
    out[1] = mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2];
    out[2] = mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2];
}

void mult_mat3_mat3(mat3 mat0, mat3 mat1, mat3 out){
    mat3 aux;
    aux[0][0] = mat0[0][0] * mat1[0][0] +
                mat0[0][1] * mat1[1][0] +
                mat0[0][2] * mat1[2][0];
    aux[0][1] = mat0[0][0] * mat1[0][1] + 
                mat0[0][1] * mat1[1][1] +
                mat0[0][2] * mat1[2][1];
    aux[0][2] = mat0[0][0] * mat1[0][2] +
                mat0[0][1] * mat1[1][2] +
                mat0[0][2] * mat1[2][2];
    aux[1][0] = mat0[1][0] * mat1[0][0] +
                mat0[1][1] * mat1[1][0] +
                mat0[1][2] * mat1[2][0];
    aux[1][1] = mat0[1][0] * mat1[0][1] +
                mat0[1][1] * mat1[1][1] +
                mat0[1][2] * mat1[2][1];
    aux[1][2] = mat0[1][0] * mat1[0][2] +
                mat0[1][1] * mat1[1][2] +
                mat0[1][2] * mat1[2][2];
    aux[2][0] = mat0[2][0] * mat1[0][0] +
                mat0[2][1] * mat1[1][0] +
                mat0[2][2] * mat1[2][0];
    aux[2][1] = mat0[2][0] * mat1[0][1] +
                mat0[2][1] * mat1[1][1] +
                mat0[2][2] * mat1[2][1];
    aux[2][2] = mat0[2][0] * mat1[0][2] +
                mat0[2][1] * mat1[1][2] +
                mat0[2][2] * mat1[2][2];
    assign_mat3(out, aux);
}

void multd_mat3_mat3(mat3 mat0, mat3 mat1, mat3 out){
    out[0][0] = mat0[0][0] * mat1[0][0] +
                mat0[0][1] * mat1[1][0] +
                mat0[0][2] * mat1[2][0];
    out[0][1] = mat0[0][0] * mat1[0][1] +
                mat0[0][1] * mat1[1][1] +
                mat0[0][2] * mat1[2][1];
    out[0][2] = mat0[0][0] * mat1[0][2] +
                mat0[0][1] * mat1[1][2] +
                mat0[0][2] * mat1[2][2];
    out[1][0] = mat0[1][0] * mat1[0][0] +
                mat0[1][1] * mat1[1][0] +
                mat0[1][2] * mat1[2][0];
    out[1][1] = mat0[1][0] * mat1[0][1] +
                mat0[1][1] * mat1[1][1] +
                mat0[1][2] * mat1[2][1];
    out[1][2] = mat0[1][0] * mat1[0][2] +
                mat0[1][1] * mat1[1][2] +
                mat0[1][2] * mat1[2][2];
    out[2][0] = mat0[2][0] * mat1[0][0] +
                mat0[2][1] * mat1[1][0] +
                mat0[2][2] * mat1[2][0];
    out[2][1] = mat0[2][0] * mat1[0][1] +
                mat0[2][1] * mat1[1][1] +
                mat0[2][2] * mat1[2][1];
    out[2][2] = mat0[2][0] * mat1[0][2] +
                mat0[2][1] * mat1[1][2] +
                mat0[2][2] * mat1[2][2];
}

void inverse_mat3(mat3 mat, mat3 inv){
    float det = det_mat3(mat);
	inv[0][0] = det2(mat[1][1], mat[1][2],
                     mat[2][1], mat[2][2]) / det;
    inv[0][1] = -det2(mat[0][1], mat[0][2],
                      mat[2][1], mat[2][2]) / det;
	inv[0][2] = det2(mat[0][1], mat[0][2],
                     mat[1][1], mat[1][2]) / det;
	inv[1][0] = -det2(mat[1][0], mat[1][2],
                      mat[2][0], mat[2][2]) / det;
	inv[1][1] = det2(mat[0][0], mat[0][2],
                     mat[2][0], mat[2][2]) / det;
	inv[1][2] = -det2(mat[0][0], mat[0][2],
                      mat[1][0], mat[1][2]) / det;
    inv[2][0] = det2(mat[1][0], mat[1][1],
                     mat[2][0], mat[2][1]) / det;
    inv[2][1] = -det2(mat[0][0], mat[0][1],
                      mat[2][0], mat[2][1]) / det;
	inv[2][2] = det2(mat[0][0], mat[0][1],
                     mat[1][0], mat[1][1]) / det;
}

void rotate_mat3(mat3 mat, float angle, float x, float y, float z){
    float c = cos(angle);
    float one_minus_c = 1.0f - c;
    float s = sin(angle);
    mat[0][0] = x*x*one_minus_c + c;    mat[0][1] = x*y*one_minus_c - z*s;  mat[0][2] = x*z*one_minus_c + y*s;
    mat[1][0] = x*y*one_minus_c + z*s;  mat[1][1] = y*y*one_minus_c + c;    mat[1][2] = y*z*one_minus_c - x*s;
    mat[2][0] = x*z*one_minus_c - y*s;  mat[2][1] = y*z*one_minus_c + x*s;  mat[2][2] = z*z*one_minus_c + c;
}

void rotate_align_mat3(mat3 mat, vec3 from_dir, vec3 to_dir){

    vec3 axis;
    cross_product(from_dir, to_dir, axis);
    float c = dot_vec3(from_dir, to_dir);
    float k = 1.0f/(1.0f + c);
    float x = axis[0];
    float y = axis[1];
    float z = axis[2];

    mat[0][0] = x*x*k + c;  mat[0][1] = x*y*k - z;  mat[0][2] = x*z*k + y;
    mat[1][0] = x*y*k + z;  mat[1][1] = y*y*k + c;  mat[1][2] = y*z*k - x;
    mat[2][0] = x*z*k - y;  mat[2][1] = y*z*k + x;  mat[2][2] = z*z*k + c;
}

void rotatex_mat3(mat3 mat, float angle){

    float c,s;
    c = cos(angle);
    s = sin(angle);

    mat[0][0] = 1.0f;  mat[0][1] = 0.0f;  mat[0][2] = 0.0f;
    mat[1][0] = 0.0f;  mat[1][1] = c;     mat[1][2] = -s;
    mat[2][0] = 0.0f;  mat[2][1] = s;     mat[2][2] = c;
}

void rotatey_mat3(mat3 mat, float angle){

    float c,s;
    c = cos(angle);
    s = sin(angle);

    mat[0][0] = c;     mat[0][1] = 0.0f;  mat[0][2] = s;
    mat[1][0] = 0.0f;  mat[1][1] = 1.0f;  mat[1][2] = 0.0f;
    mat[2][0] = -s;    mat[2][1] = 0.0f;  mat[2][2] = c;
}

void rotatez_mat3(mat3 mat, float angle){

    float c,s;
    c = cos(angle);
    s = sin(angle);

    mat[0][0] = c;     mat[0][1] = -s;    mat[0][2] = 0.0f;
    mat[1][0] = s;     mat[1][1] = c;     mat[1][2] = 0.0f;
    mat[2][0] = 0.0f;  mat[2][1] = 0.0f;  mat[2][2] = 1.0f;
}

void scale_mat3(mat3 mat, float sx, float sy, float sz){
    mat[0][0] = sx  ;  mat[0][1] = 0.0f;  mat[0][2] = 0.0f;
    mat[1][0] = 0.0f;  mat[1][1] = sy;    mat[1][2] = 0.0f;
    mat[2][0] = 0.0f;  mat[2][1] = 0.0f;  mat[2][2] = sz;
}

void scale_dir_mat3(mat3 mat, float k, float x, float y, float z){

    float k_minus_1 = k - 1.0f;
    mat[0][0] = k_minus_1 * x * x + 1.0f;  mat[0][1] = k_minus_1 * x * y;         mat[0][2] = k_minus_1 * x * z;
    mat[1][0] = k_minus_1 * x * y;         mat[1][1] = k_minus_1 * y * y + 1.0f;  mat[1][2] = k_minus_1 * y * z;
    mat[2][0] = k_minus_1 * x * z;         mat[2][1] = k_minus_1 * y * z;         mat[2][2] = k_minus_1 * z *z + 1.0f;

}

/*  MATRIX 4x4  */

void assign_mat4(mat4 dst, mat4 src){
    dst[0][0] = src[0][0];  dst[0][1] = src[0][1];  dst[0][2] = src[0][2];  dst[0][3] = src[0][3];
    dst[1][0] = src[1][0];  dst[1][1] = src[1][1];  dst[1][2] = src[1][2];  dst[1][3] = src[1][3];
    dst[2][0] = src[2][0];  dst[2][1] = src[2][1];  dst[2][2] = src[2][2];  dst[2][3] = src[2][3];
    dst[3][0] = src[3][0];  dst[3][1] = src[3][1];  dst[3][2] = src[3][2];  dst[3][3] = src[3][3];
}

void assignT_mat4(mat4 dst, mat4 src){
    dst[0][0] = src[0][0];  dst[0][1] = src[1][0];  dst[0][2] = src[2][0];  dst[0][3] = src[3][0];
    dst[1][0] = src[0][1];  dst[1][1] = src[1][1];  dst[1][2] = src[2][1];  dst[1][3] = src[3][1];
    dst[2][0] = src[0][2];  dst[2][1] = src[1][2];  dst[2][2] = src[2][2];  dst[2][3] = src[3][2];
    dst[3][0] = src[0][3];  dst[3][1] = src[1][3];  dst[3][2] = src[2][3];  dst[3][3] = src[3][3];
}

void identity_mat4(mat4 mat){
    mat[0][0] = 1.0f;  mat[0][1] = 0.0f;  mat[0][2] = 0.0f;  mat[0][3] = 0.0f;
    mat[1][0] = 0.0f;  mat[1][1] = 1.0f;  mat[1][2] = 0.0f;  mat[1][3] = 0.0f;
    mat[2][0] = 0.0f;  mat[2][1] = 0.0f;  mat[2][2] = 1.0f;  mat[2][3] = 0.0f;
    mat[3][0] = 0.0f;  mat[3][1] = 0.0f;  mat[3][2] = 0.0f;  mat[3][3] = 1.0f;
}

void mult_mat4_vec4(mat4 mat, vec4 vec, vec4 out){
    vec4 aux;
    aux[0] = mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3] * vec[3];
    aux[1] = mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3] * vec[3];
    aux[2] = mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3] * vec[3];
    aux[3] = mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[3][3] * vec[3];
    out[0] = aux[0];
    out[1] = aux[1];
    out[2] = aux[2];
    out[3] = aux[3];
}

void multd_mat4_vec4(mat4 mat, vec4 vec, vec4 out){
    out[0] = mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3] * vec[3];
    out[1] = mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3] * vec[3];
    out[2] = mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3] * vec[3];
    out[3] = mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[3][3] * vec[3];
}

void mult_mat4_mat4(mat4 mat0, mat4 mat1, mat4 out){
    mat4 aux;
    aux[0][0] = mat0[0][0] * mat1[0][0] +
                mat0[0][1] * mat1[1][0] +
                mat0[0][2] * mat1[2][0] +
                mat0[0][3] * mat1[3][0];
    aux[0][1] = mat0[0][0] * mat1[0][1] +
                mat0[0][1] * mat1[1][1] +
                mat0[0][2] * mat1[2][1] +
                mat0[0][3] * mat1[3][1];
    aux[0][2] = mat0[0][0] * mat1[0][2] +
                mat0[0][1] * mat1[1][2] +
                mat0[0][2] * mat1[2][2] +
                mat0[0][3] * mat1[3][2];
    aux[0][3] = mat0[0][0] * mat1[0][3] +
                mat0[0][1] * mat1[1][3] +
                mat0[0][2] * mat1[2][3] +
                mat0[0][3] * mat1[3][3];
    aux[1][0] = mat0[1][0] * mat1[0][0] +
                mat0[1][1] * mat1[1][0] +
                mat0[1][2] * mat1[2][0] +
                mat0[1][3] * mat1[3][0];
    aux[1][1] = mat0[1][0] * mat1[0][1] +
                mat0[1][1] * mat1[1][1] +
                mat0[1][2] * mat1[2][1] +
                mat0[1][3] * mat1[3][1];
    aux[1][2] = mat0[1][0] * mat1[0][2] +
                mat0[1][1] * mat1[1][2] +
                mat0[1][2] * mat1[2][2] +
                mat0[1][3] * mat1[3][2];
    aux[1][3] = mat0[1][0] * mat1[0][3] +
                mat0[1][1] * mat1[1][3] +
                mat0[1][2] * mat1[2][3] +
                mat0[1][3] * mat1[3][3];
    aux[2][0] = mat0[2][0] * mat1[0][0] +
                mat0[2][1] * mat1[1][0] +
                mat0[2][2] * mat1[2][0] +
                mat0[2][3] * mat1[3][0];
    aux[2][1] = mat0[2][0] * mat1[0][1] +
                mat0[2][1] * mat1[1][1] +
                mat0[2][2] * mat1[2][1] +
                mat0[2][3] * mat1[3][1];
    aux[2][2] = mat0[2][0] * mat1[0][2] +
                mat0[2][1] * mat1[1][2] + 
                mat0[2][2] * mat1[2][2] +
                mat0[2][3] * mat1[3][2];
    aux[2][3] = mat0[2][0] * mat1[0][3] +
                mat0[2][1] * mat1[1][3] +
                mat0[2][2] * mat1[2][3] +
                mat0[2][3] * mat1[3][3];
    aux[3][0] = mat0[3][0] * mat1[0][0] +
                mat0[3][1] * mat1[1][0] +
                mat0[3][2] * mat1[2][0] +
                mat0[3][3] * mat1[3][0];
    aux[3][1] = mat0[3][0] * mat1[0][1] +
                mat0[3][1] * mat1[1][1] +
                mat0[3][2] * mat1[2][1] +
                mat0[3][3] * mat1[3][1];
    aux[3][2] = mat0[3][0] * mat1[0][2] +
                mat0[3][1] * mat1[1][2] +
                mat0[3][2] * mat1[2][2] +
                mat0[3][3] * mat1[3][2];
    aux[3][3] = mat0[3][0] * mat1[0][3] +
                mat0[3][1] * mat1[1][3] +
                mat0[3][2] * mat1[2][3] +
                mat0[3][3] * mat1[3][3];
    assign_mat4(out, aux);
}

void multd_mat4_mat4(mat4 mat0, mat4 mat1, mat4 out){
    out[0][0] = mat0[0][0] * mat1[0][0] +
                mat0[0][1] * mat1[1][0] +
                mat0[0][2] * mat1[2][0] +
                mat0[0][3] * mat1[3][0];
    out[0][1] = mat0[0][0] * mat1[0][1] +
                mat0[0][1] * mat1[1][1] +
                mat0[0][2] * mat1[2][1] +
                mat0[0][3] * mat1[3][1];
    out[0][2] = mat0[0][0] * mat1[0][2] +
                mat0[0][1] * mat1[1][2] +
                mat0[0][2] * mat1[2][2] +
                mat0[0][3] * mat1[3][2];
    out[0][3] = mat0[0][0] * mat1[0][3] +
                mat0[0][1] * mat1[1][3] +
                mat0[0][2] * mat1[2][3] +
                mat0[0][3] * mat1[3][3];
    out[1][0] = mat0[1][0] * mat1[0][0] +
                mat0[1][1] * mat1[1][0] +
                mat0[1][2] * mat1[2][0] +
                mat0[1][3] * mat1[3][0];
    out[1][1] = mat0[1][0] * mat1[0][1] +
                mat0[1][1] * mat1[1][1] +
                mat0[1][2] * mat1[2][1] +
                mat0[1][3] * mat1[3][1];
    out[1][2] = mat0[1][0] * mat1[0][2] +
                mat0[1][1] * mat1[1][2] +
                mat0[1][2] * mat1[2][2] +
                mat0[1][3] * mat1[3][2];
    out[1][3] = mat0[1][0] * mat1[0][3] +
                mat0[1][1] * mat1[1][3] +
                mat0[1][2] * mat1[2][3] +
                mat0[1][3] * mat1[3][3];
    out[2][0] = mat0[2][0] * mat1[0][0] +
                mat0[2][1] * mat1[1][0] +
                mat0[2][2] * mat1[2][0] +
                mat0[2][3] * mat1[3][0];
    out[2][1] = mat0[2][0] * mat1[0][1] +
                mat0[2][1] * mat1[1][1] +
                mat0[2][2] * mat1[2][1] +
                mat0[2][3] * mat1[3][1];
    out[2][2] = mat0[2][0] * mat1[0][2] +
                mat0[2][1] * mat1[1][2] + 
                mat0[2][2] * mat1[2][2] +
                mat0[2][3] * mat1[3][2];
    out[2][3] = mat0[2][0] * mat1[0][3] +
                mat0[2][1] * mat1[1][3] +
                mat0[2][2] * mat1[2][3] +
                mat0[2][3] * mat1[3][3];
    out[3][0] = mat0[3][0] * mat1[0][0] +
                mat0[3][1] * mat1[1][0] +
                mat0[3][2] * mat1[2][0] +
                mat0[3][3] * mat1[3][0];
    out[3][1] = mat0[3][0] * mat1[0][1] +
                mat0[3][1] * mat1[1][1] +
                mat0[3][2] * mat1[2][1] +
                mat0[3][3] * mat1[3][1];
    out[3][2] = mat0[3][0] * mat1[0][2] +
                mat0[3][1] * mat1[1][2] +
                mat0[3][2] * mat1[2][2] +
                mat0[3][3] * mat1[3][2];
    out[3][3] = mat0[3][0] * mat1[0][3] +
                mat0[3][1] * mat1[1][3] +
                mat0[3][2] * mat1[2][3] +
                mat0[3][3] * mat1[3][3];
}

void inverse_mat4(mat4 mat, mat4 inv){
    float det = det_mat4(mat);
    inv[0][0] = det3(mat[1][1], mat[1][2], mat[1][3],
                     mat[2][1], mat[2][2], mat[2][3],
                     mat[3][1], mat[3][2], mat[3][3]) / det;
    inv[0][1] = -det3(mat[0][1], mat[0][2], mat[0][3],
                      mat[2][1], mat[2][2], mat[2][3],
                      mat[3][1], mat[3][2], mat[3][3]) / det;
    inv[0][2] = det3(mat[0][1], mat[0][2], mat[0][3],
                     mat[1][1], mat[1][2], mat[1][3],
                     mat[3][1], mat[3][2], mat[3][3]) / det;
    inv[0][3] = -det3(mat[0][1], mat[0][2], mat[0][3],
                      mat[1][1], mat[1][2], mat[1][3],
                      mat[2][1], mat[2][2], mat[2][3]) / det;
    inv[1][0] = -det3(mat[1][0], mat[1][2], mat[1][3],
                      mat[2][0], mat[2][2], mat[2][3],
                      mat[3][0], mat[3][2], mat[3][3]) / det;
    inv[1][1] = det3(mat[0][0], mat[0][2], mat[0][3],
                     mat[2][0], mat[2][2], mat[2][3],
                     mat[3][0], mat[3][2], mat[3][3]) / det;
    inv[1][2] = -det3(mat[0][0], mat[0][2], mat[0][3],
                      mat[1][0], mat[1][2], mat[1][3],
                      mat[3][0], mat[3][2], mat[3][3]) / det;
    inv[1][3] = det3(mat[0][0], mat[0][2], mat[0][3],
                     mat[1][0], mat[1][2], mat[1][3],
                     mat[2][0], mat[2][2], mat[2][3]) / det;
    inv[2][0] = det3(mat[1][0], mat[1][1], mat[1][3],
                     mat[2][0], mat[2][1], mat[2][3],
                     mat[3][0], mat[3][1], mat[3][3]) / det;
    inv[2][1] = -det3(mat[0][0], mat[0][1], mat[0][3],
                      mat[2][0], mat[2][1], mat[2][3],
                      mat[3][0], mat[3][1], mat[3][3]) / det;
    inv[2][2] = det3(mat[0][0], mat[0][1], mat[0][3],
                     mat[1][0], mat[1][1], mat[1][3],
                     mat[3][0], mat[3][1], mat[3][3]) / det;
    inv[2][3] = -det3(mat[0][0], mat[0][1], mat[0][3],
                      mat[1][0], mat[1][1], mat[1][3],
                      mat[2][0], mat[2][1], mat[2][3]) / det;
    inv[3][0] = -det3(mat[1][0], mat[1][1], mat[1][2],
                      mat[2][0], mat[2][1], mat[2][2],
                      mat[3][0], mat[3][1], mat[3][2]) / det;
    inv[3][1] = det3(mat[0][0], mat[0][1], mat[0][2],
                     mat[2][0], mat[2][1], mat[2][2],
                     mat[3][0], mat[3][1], mat[3][2]) / det;
    inv[3][2] = -det3(mat[0][0], mat[0][1], mat[0][2],
                      mat[1][0], mat[1][1], mat[1][2],
                      mat[3][0], mat[3][1], mat[3][2]) / det;
    inv[3][3] = det3(mat[0][0], mat[0][1], mat[0][2],
                     mat[1][0], mat[1][1], mat[1][2],
                     mat[2][0], mat[2][1], mat[2][2]) / det;
}

void inverse_modeling_mat4(mat4 mat, mat4 inv){
    
	float det = det3(mat[0][0], mat[0][1], mat[0][2], mat[1][0], mat[1][1], mat[1][2], mat[2][0], mat[2][1], mat[2][2]);

	//Calculate the inverse of the left top 3x3 matrix
	inv[0][0] = det2(mat[1][1], mat[1][2],
                     mat[2][1], mat[2][2]) / det;
	inv[0][1] = -det2(mat[0][1], mat[0][2],
                      mat[2][1], mat[2][2]) / det;
	inv[0][2] = det2(mat[0][1], mat[0][2],
                     mat[1][1], mat[1][2]) / det;
	inv[1][0] = -det2(mat[1][0], mat[1][2],
                      mat[2][0], mat[2][2]) / det;
	inv[1][1] = det2(mat[0][0], mat[0][2],
                     mat[2][0], mat[2][2]) / det;
	inv[1][2] = -det2(mat[0][0], mat[0][2],
                      mat[1][0], mat[1][2]) / det;
	inv[2][0] = det2(mat[1][0], mat[1][1],
                     mat[2][0], mat[2][1]) / det;
	inv[2][1] = -det2(mat[0][0], mat[0][1],
                      mat[2][0], mat[2][1]) / det;
	inv[2][2] = det2(mat[0][0], mat[0][1],
                     mat[1][0], mat[1][1]) / det;

	//Multiply translation vector by inverse
	//Set translation vector
	float translation_vec[3];
	translation_vec[0] = mat[0][3];
	translation_vec[1] = mat[1][3];
	translation_vec[2] = mat[2][3];

	inv[0][3] = - ( inv[0][0] * translation_vec[0] + inv[0][1] * translation_vec[1] + inv[0][2] * translation_vec[2] );
	inv[1][3] = - ( inv[1][0] * translation_vec[0] + inv[1][1] * translation_vec[1] + inv[1][2] * translation_vec[2] );
	inv[2][3] = - ( inv[2][0] * translation_vec[0] + inv[2][1] * translation_vec[1] + inv[2][2] * translation_vec[2] );

	//Set last row, first 3 entries to zero and last entry to one
	inv[3][0] = 0.0f;
	inv[3][1] = 0.0f;
	inv[3][2] = 0.0f;
	inv[3][3] = 1.0f;
}

void rotate_mat4(mat4 mat, float angle, float x, float y, float z){
    
    float c = cos(angle);
    float one_minus_c = 1.0f - c;
    float s = sin(angle);

    mat[0][0] = x*x*one_minus_c + c;    mat[0][1] = x*y*one_minus_c - z*s;  mat[0][2] = x*z*one_minus_c + y*s;  mat[0][3] = 0.0f;
    mat[1][0] = x*y*one_minus_c + z*s;  mat[1][1] = y*y*one_minus_c + c;    mat[1][2] = y*z*one_minus_c - x*s;  mat[1][3] = 0.0f;
    mat[2][0] = x*z*one_minus_c - y*s;  mat[2][1] = y*z*one_minus_c + x*s;  mat[2][2] = z*z*one_minus_c + c;    mat[3][2] = 0.0f;
    mat[3][0] = 0.0f;                   mat[3][1] = 0.0f;                   mat[2][3] = 0.0f;                   mat[3][3] = 1.0f;
}

void rotate_align_mat4(mat4 mat, vec3 from_dir, vec3 to_dir){

    vec3 axis;
    cross_product(from_dir, to_dir, axis);
    float c = dot_vec3(from_dir, to_dir);
    float k = 1.0f/(1.0f + c);
    float x = axis[0];
    float y = axis[1];
    float z = axis[2];

    mat[0][0] = x*x*k + c;  mat[0][1] = x*y*k - z;  mat[0][2] = x*z*k + y;  mat[0][3] = 0.0f;
    mat[1][0] = x*y*k + z;  mat[1][1] = y*y*k + c;  mat[1][2] = y*z*k - x;  mat[1][3] = 0.0f;
    mat[2][0] = x*z*k - y;  mat[2][1] = y*z*k + x;  mat[2][2] = z*z*k + c;  mat[2][3] = 0.0f;
    mat[3][0] = 0.0f;       mat[3][1] = 0.0f;       mat[3][2] = 0.0f;       mat[3][3] = 1.0f;
}

void rotatex_mat4(mat4 mat, float angle){

    float c,s;
    c = cos(angle);
    s = sin(angle);

    mat[0][0] = 1.0f;  mat[0][1] = 0.0f;  mat[0][2] = 0.0f;  mat[0][3] = 0.0f;
    mat[1][0] = 0.0f;  mat[1][1] = c;     mat[1][2] = -s;    mat[1][3] = 0.0f;
    mat[2][0] = 0.0f;  mat[2][1] = s;     mat[2][2] = c;     mat[2][3] = 0.0f;
    mat[3][0] = 0.0f;  mat[3][1] = 0.0f;  mat[3][2] = 0.0f;  mat[3][3] = 1.0f;
}

void rotatey_mat4(mat4 mat, float angle){

    float c,s;
    c = cos(angle);
    s = sin(angle);

    mat[0][0] = c;     mat[0][1] = 0.0f;  mat[0][2] = s;     mat[0][3] = 0.0f;
    mat[1][0] = 0.0f;  mat[1][1] = 1.0f;  mat[1][2] = 0.0f;  mat[1][3] = 0.0f;
    mat[2][0] = -s;    mat[2][1] = 0.0f;  mat[2][2] = c;     mat[2][3] = 0.0f;
    mat[3][0] = 0.0f;  mat[3][1] = 0.0f;  mat[3][2] = 0.0f;  mat[3][3] = 1.0f;
}

void rotatez_mat4(mat4 mat, float angle){

    float c,s;
    c = cos(angle);
    s = sin(angle);

    mat[0][0] = c;     mat[0][1] = -s;    mat[0][2] = 0.0f;  mat[0][3] = 0.0f;
    mat[1][0] = s;     mat[1][1] = c;     mat[1][2] = 0.0f;  mat[1][3] = 0.0f;
    mat[2][0] = 0.0f;  mat[2][1] = 0.0f;  mat[2][2] = 1.0f;  mat[2][3] = 0.0f;
    mat[3][0] = 0.0f;  mat[3][1] = 0.0f;  mat[3][2] = 0.0f;  mat[3][3] = 1.0f;
}

void translate_mat4(mat4 mat, float x, float y, float z){
    mat[0][0] = 1.0f;  mat[0][1] = 0.0f;  mat[0][2] = 0.0f;  mat[0][3] = x;
    mat[1][0] = 0.0f;  mat[1][1] = 1.0f;  mat[1][2] = 0.0f;  mat[1][3] = y;
    mat[2][0] = 0.0f;  mat[2][1] = 0.0f;  mat[2][2] = 1.0f;  mat[2][3] = z;
    mat[3][0] = 0.0f;  mat[3][1] = 0.0f;  mat[3][2] = 0.0f;  mat[3][3] = 1.0f;
}

void scale_mat4(mat4 mat, float sx, float sy, float sz){
    mat[0][0] = sx  ;  mat[0][1] = 0.0f;  mat[0][2] = 0.0f;  mat[0][3] = 0.0f;
    mat[1][0] = 0.0f;  mat[1][1] = sy;    mat[1][2] = 0.0f;  mat[1][3] = 0.0f;
    mat[2][0] = 0.0f;  mat[2][1] = 0.0f;  mat[2][2] = sz;    mat[2][3] = 0.0f;
    mat[3][0] = 0.0f;  mat[3][1] = 0.0f;  mat[3][2] = 0.0f;  mat[3][3] = 1.0f;
}

void scale_dir_mat4(mat4 mat, float k, float x, float y, float z){

    float k_minus_1 = k - 1.0f;
    mat[0][0] = k_minus_1 * x * x + 1.0f;  mat[0][1] = k_minus_1 * x * y;         mat[0][2] = k_minus_1 * x * z;        mat[0][3] = 0.0f;
    mat[1][0] = k_minus_1 * x * y;         mat[1][1] = k_minus_1 * y * y + 1.0f;  mat[1][2] = k_minus_1 * y * z;        mat[1][3] = 0.0f;
    mat[2][0] = k_minus_1 * x * z;         mat[2][1] = k_minus_1 * y * z;         mat[2][2] = k_minus_1 * z *z + 1.0f;  mat[2][3] = 0.0f;
    mat[3][0] = 0.0f;                      mat[3][1] = 0.0f;                      mat[3][2] = 0.0f;                     mat[3][3] = 1.0f;

}

void frustum_mat4(mat4 mat, float l, float r, float b, float t, float n, float f){
    mat[0][0] = 2.0f*n/(r-l);  mat[0][1] = 0.0f;          mat[0][2] = (l+r)/(r-l);  mat[0][3] = 0.0f;
    mat[1][0] = 0.0f;          mat[1][1] = 2.0f*n/(t-b);  mat[1][2] = (t+b)/(t-b);  mat[1][3] = 0.0f;
    mat[2][0] = 0.0f;          mat[2][1] = 0.0f;          mat[2][2] = (f+n)/(f-n);  mat[2][3] = (2.0f*f*n)/(f-n);
    mat[3][0] = 0.0f;          mat[3][1] = 0.0f;          mat[3][2] = -1.0f;        mat[3][3] = 0.0f;
}

void perspective_mat4(mat4 mat, float angle, float aspect_ratio, float n, float f){

    float tan_mid = tan(angle/2.0f);
    float t = tan_mid * n;
    float b = -t;
    float r = aspect_ratio * t;
    float l = -r;
    frustum_mat4(mat, l, r, b, t, n, f);

}

void orthographic_mat4(mat4 mat, float l, float r, float b, float t, float n, float f){

    mat[0][0] = 2.0f/(r-l);  mat[0][1] = 0.0f;        mat[0][2] = 0.0f;        mat[0][3] = -(l+r)/(r-l);
    mat[1][0] = 0.0f;        mat[1][1] = 2.0f/(t-b);  mat[1][2] = 0.0f;        mat[1][3] = -(t+b)/(t-b);
    mat[2][0] = 0.0f;        mat[2][1] = 0.0f;        mat[2][2] = 2.0f/(f-n);  mat[2][3] = (f+n)/(f-n);
    mat[3][0] = 0.0f;        mat[3][1] = 0.0f;        mat[3][2] = 0.0f;        mat[3][3] = 1.0f;

}

void lookat_mat4(mat4 mat, vec3 eye_pos, vec3 at_pos, vec3 up_vector){

    vec3 camera_backward, camera_up, camera_right;
    /* Camera Backward Vector */
    camera_backward[0] = eye_pos[0] - at_pos[0];
    camera_backward[1] = eye_pos[1] - at_pos[1];
    camera_backward[2] = eye_pos[2] - at_pos[2];
    normalize_vec3(camera_backward);
    /* Camera Right vector */
    cross_product(up_vector, camera_backward, camera_right);
    normalize_vec3(camera_right);
    /* Camera Up vector */
    cross_product(camera_backward, camera_right, camera_up);
    normalize_vec3(camera_up);

    mat[0][0] = camera_right[0];     mat[0][1] = camera_right[1];     mat[0][2] = camera_right[2];     mat[0][3] = -dot_vec3(camera_right, eye_pos);
    mat[1][0] = camera_up[0];        mat[1][1] = camera_up[1];        mat[1][2] = camera_up[2];        mat[1][3] = -dot_vec3(camera_up, eye_pos);
    mat[2][0] = camera_backward[0];  mat[2][1] = camera_backward[1];  mat[2][2] = camera_backward[2];  mat[2][3] = -dot_vec3(camera_backward, eye_pos);
    mat[3][0] = 0.0f;                mat[3][1] = 0.0f;                mat[3][2] = 0.0f;                mat[3][3] = 1.0f;

}