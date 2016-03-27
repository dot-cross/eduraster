#include "pipeline.h"

#define OUTSIDE_LEFT_PLANE 32
#define OUTSIDE_RIGHT_PLANE 16
#define OUTSIDE_BOTTOM_PLANE 8
#define OUTSIDE_TOP_PLANE 4
#define OUTSIDE_NEAR_PLANE 2
#define OUTSIDE_FAR_PLANE 1

#define IS_OUTSIDE_LEFT(vertex) ( vertex->position[VAR_X] < -vertex->position[VAR_W] )
#define IS_OUTSIDE_RIGHT(vertex) ( vertex->position[VAR_X] > vertex->position[VAR_W] )
#define IS_OUTSIDE_BOTTOM(vertex) ( vertex->position[VAR_Y] < -vertex->position[VAR_W] )
#define IS_OUTSIDE_TOP(vertex) ( vertex->position[VAR_Y] > vertex->position[VAR_W] )
#define IS_OUTSIDE_NEAR(vertex) ( vertex->position[VAR_Z] > vertex->position[VAR_W] )
#define IS_OUTSIDE_FAR(vertex) ( vertex->position[VAR_Z] < -vertex->position[VAR_W] )

#define CHECK_SIZE_AND_SWAP(input, output, aux, input_size, output_size) { \
if(output_size < 3){ \
    return PRIMITIVE_NO_VISIBLE; \
}else{ \
    aux = input; \
    input = output; \
    output = aux; \
    input_size = output_size; \
    output_size = 0; \
    } \
}

// NDC planes
static float ndc_left[]   = {-1.0f,  0.0f,  0.0f, -1.0f};
static float ndc_right[]  = { 1.0f,  0.0f,  0.0f, -1.0f};
static float ndc_bottom[] = { 0.0f, -1.0f,  0.0f, -1.0f};
static float ndc_top[]    = { 0.0f,  1.0f,  0.0f, -1.0f};
static float ndc_near[]   = { 0.0f,  0.0f,  1.0f, -1.0f};
static float ndc_far[]    = { 0.0f,  0.0f, -1.0f, -1.0f};

int calculate_outcode(struct vertex_output *vertex){

    int outcode = 0;

    if( IS_OUTSIDE_LEFT(vertex) ){
        outcode = outcode | OUTSIDE_LEFT_PLANE;
    }
    if( IS_OUTSIDE_RIGHT(vertex) ){
        outcode = outcode | OUTSIDE_RIGHT_PLANE;
    }
    if( IS_OUTSIDE_BOTTOM(vertex) ){
        outcode = outcode | OUTSIDE_BOTTOM_PLANE;
    }
    if( IS_OUTSIDE_TOP(vertex) ){
        outcode = outcode | OUTSIDE_TOP_PLANE;
    }
    if( IS_OUTSIDE_NEAR(vertex) ){
        outcode = outcode | OUTSIDE_NEAR_PLANE;
    }
    if( IS_OUTSIDE_FAR(vertex) ){
        outcode = outcode | OUTSIDE_FAR_PLANE;
    }
    return outcode;

}

static unsigned int add_new_vertex(struct vertex_output *vertex0, struct vertex_output *vertex1, float t){

    unsigned int new_index = output_buffer_size++;
    output_buffer[new_index].processed = ER_FALSE;
    struct vertex_output *new_vertex = &(output_buffer[new_index].vertex);

    new_vertex->position[VAR_X] = vertex0->position[VAR_X] + t * ( vertex1->position[VAR_X] - vertex0->position[VAR_X] );
    new_vertex->position[VAR_Y] = vertex0->position[VAR_Y] + t * ( vertex1->position[VAR_Y] - vertex0->position[VAR_Y] );
    new_vertex->position[VAR_Z] = vertex0->position[VAR_Z] + t * ( vertex1->position[VAR_Z] - vertex0->position[VAR_Z] );
    new_vertex->position[VAR_W] = vertex0->position[VAR_W] + t * ( vertex1->position[VAR_W] - vertex0->position[VAR_W] );
    new_vertex->point_size = vertex0->point_size + t * ( vertex1->point_size - vertex0->point_size );
    int k;
    for(k = 0; k < current_program->varying_attributes; k++){
        new_vertex->attributes[k] = vertex0->attributes[k] + t * ( vertex1->attributes[k] - vertex0->attributes[k] );
    }

    return new_index;

}

