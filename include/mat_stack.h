#ifndef __MATRIX_STACK__
#define __MATRIX_STACK__

#define MODELVIEW_MATRIX_STACK_SIZE 24
#define PROJECTION_MATRIX_STACK_SIZE 4

extern er_MatrixModeEnum mv_stack_counter;
extern mat4 mv_stack[MODELVIEW_MATRIX_STACK_SIZE];
extern mat4 inv_mv_stack[MODELVIEW_MATRIX_STACK_SIZE];
extern mat3 nm_stack[MODELVIEW_MATRIX_STACK_SIZE];
extern er_Bool mv_flag[MODELVIEW_MATRIX_STACK_SIZE];
extern unsigned int proj_stack_counter;
extern mat4 proj_stack[PROJECTION_MATRIX_STACK_SIZE];
extern mat4 inv_proj_stack[PROJECTION_MATRIX_STACK_SIZE];
extern er_Bool proj_flag[PROJECTION_MATRIX_STACK_SIZE];
extern mat4 mv_proj_matrix;

void update_matrix_data();

#endif
