#include "pipeline.h"

static er_MatrixModeEnum matrix_mode;

//Modelview Matrix Stack
unsigned int mv_stack_counter;
mat4 mv_stack[MODELVIEW_MATRIX_STACK_SIZE];
mat4 inv_mv_stack[MODELVIEW_MATRIX_STACK_SIZE];
mat3 nm_stack[MODELVIEW_MATRIX_STACK_SIZE];
er_Bool mv_flag[MODELVIEW_MATRIX_STACK_SIZE];
//Projection matrix stack
unsigned int proj_stack_counter;
mat4 proj_stack[PROJECTION_MATRIX_STACK_SIZE];
mat4 inv_proj_stack[PROJECTION_MATRIX_STACK_SIZE];
er_Bool proj_flag[PROJECTION_MATRIX_STACK_SIZE];
//Current Modelview Projection matrix
mat4 mv_proj_matrix;

er_StatusEnum er_get_matrix(mat4 matrix){

    if(matrix == NULL){
        return ER_NULL_POINTER;
    }
    if(matrix_mode == ER_MODELVIEW){
        assign_mat4(matrix, mv_stack[mv_stack_counter]);
    }else if(matrix_mode == ER_PROJECTION){
        assign_mat4(matrix, proj_stack[proj_stack_counter]);
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_push_matrix(){

    if(matrix_mode == ER_MODELVIEW){
        if(mv_stack_counter < MODELVIEW_MATRIX_STACK_SIZE - 1){
            assign_mat4(mv_stack[mv_stack_counter+1], mv_stack[mv_stack_counter]);
            mv_flag[mv_stack_counter+1] = ER_TRUE;
            mv_stack_counter++;
        }else{
            return ER_STACK_OVERFLOW;        
        }
    }else if(matrix_mode == ER_PROJECTION){
        if(proj_stack_counter < PROJECTION_MATRIX_STACK_SIZE - 1){
            assign_mat4(proj_stack[proj_stack_counter+1], proj_stack[proj_stack_counter]);
            proj_flag[proj_stack_counter+1] = ER_TRUE;
            proj_stack_counter++;
        }else{
            return ER_STACK_OVERFLOW;
        }
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_pop_matrix(){

    if(matrix_mode == ER_MODELVIEW){
        if(mv_stack_counter > 0){
            mv_flag[mv_stack_counter] = ER_TRUE;
            mv_stack_counter--;
        }else{
            return ER_STACK_UNDERFLOW;
        }
    }else if(matrix_mode == ER_PROJECTION){
        if(proj_stack_counter > 0){
            proj_flag[proj_stack_counter] = ER_TRUE;
            proj_stack_counter--;
        }else{
            return ER_STACK_UNDERFLOW;
        }
    }
    return ER_NO_ERROR;
}

void er_load_identity(){

    if(matrix_mode == ER_MODELVIEW){
        identity_mat4(mv_stack[mv_stack_counter]);
        mv_flag[mv_stack_counter] = ER_TRUE;
    }else if(matrix_mode == ER_PROJECTION){
        identity_mat4(proj_stack[proj_stack_counter]);
        proj_flag[proj_stack_counter] = ER_TRUE;
    }

}

er_StatusEnum er_matrix_mode(er_MatrixModeEnum mode){
    if(mode == ER_MODELVIEW || mode == ER_PROJECTION){
        matrix_mode = mode;
    }else{
        return ER_INVALID_ARGUMENT;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_multiply_matrix(mat4 matrix){

    if(matrix == NULL){
        return ER_NULL_POINTER;
    }
    if(matrix_mode == ER_MODELVIEW){
        mult_mat4_mat4(mv_stack[mv_stack_counter], matrix, mv_stack[mv_stack_counter]);
        mv_flag[mv_stack_counter] = ER_TRUE;
    }else if(matrix_mode == ER_PROJECTION){
        mult_mat4_mat4(proj_stack[proj_stack_counter], matrix, proj_stack[proj_stack_counter]);
        proj_flag[proj_stack_counter] = ER_TRUE;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_load_matrix_transpose(mat4 matrix){

    if(matrix == NULL){
        return ER_NULL_POINTER;
    }
    if(matrix_mode == ER_MODELVIEW){
        assignT_mat4( mv_stack[mv_stack_counter], matrix);
        mv_flag[mv_stack_counter] = ER_TRUE;
    }else if(matrix_mode == ER_PROJECTION){
        assignT_mat4( proj_stack[proj_stack_counter] , matrix);
        proj_flag[proj_stack_counter] = ER_TRUE;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_load_matrix(mat4 matrix){

    if(matrix == NULL){
        return ER_NULL_POINTER;
    }
    if(matrix_mode == ER_MODELVIEW){
        assign_mat4(mv_stack[mv_stack_counter], matrix);
        mv_flag[mv_stack_counter] = ER_TRUE;
    }else if(matrix_mode == ER_PROJECTION){
        assign_mat4(proj_stack[proj_stack_counter] , matrix);
        proj_flag[proj_stack_counter] = ER_TRUE;
    }
    return ER_NO_ERROR;
}

void er_rotate(float angle, float x, float y, float z){
    mat4 matrix_aux;
    rotate_mat4(matrix_aux, deg_to_rad(angle), x, y, z);
    er_multiply_matrix(matrix_aux);
}

void er_rotate_x(float angle){
    mat4 matrix_aux;
    rotatex_mat4(matrix_aux, deg_to_rad(angle));
    er_multiply_matrix(matrix_aux);
}

void er_rotate_y(float angle){
    mat4 matrix_aux;
    rotatey_mat4(matrix_aux, deg_to_rad(angle));
    er_multiply_matrix(matrix_aux);
}

void er_rotate_z(float angle){
    mat4 matrix_aux;
    rotatez_mat4(matrix_aux, deg_to_rad(angle));
    er_multiply_matrix(matrix_aux);
}

void er_translate(float x, float y, float z){
    mat4 matrix_aux;
    translate_mat4(matrix_aux, x, y, z);
    er_multiply_matrix(matrix_aux);
}

void er_scale(float x, float y, float z){
    mat4 matrix_aux;
    scale_mat4(matrix_aux, x, y, z);
    er_multiply_matrix(matrix_aux);
}

void er_scale_direction(float k, float x, float y, float z){
    mat4 matrix_aux;
    scale_dir_mat4(matrix_aux, k, x, y, z);
    er_multiply_matrix(matrix_aux);
}

void er_frustum( float l, float r, float b, float t, float n, float f){
    mat4 matrix_aux;
    frustum_mat4(matrix_aux, l, r, b, t, n, f);
    er_multiply_matrix(matrix_aux);
}

void er_perspective( float angle, float aspect_ratio, float n, float f){
    mat4 matrix_aux;
    perspective_mat4(matrix_aux, deg_to_rad(angle), aspect_ratio, n, f);
    er_multiply_matrix(matrix_aux);
}

void er_orthographic(float l, float r, float b, float t, float n, float f){
    mat4 matrix_aux;
    orthographic_mat4(matrix_aux, l, r, b, t, n, f);
    er_multiply_matrix(matrix_aux);
}

void er_look_at(float eye_x, float eye_y, float eye_z, float at_x, float at_y, float at_z, float up_x, float up_y, float up_z){
    mat4 matrix_aux;
    vec3 eye_pos;
    eye_pos[VAR_X] = eye_x;
    eye_pos[VAR_Y] = eye_y;
    eye_pos[VAR_Z] = eye_z;
    vec3 at_pos;
    at_pos[VAR_X] = at_x;
    at_pos[VAR_Y] = at_y;
    at_pos[VAR_Z] = at_z;
    vec3 up_vector;
    up_vector[VAR_X] = up_x;
    up_vector[VAR_Y] = up_y;
    up_vector[VAR_Z] = up_z;
    lookat_mat4(matrix_aux, eye_pos, at_pos, up_vector);
    er_multiply_matrix(matrix_aux);
}

void update_matrix_data(){

    if(mv_flag[mv_stack_counter] == ER_TRUE){
        /* Calcule matrix inverse */
        inverse_modeling_mat4(mv_stack[mv_stack_counter], inv_mv_stack[mv_stack_counter]);

        float (*inv_mv)[4] = inv_mv_stack[mv_stack_counter];
        float (*normal_mat)[3] = nm_stack[mv_stack_counter];
        /* Set normal matrix*/
        normal_mat[0][0] = inv_mv[0][0];  normal_mat[0][1] = inv_mv[1][0];  normal_mat[0][2] = inv_mv[2][0];
        normal_mat[1][0] = inv_mv[0][1];  normal_mat[1][1] = inv_mv[1][1];  normal_mat[1][2] = inv_mv[2][1];
        normal_mat[2][0] = inv_mv[0][2];  normal_mat[2][1] = inv_mv[1][2];  normal_mat[2][2] = inv_mv[2][2];

        mv_flag[mv_stack_counter] = ER_FALSE;
    }

    if(proj_flag[proj_stack_counter] == ER_TRUE){
        /* Calcule matrix inverse */
        inverse_mat4(proj_stack[proj_stack_counter], inv_proj_stack[proj_stack_counter]);
        proj_flag[proj_stack_counter] = ER_FALSE;
    }

    /* Premultiply projection and modelview matrix */
    multd_mat4_mat4(proj_stack[proj_stack_counter], mv_stack[mv_stack_counter], mv_proj_matrix);

}