int clip_point(unsigned int input_index){

    if( !output_buffer[input_index].outcode){
        output_indices[output_indices_size++] = input_index;
        return PRIMITIVE_VISIBLE;
    }
    return PRIMITIVE_NO_VISIBLE;
}

static int clip_LB(float denom, float num, float *tE, float *tL){

    float t;

    if(denom > 0){ /*Potencially entering*/

        t = num / denom;

        if(t > *tL){
            return PRIMITIVE_NO_VISIBLE;
        }
        if(t > *tE){
            *tE = t;
        }

    }else if(denom < 0){ /*Potencially leaving*/

        t = num / denom;

        if(t < *tE){
            return PRIMITIVE_NO_VISIBLE;
        }

        if(t < *tL){
            *tL = t;
        }

    }else if(num > 0){ /* Parallel to edge*/
        return PRIMITIVE_NO_VISIBLE;
    }

    return PRIMITIVE_VISIBLE;
}

/* Liang-Barsky algorithm for clipping lines */
int clip_line(unsigned int vertex0_index, unsigned int vertex1_index){

    int outcode0, outcode1;
    outcode0 = output_buffer[vertex0_index].outcode;
    outcode1 = output_buffer[vertex1_index].outcode;

    if( !( outcode0 | outcode1)){
        output_indices[output_indices_size++] = vertex0_index;
        output_indices[output_indices_size++] = vertex1_index;
        return PRIMITIVE_TRIVIALLY_ACCEPTED;
    }else if(outcode0 & outcode1){
        return PRIMITIVE_NO_VISIBLE;
    }

    struct vertex_output* vertex0 = &(output_buffer[vertex0_index].vertex);
    struct vertex_output* vertex1 = &(output_buffer[vertex1_index].vertex);

    float delta_x, delta_y, delta_z, delta_w;
    delta_x = vertex1->position[VAR_X] - vertex0->position[VAR_X];
    delta_y = vertex1->position[VAR_Y] - vertex0->position[VAR_Y];
    delta_z = vertex1->position[VAR_Z] - vertex0->position[VAR_Z];
    delta_w = vertex1->position[VAR_W] - vertex0->position[VAR_W];

    float t_E = 0.0f;
    float t_L = 1.0f;
    unsigned int start_index = vertex0_index;
    unsigned int end_index = vertex1_index;

    if( clip_LB( delta_x + delta_w, - vertex0->position[VAR_X] - vertex0->position[VAR_W] , &t_E, &t_L) == PRIMITIVE_VISIBLE ){ /* Intersect with left plane*/
        if( clip_LB( - delta_x + delta_w, vertex0->position[VAR_X] - vertex0->position[VAR_W] , &t_E, &t_L) == PRIMITIVE_VISIBLE ){ /* Intersect with right plane*/
            if( clip_LB( delta_y + delta_w, - vertex0->position[VAR_Y] - vertex0->position[VAR_W] , &t_E, &t_L) == PRIMITIVE_VISIBLE ){ /* Intersect with bottom plane*/
                if( clip_LB( - delta_y + delta_w, vertex0->position[VAR_Y] - vertex0->position[VAR_W] , &t_E, &t_L) == PRIMITIVE_VISIBLE ){ /* Intersect with top plane*/
                    if( clip_LB( - delta_z + delta_w, vertex0->position[VAR_Z] - vertex0->position[VAR_W] , &t_E, &t_L) == PRIMITIVE_VISIBLE ){ /* Intersect with near plane*/
                        if( clip_LB( delta_z + delta_w, - vertex0->position[VAR_Z] - vertex0->position[VAR_W] , &t_E, &t_L) == PRIMITIVE_VISIBLE ){ /* Intersect with far plane*/

                            if(t_L < 1.0f){
                                end_index = add_new_vertex(vertex0, vertex1, t_L);
                            }
                            if(t_E > 0.0f){
                                start_index = add_new_vertex(vertex0, vertex1, t_E);
                            }
                            output_indices[output_indices_size++] = start_index;
                            output_indices[output_indices_size++] = end_index;

                            return PRIMITIVE_VISIBLE;

                        }
                    }
                }
            }
        }
    }  

    return PRIMITIVE_NO_VISIBLE;

}

/*
 * Clip a n-polygon against the given half space.
 */
