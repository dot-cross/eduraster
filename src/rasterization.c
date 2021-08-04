#include "pipeline.h"

typedef struct Edge{
    float x, z, w;
    float attributes[ATTRIBUTES_SIZE];
    float attributes_step[ATTRIBUTES_SIZE];
    float step_x, step_z, step_w;
    int start_y, end_y;
} Edge;

void draw_point_sprite(er_VertexOutput *vertex, er_PolygonFaceEnum face){

    float half_size = 0.5f * vertex->point_size;
    int start_x, end_x;
    int start_y, end_y;
    int i,j;

    start_x = (int)ceil( vertex->position[VAR_X] - half_size );
    if(start_x < (int)window_origin_x){
        start_x = window_origin_x;
    }
    end_x = (int)ceil( vertex->position[VAR_X] + half_size ) - 1;
    if(end_x >= (int)window_width){
        end_x = window_width - 1;
    }
    start_y = (int)ceil( vertex->position[VAR_Y] - half_size );
    if(start_y < (int)window_origin_y){
        start_y = window_origin_y;
    }
    end_y = (int)ceil( vertex->position[VAR_Y] + half_size ) - 1;
    if(end_y >= (int)window_height){
        end_y = window_height - 1;
    }

    er_FragInput input;
    input.frag_coord[VAR_Z] = vertex->position[VAR_Z];
    input.frag_coord[VAR_W] = vertex->position[VAR_W];
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    int k;
    for(k = 0; k < current_program->varying_attributes; k++){
        input.attributes[k] = vertex->attributes[k];
        input.ddx[k] = 0.0f;
        input.ddy[k] = 0.0f;
    }
    input.dz_dx = 0.0f;
    input.dz_dy = 0.0f;
    input.dw_dx = 0.0f;
    input.dw_dy = 0.0f;
    input.point_size = vertex->point_size;
    float one_over_size = 1.0f / vertex->point_size;

    if(point_sprite_coord_origin == ER_POINT_SPRITE_LOWER_LEFT){

        for(i = start_y; i <= end_y; i++){
            input.frag_coord[VAR_Y] = i;
            input.point_coord[VAR_Y] = 0.5f + ( i - vertex->position[VAR_Y]) * one_over_size;
            for(j = start_x; j <= end_x; j++){
                input.frag_coord[VAR_X] = j;
                input.point_coord[VAR_X] = 0.5f + ( j - vertex->position[VAR_X]) *one_over_size;
                current_program->fragment_shader(i, j, &input, &global_variables);
            }
        }

    }else{

        for(i = start_y; i <= end_y; i++){
            input.frag_coord[VAR_Y] = i;
            input.point_coord[VAR_Y] = 0.5f - ( i - vertex->position[VAR_Y]) * one_over_size;
            for(j = start_x; j <= end_x; j++){
                input.frag_coord[VAR_X] = j;
                input.point_coord[VAR_X] = 0.5f + ( j - vertex->position[VAR_X]) * one_over_size;
                current_program->fragment_shader(i, j, &input, &global_variables);
            }
        }

    }

}

void draw_point(er_VertexOutput *vertex, er_PolygonFaceEnum face){

    float half_size = vertex->point_size / 2.0f;
    int start_x, end_x;
    int start_y, end_y;
    int i,j;

    start_x = (int)ceil( vertex->position[VAR_X] - half_size );
    if(start_x < (int)window_origin_x){
        start_x = window_origin_x;
    }
    end_x = (int)ceil( vertex->position[VAR_X] + half_size ) - 1;
    if(end_x >= (int)window_width){
        end_x = window_width - 1;
    }
    start_y = (int)ceil( vertex->position[VAR_Y] - half_size );
    if(start_y < (int)window_origin_y){
        start_y = window_origin_y;
    }
    end_y = (int)ceil( vertex->position[VAR_Y] + half_size ) - 1;
    if(end_y >= (int)window_height){
        end_y = window_height - 1;
    }

    er_FragInput input;
    input.frag_coord[VAR_Z] = vertex->position[VAR_Z];
    input.frag_coord[VAR_W] = vertex->position[VAR_W];
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    int k;
    for(k = 0; k < current_program->varying_attributes; k++){
        input.attributes[k] = vertex->attributes[k];
        input.ddx[k] = 0.0f;
        input.ddy[k] = 0.0f;
    }
    input.dz_dx = 0.0f;
    input.dz_dy = 0.0f;
    input.dw_dx = 0.0f;
    input.dw_dy = 0.0f;

    for(i = start_y; i <= end_y;i++){
        input.frag_coord[VAR_Y] = i;
        for(j = start_x; j <= end_x;j++){
            input.frag_coord[VAR_X] = j;
            current_program->fragment_shader(i, j, &input, &global_variables);
        }
    }

}

