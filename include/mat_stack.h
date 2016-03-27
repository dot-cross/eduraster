#ifndef __MATRIX_STACK__
#define __MATRIX_STACK__

#define MODELVIEW_MATRIX_STACK_SIZE 24
#define PROJECTION_MATRIX_STACK_SIZE 4

extern int mv_stack_counter;
extern mat4 mv_stack[MODELVIEW_MATRIX_STACK_SIZE];
extern mat4 inv_mv_stack[MODELVIEW_MATRIX_STACK_SIZE];
extern mat3 nm_stack[MODELVIEW_MATRIX_STACK_SIZE];
extern int mv_flag[MODELVIEW_MATRIX_STACK_SIZE];
extern int proj_stack_counter;
extern mat4 proj_stack[PROJECTION_MATRIX_STACK_SIZE];
extern mat4 inv_proj_stack[PROJECTION_MATRIX_STACK_SIZE];
extern int proj_flag[PROJECTION_MATRIX_STACK_SIZE];
extern mat4 mv_proj_matrix;

void update_matrix_data();

#endif