static void clip_polygon(float *plane, unsigned int *input, unsigned int *output, unsigned int input_size, unsigned int *output_size){

    unsigned int i, polygon_size;
    float distance0, distance1, t;
    struct vertex_output *vertex0;
    struct vertex_output *vertex1;

    polygon_size = 0;
    vertex0 = &(output_buffer[ input[input_size - 1] ].vertex);
    distance0 = dot_vec4(plane, vertex0->position);

    for(i = 0; i < input_size; i++){

        vertex1 = &(output_buffer[ input[i] ].vertex);
        distance1 = dot_vec4(plane, vertex1->position);

        if(distance1 <= 0.0f){
            if(distance0 > 0.0f){
                /* Add a new interpolated vertex as start point */
                t = distance0 / (distance0 - distance1);
                output[polygon_size++] = add_new_vertex(vertex0, vertex1, t);
            }
            /* Add end point */
            output[polygon_size++] = input[i];
        }else if(distance0 <= 0.0f){
            /* Add a new interpolated vertex as end point */
            t = distance0 / (distance0 - distance1);
            output[polygon_size++] = add_new_vertex(vertex0, vertex1, t);
        }

        vertex0 = vertex1;
        distance0 = distance1;

    }

    *output_size = polygon_size;

}

/* 
 * Algorithm for clipping a triangle in homogeneous coordinates.
 * Based on Sutherland-Hodgman algorithm.
*/
int clip_triangle(unsigned int vertex0_index, unsigned int vertex1_index, unsigned int vertex2_index){

    int outcode0, outcode1, outcode2;
    outcode0 = output_buffer[vertex0_index].outcode;
    outcode1 = output_buffer[vertex1_index].outcode;
    outcode2 = output_buffer[vertex2_index].outcode;
    int triangle_mask = outcode0 | outcode1 | outcode2;

    if( !triangle_mask ){ /* Test if the three points are wholly inside */
        output_indices[output_indices_size++] = vertex0_index;
        output_indices[output_indices_size++] = vertex1_index;
        output_indices[output_indices_size++] = vertex2_index;
        return PRIMITIVE_TRIVIALLY_ACCEPTED;
    }else if(outcode0 & outcode1 & outcode2){ /* Test if the three points are on the same half space */
        return PRIMITIVE_NO_VISIBLE;
    }

    unsigned int input_size, output_size;
    unsigned int buffer0[9];
    unsigned int buffer1[9];
    unsigned int* input;
    unsigned int* output;
    unsigned int* aux;

    /* Init temporary buffers for indices */
    input = buffer0;
    output = buffer1;
    input[0] = vertex0_index;
    input[1] = vertex1_index;
    input[2] = vertex2_index;
    input_size = 3;
    output_size = 0;

    /* Clip against left plane */
    if(triangle_mask & OUTSIDE_LEFT_PLANE){
        clip_polygon(ndc_left, input, output, input_size, &output_size);
        CHECK_SIZE_AND_SWAP(input, output, aux, input_size, output_size);
    }

    /* Clip against right plane */
    if(triangle_mask & OUTSIDE_RIGHT_PLANE){
        clip_polygon(ndc_right, input, output, input_size, &output_size);
        CHECK_SIZE_AND_SWAP(input, output, aux, input_size, output_size);
    }

    /* Clip against bottom plane */
    if(triangle_mask & OUTSIDE_BOTTOM_PLANE){
        clip_polygon(ndc_bottom, input, output, input_size, &output_size);
        CHECK_SIZE_AND_SWAP(input, output, aux, input_size, output_size);
    }

    /* Clip against top plane */
    if(triangle_mask & OUTSIDE_TOP_PLANE){
        clip_polygon(ndc_top, input, output, input_size, &output_size);
        CHECK_SIZE_AND_SWAP(input, output, aux, input_size, output_size);
    }

    /* Clip against near plane */
    if(triangle_mask & OUTSIDE_NEAR_PLANE){
        clip_polygon(ndc_near, input, output, input_size, &output_size);
        CHECK_SIZE_AND_SWAP(input, output, aux, input_size, output_size);
    }

    /* Clip against far plane */
    if(triangle_mask & OUTSIDE_FAR_PLANE){
        clip_polygon(ndc_far, input, output, input_size, &output_size);
        CHECK_SIZE_AND_SWAP(input, output, aux, input_size, output_size);
    }

    /* Triangulate the n-polygon, and writes indices of visible triangles */
    unsigned int i = 0;
    for(i = 1; i < input_size - 1; i++){
        output_indices[output_indices_size++] = input[0];
        output_indices[output_indices_size++] = input[i];
        output_indices[output_indices_size++] = input[i+1];
    }

    return PRIMITIVE_VISIBLE;

}