static void draw_vertical_negative(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_S, t;
    int k;
    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy;
    int x0, y0, y1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    y1 = uiround( vertex1->position[VAR_Y] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_S = -p1p0[VAR_Y] / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    int i, j = x0;
    for(i = y0 - 1; i > y1; i--){
        t += t_S;
        input.frag_coord[VAR_X] = j;
        input.frag_coord[VAR_Y] = i;
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        current_program->fragment_shader(i, j, &input, &global_variables);
    }

}

static void draw_vertical_positive(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_N, t;
    int k;
    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy;
    int x0, y0, y1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    y1 = uiround( vertex1->position[VAR_Y] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_N = p1p0[VAR_Y] / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    int i, j = x0;
    for(i = y0 + 1; i < y1; i++){
        t += t_N;
        input.frag_coord[VAR_X] = j;
        input.frag_coord[VAR_Y] = i;
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        current_program->fragment_shader(i, j, &input, &global_variables);
    }

}

static void draw_horizontal_negative(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_W, t;
    int k;
    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy;
    int x0, y0, x1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    x1 = uiround( vertex1->position[VAR_X] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_W = -p1p0[VAR_X] / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    int i, j = y0;
    for(i = x0 - 1; i > x1; i--){
        t += t_W;
        input.frag_coord[VAR_X] = i;
        input.frag_coord[VAR_Y] = j;
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        current_program->fragment_shader(j, i, &input, &global_variables);
    }

}

static void draw_horizontal_positive(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_E, t;
    int k;
    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy;
    int x0, y0, x1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    x1 = uiround( vertex1->position[VAR_X] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_E = p1p0[VAR_X] / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    int i, j = y0;
    for(i = x0 + 1; i < x1; i++){
        t += t_E;
        input.frag_coord[VAR_X] = i;
        input.frag_coord[VAR_Y] = j;
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        current_program->fragment_shader(j, i, &input, &global_variables);
    }

}

/*
 * Midpoint line algorithm, Octant 6
 */
static void draw_line_case6(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_S, t_SW, t;
    int k;

    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy, residue;
    int x0, y0, y1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    y1 = uiround( vertex1->position[VAR_Y] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    residue = 2.0f*(dy * ( x0 - vertex0->position[VAR_X]) - dx * (y0 - vertex0->position[VAR_Y]));
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_S = -p1p0[VAR_Y] / square_length;
    t_SW = (-p1p0[VAR_X] - p1p0[VAR_Y]) / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    float mid = residue - dy + 2 * dx;
    float increment_S = 2 * dx;
    float increment_SW = 2 * (-dy + dx);
    int i,j = x0;

    for(i = y0 - 1; i > y1; i--){
        if(mid >= 0){
            mid += increment_S;
            t += t_S;
        }else{
            mid += increment_SW;
            t += t_SW;
            j--;
        }
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        input.frag_coord[VAR_X] = j;
        input.frag_coord[VAR_Y] = i;
        current_program->fragment_shader(i, j, &input, &global_variables);
    }

}

/*
 * Midpoint line algorithm, Octant 7
 */
static void draw_line_case7(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_S, t_SE, t;
    int k;

    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy, residue;
    int x0, y0, y1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    y1 = uiround( vertex1->position[VAR_Y] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    residue = 2.0f*(dy * ( x0 - vertex0->position[VAR_X]) - dx * (y0 - vertex0->position[VAR_Y]));
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_S = -p1p0[VAR_Y] / square_length;
    t_SE = (p1p0[VAR_X] - p1p0[VAR_Y]) / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    float mid = residue + dy + 2 * dx;
    float increment_S = 2 * dx;
    float increment_SE = 2 * (dy + dx);
    int i,j = x0;

    for(i = y0 - 1; i > y1; i--){
        if(mid >= 0){
            mid += increment_SE;
            t += t_SE;
            j++;
        }else{
            mid += increment_S;
            t += t_S;
        }
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        input.frag_coord[VAR_X] = j;
        input.frag_coord[VAR_Y] = i;
        current_program->fragment_shader(i, j, &input, &global_variables);
    }

}

/*
 * Midpoint line algorithm, Octant 8
 */
static void draw_line_case8(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_E, t_SE, t;
    int k;

    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy, residue;
    int x0, y0, x1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    x1 = uiround( vertex1->position[VAR_X] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    residue = 2.0f*(dy * ( x0 - vertex0->position[VAR_X]) - dx * (y0 - vertex0->position[VAR_Y]));
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_E = p1p0[VAR_X] / square_length;
    t_SE = (p1p0[VAR_X] - p1p0[VAR_Y]) / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    float mid = residue + 2 * dy + dx;
    float increment_E = 2 * dy;
    float increment_SE = 2 * (dy + dx);
    int i,j = y0;

    for(i = x0 + 1; i < x1; i++){
        if(mid > 0){
            mid += increment_E;
            t += t_E;
        }else{
            j--;
            mid += increment_SE;
            t += t_SE;
        }
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        input.frag_coord[VAR_X] = i;
        input.frag_coord[VAR_Y] = j;
        current_program->fragment_shader(j, i, &input, &global_variables);
    }

}

/*
 * Midpoint line algorithm, Octant 3
 */
static void draw_line_case3(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_N, t_NW, t;
    int k;

    /* Fragment settings */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy, residue;
    int x0, y0, y1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    y1 = uiround( vertex1->position[VAR_Y] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    residue = 2.0f*(dy * ( x0 - vertex0->position[VAR_X]) - dx * (y0 - vertex0->position[VAR_Y]));
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_N = p1p0[VAR_Y] / square_length;
    t_NW = (-p1p0[VAR_X] + p1p0[VAR_Y]) / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    float mid = residue -  dy - 2*dx;
    float increment_N = -2 * dx;
    float increment_NW = -2 * (dy + dx);
    int i,j = x0;

    for(i = y0+1; i < y1; i++){
        if(mid > 0){
            j--;
            mid += increment_NW;
            t += t_NW;
        }else{
            mid += increment_N;
            t += t_N;
        }
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        input.frag_coord[VAR_X] = j;
        input.frag_coord[VAR_Y] = i;
        current_program->fragment_shader(i, j, &input, &global_variables);
    }

}

/*
 * Midpoint line algorithm, Octant 2
 */
static void draw_line_case2(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_N, t_NE, t;
    int k;

    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy, residue;
    int x0, y0, y1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    y1 = uiround( vertex1->position[VAR_Y] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    residue = 2.0f*(dy * ( x0 - vertex0->position[VAR_X]) - dx * (y0 - vertex0->position[VAR_Y]));
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_N = p1p0[VAR_Y] / square_length;
    t_NE = (p1p0[VAR_X] + p1p0[VAR_Y]) / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    float mid = residue + dy - 2 * dx;
    float increment_N = -2 * dx;
    float increment_NE = 2 * (dy - dx);
    int i,j = x0;

    for(i = y0+1; i < y1; i++){
        if(mid > 0){
            mid += increment_N;
            t += t_N;
        }else{
            j++;
            mid += increment_NE;
            t += t_NE;
        }
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        input.frag_coord[VAR_X] = j;
        input.frag_coord[VAR_Y] = i;
        current_program->fragment_shader(i, j,&input, &global_variables);
    }

}

/*
 * Midpoint line algorithm, Octact 5
 */
static void draw_line_case5(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_W, t_SW, t;
    int k;

    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy, residue;
    int x0, y0, x1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    x1 = uiround( vertex1->position[VAR_X] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    residue = 2.0f*(dy * ( x0 - vertex0->position[VAR_X]) - dx * (y0 - vertex0->position[VAR_Y]));
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_W = -p1p0[VAR_X] / square_length;
    t_SW = (-p1p0[VAR_X] - p1p0[VAR_Y]) / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    float mid = residue - 2 * dy + dx;
    float increment_W = -2 * dy;
    float increment_SW = 2 * (-dy + dx);
    int i, j = y0;

    for(i = x0 - 1; i > x1; i--){
        if(mid >= 0){
            j--;
            mid += increment_SW;
            t += t_SW;
        }else{
            mid += increment_W;
            t += t_W;
        }
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        input.frag_coord[VAR_X] = i;
        input.frag_coord[VAR_Y] = j;
        current_program->fragment_shader(j, i, &input, &global_variables);
    }

}

/*
 * Midpoint line algorithm, Octact 4
 */
static void draw_line_case4(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_W, t_NW, t;
    int k;

    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy, residue;
    int x0, y0, x1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    x1 = uiround( vertex1->position[VAR_X] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    residue = 2.0f*(dy * ( x0 - vertex0->position[VAR_X]) - dx * (y0 - vertex0->position[VAR_Y]));
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_W = -p1p0[VAR_X] / square_length;
    t_NW = (-p1p0[VAR_X] + p1p0[VAR_Y]) / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    float mid = residue - 2 * dy - dx;
    float increment_W = -2 * dy;
    float increment_NW = -2 * (dy + dx);
    int i, j = y0;

    for(i = x0-1; i > x1; i--){
        if(mid >= 0){
            mid += increment_W;
            t += t_W;
        }else{
            j++;
            mid += increment_NW;
            t += t_NW;
        }
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        input.frag_coord[VAR_X] = i;
        input.frag_coord[VAR_Y] = j;
        current_program->fragment_shader(j, i, &input, &global_variables);
    }

}

/*
 * Midpoint line algorithm, Octant 1
 */
static void draw_line_case1(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    er_FragInput input;
    float delta_z, delta_w;
    float delta[ATTRIBUTES_SIZE];
    float t_E, t_NE, t;
    int k;
    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;
    /* Calculate endpoints, deltas and residue */
    float dx, dy, residue;
    int x0, y0, x1;
    x0 = uiround( vertex0->position[VAR_X] );
    y0 = uiround( vertex0->position[VAR_Y] );
    x1 = uiround( vertex1->position[VAR_X] );
    dx = vertex1->position[VAR_X] - vertex0->position[VAR_X]; 
    dy = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    residue = 2.0f*(dy * ( x0 - vertex0->position[VAR_X]) - dx * (y0 - vertex0->position[VAR_Y]));
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    for(k = 0; k < current_program->varying_attributes; k++){
        delta[k] = vertex1->attributes[k] - vertex0->attributes[k];
    }
    /* Calculate interpolation parameters */
    vec2 p1p0, prp0;
    p1p0[VAR_X] = dx;
    p1p0[VAR_Y] = dy;
    prp0[VAR_X] = x0 - vertex0->position[VAR_X];
    prp0[VAR_Y] = y0 - vertex0->position[VAR_Y];
    float square_length = dot_vec2(p1p0, p1p0);
    t = dot_vec2(prp0, p1p0) / square_length;
    t_E = p1p0[VAR_X] / square_length;
    t_NE = (p1p0[VAR_X] + p1p0[VAR_Y]) / square_length;
    if(t > 0.0f){
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
    }else{
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z];
        input.frag_coord[VAR_W] = vertex0->position[VAR_W];
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k];
        }
    }
    /* Write first point */
    input.frag_coord[VAR_X] = x0;
    input.frag_coord[VAR_Y] = y0;
    current_program->fragment_shader(y0, x0, &input, &global_variables);

    float mid = residue + 2 * dy - dx;
    float increment_E = 2 * dy;
    float increment_NE = 2 * (dy - dx);
    int i, j = y0;
    for(i = x0 + 1; i < x1; i++){
        if(mid > 0){
            j++;
            mid += increment_NE;
            t += t_NE;
        }else{
            mid += increment_E;
            t += t_E;
        }
        input.frag_coord[VAR_Z] = vertex0->position[VAR_Z] + t * delta_z;
        input.frag_coord[VAR_W] = vertex0->position[VAR_W] + t * delta_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = vertex0->attributes[k] + t * delta[k];
        }
        input.frag_coord[VAR_X] = i;
        input.frag_coord[VAR_Y] = j;
        current_program->fragment_shader(j, i, &input, &global_variables);
    }

}

/*
 * Rasterization of lines.
*/
void draw_line(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face){

    int x0, y0, x1, y1;
    x0 = uiround(vertex0->position[VAR_X]);
    y0 = uiround(vertex0->position[VAR_Y]);
    x1 = uiround(vertex1->position[VAR_X]);
    y1 = uiround(vertex1->position[VAR_Y]);
    float dx = fabs(vertex1->position[VAR_X] - vertex0->position[VAR_X]);
    float dy = fabs(vertex1->position[VAR_Y] - vertex0->position[VAR_Y]);

    if(x0 < x1){
        if(y0 < y1){ /* First Cuadrant */
            if(dy > dx){
                draw_line_case2(vertex0, vertex1, face);
            }else{
                draw_line_case1(vertex0, vertex1, face);
            }
        }else if(y0 > y1){ /* Fourth Cuadrant */
            if(dy > dx){
                draw_line_case7(vertex0, vertex1, face);
            }else{
                draw_line_case8(vertex0, vertex1, face);
            }
        }else{
            draw_horizontal_positive(vertex0, vertex1, face);
        }
    }else if(x0 > x1){
        if(y0 < y1){ /* Second Cuadrant */
            if(dy > dx){
                draw_line_case3(vertex0, vertex1, face);
            }else{
                draw_line_case4(vertex0, vertex1, face);
            }
        }else if(y0 > y1){ /* Third Cuadrant */
            if(dy > dx){
                draw_line_case6(vertex0, vertex1, face);
            }else{
                draw_line_case5(vertex0, vertex1, face);
            }
        }else{
            draw_horizontal_negative(vertex0, vertex1, face);
        }
    }else if(y0 < y1){
        draw_vertical_positive(vertex0, vertex1, face);
    }else if(y0 > y1){
        draw_vertical_negative(vertex0, vertex1, face);
    }

}

static void init_left_edge(Edge *t_edge, er_VertexOutput *bottom, er_VertexOutput *top){

    int start_y = ceil(bottom->position[VAR_Y]);
    int end_y = (int)ceil(top->position[VAR_Y]) - 1;
    float prestep_y = start_y - bottom->position[VAR_Y];
    float y_range = top->position[VAR_Y] - bottom->position[VAR_Y];
    float x_range = top->position[VAR_X] - bottom->position[VAR_X];
    float step_x = x_range / y_range;
    t_edge->start_y = start_y;
    t_edge->end_y = end_y;
    t_edge->step_x = step_x;
    t_edge->x = bottom->position[VAR_X] + step_x * prestep_y;

    t_edge->step_z = (top->position[VAR_Z] - bottom->position[VAR_Z]) / y_range;
    t_edge->z = bottom->position[VAR_Z] + t_edge->step_z * prestep_y;

    t_edge->step_w = (top->position[VAR_W] - bottom->position[VAR_W]) / y_range;
    t_edge->w = bottom->position[VAR_W] + t_edge->step_w * prestep_y;

    int k;
    for(k = 0; k < current_program->varying_attributes; k++){
        t_edge->attributes_step[k] = (top->attributes[k] - bottom->attributes[k]) / y_range;
        t_edge->attributes[k] = bottom->attributes[k] + t_edge->attributes_step[k] * prestep_y;
    }

}

static void init_right_edge(Edge *t_edge, er_VertexOutput *bottom, er_VertexOutput *top){

    int start_y = ceil(bottom->position[VAR_Y]);
    int end_y = (int)ceil(top->position[VAR_Y]) - 1;
    float prestep_y = start_y - bottom->position[VAR_Y];
    float y_range = top->position[VAR_Y] - bottom->position[VAR_Y];
    float x_range = top->position[VAR_X] - bottom->position[VAR_X];
    float step_x = x_range / y_range;
    t_edge->start_y = start_y;
    t_edge->end_y = end_y;
    t_edge->step_x = step_x;
    t_edge->x = bottom->position[VAR_X] + step_x * prestep_y;

}

/*
 * Scan line conversion of a triangle given on CCW order. Generic interpolation of parameters.
 * Sampling on pixel centers, with subpixel precision and consistent bottom-left fill convention.
*/
void draw_triangle(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_VertexOutput *vertex2, er_PolygonFaceEnum face){

    Edge bottom_to_top, bottom_to_middle, middle_to_top;
    Edge *left0, *right0;
    Edge *left1, *right1;
    er_FragInput input;

    /* Calculate gradients */
    float dx10, dx20, dy10, dy20, dattrib10, dattrib20, a, b, one_over_c;
    dx10 = vertex1->position[VAR_X] - vertex0->position[VAR_X];
    dx20 = vertex2->position[VAR_X] - vertex0->position[VAR_X];
    dy10 = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    dy20 = vertex2->position[VAR_Y] - vertex0->position[VAR_Y];
    one_over_c = 1.0f / (dx10 * dy20 - dx20 * dy10);
    /* Z Gradients */
    dattrib10 = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    dattrib20 = vertex2->position[VAR_Z] - vertex0->position[VAR_Z];
    a = dy10 * dattrib20 - dy20 * dattrib10;
    b = dx20 * dattrib10 - dx10 * dattrib20;
    input.dz_dx = -a * one_over_c;
    input.dz_dy = -b * one_over_c;
    /* 1/W Gradients */
    dattrib10 = vertex1->position[VAR_W] - vertex0->position[VAR_W];
    dattrib20 = vertex2->position[VAR_W] - vertex0->position[VAR_W];
    a = dy10 * dattrib20 - dy20 * dattrib10;
    b = dx20 * dattrib10 - dx10 * dattrib20;
    input.dw_dx = -a * one_over_c;
    input.dw_dy = -b * one_over_c;
    /* Gradients of varying attributes */
    int k;
    for(k = 0; k < current_program->varying_attributes; k++){
        dattrib10 = vertex1->attributes[k] - vertex0->attributes[k];
        dattrib20 = vertex2->attributes[k] - vertex0->attributes[k];
        a = dy10 * dattrib20 - dy20 * dattrib10;
        b = dx20 * dattrib10 - dx10 * dattrib20;
        input.ddx[k] = -a * one_over_c;
        input.ddy[k] = -b * one_over_c;
    }
    /* Front facing flag */
    input.front_facing = (face == ER_FRONT) ? ER_TRUE: ER_FALSE;

    /* Edges setup */
    float y0, y1, y2;
    y0 = vertex0->position[VAR_Y];
    y1 = vertex1->position[VAR_Y];
    y2 = vertex2->position[VAR_Y];

    if(y0 < y1){
        if(y1 < y2){
            init_left_edge(&bottom_to_top, vertex0, vertex2);
            init_right_edge(&bottom_to_middle, vertex0, vertex1);
            init_right_edge(&middle_to_top, vertex1, vertex2);
            left0 = &bottom_to_top; right0 = &bottom_to_middle;
            left1 = &bottom_to_top; right1 = &middle_to_top;
        }else{
            if(y0 < y2){
                init_left_edge(&bottom_to_middle, vertex0, vertex2);
                init_left_edge(&middle_to_top, vertex2, vertex1);
                init_right_edge(&bottom_to_top, vertex0, vertex1);
                left0 = &bottom_to_middle; right0 = &bottom_to_top;
                left1 = &middle_to_top; right1 = &bottom_to_top;
            }else{
                init_left_edge(&bottom_to_top, vertex2, vertex1);
                init_right_edge(&bottom_to_middle, vertex2, vertex0);
                init_right_edge(&middle_to_top, vertex0, vertex1);
                left0 = &bottom_to_top; right0 = &bottom_to_middle;
                left1 = &bottom_to_top; right1 = &middle_to_top;
            }
        }
    }else{
        if(y0 < y2){
            init_left_edge(&bottom_to_middle, vertex1, vertex0);
            init_left_edge(&middle_to_top, vertex0, vertex2);
            init_right_edge(&bottom_to_top, vertex1, vertex2);
            left0 = &bottom_to_middle; right0 = &bottom_to_top;
            left1 = &middle_to_top; right1 = &bottom_to_top;
        }else{
            if(y1 < y2){
                init_left_edge(&bottom_to_top, vertex1, vertex0);
                init_right_edge(&middle_to_top, vertex2, vertex0);
                init_right_edge(&bottom_to_middle, vertex1, vertex2);
                left0 = &bottom_to_top; right0 = &bottom_to_middle;
                left1 = &bottom_to_top; right1 = &middle_to_top;
            }else{
                init_left_edge(&bottom_to_middle, vertex2, vertex1);
                init_left_edge(&middle_to_top, vertex1, vertex0);
                init_right_edge(&bottom_to_top, vertex2, vertex0);
                left0 = &bottom_to_middle; right0 = &bottom_to_top;
                left1 = &middle_to_top; right1 = &bottom_to_top;
            }
        }
    }

    int y, x, start_x, end_x;
    float prestep_x;
    for(y = bottom_to_middle.start_y; y <= bottom_to_middle.end_y; y++){
        start_x = ceil(left0->x);
        end_x = (int)ceil(right0->x) - 1;
        prestep_x = start_x - left0->x;
        /* Prestep interpolators for scanline */
        input.frag_coord[VAR_Z] = left0->z + input.dz_dx * prestep_x;
        input.frag_coord[VAR_W] = left0->w + input.dw_dx * prestep_x;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = left0->attributes[k] + input.ddx[k] * prestep_x;
        }
        /* Scan line interpolation*/
        for(x = start_x; x <= end_x; x++){
            input.frag_coord[VAR_X] = x;
            input.frag_coord[VAR_Y] = y;
            current_program->fragment_shader(y, x, &input, &global_variables);
            input.frag_coord[VAR_Z] += input.dz_dx;
            input.frag_coord[VAR_W] += input.dw_dx;
            for(k = 0; k < current_program->varying_attributes; k++){
                input.attributes[k] += input.ddx[k];
            }
        }
        /* Step along left edge */
        left0->x += left0->step_x;
        left0->z += left0->step_z;
        left0->w += left0->step_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            left0->attributes[k] += left0->attributes_step[k];
        }
        /* Step along right edge */
        right0->x += right0->step_x;
    }

    for(y = middle_to_top.start_y; y <= middle_to_top.end_y; y++){
        start_x = ceil(left1->x);
        end_x = (int)ceil(right1->x) - 1;
        prestep_x = start_x - left1->x;
        /* Prestep interpolators for scanline */
        input.frag_coord[VAR_Z] = left1->z + input.dz_dx * prestep_x;
        input.frag_coord[VAR_W] = left1->w + input.dw_dx * prestep_x;
        for(k = 0; k < current_program->varying_attributes; k++){
            input.attributes[k] = left1->attributes[k] + input.ddx[k] * prestep_x;
        }
        /* Scan line interpolation*/
        for(x = start_x; x <= end_x; x++){
            input.frag_coord[VAR_X] = x;
            input.frag_coord[VAR_Y] = y;
            current_program->fragment_shader(y, x, &input, &global_variables);
            input.frag_coord[VAR_Z] += input.dz_dx;
            input.frag_coord[VAR_W] += input.dw_dx;
            for(k = 0; k < current_program->varying_attributes; k++){
                input.attributes[k] += input.ddx[k];
            }
        }
        /* Step along left edge */
        left1->x += left1->step_x;
        left1->z += left1->step_z;
        left1->w += left1->step_w;
        for(k = 0; k < current_program->varying_attributes; k++){
            left1->attributes[k] += left1->attributes_step[k];
        }
        /* Step along right edge */
        right1->x += right1->step_x;
    }

}
