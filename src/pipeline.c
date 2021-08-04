#include "pipeline.h"

#define VERTEX_CACHE_SIZE 32

typedef struct VertexCacheRegister{
    int input_index;
    int output_index;
} VertexCacheRegister;

//Vertex Cache
static VertexCacheRegister vertex_cache[VERTEX_CACHE_SIZE];

//Input Buffer
er_VertexInput *input_buffer = NULL;
unsigned int input_buffer_size;

//Indices buffer
unsigned int *input_indices = NULL;
unsigned int input_indices_size;

//Output Buffer
OutputBufferRegister *output_buffer = NULL;
unsigned int output_buffer_size;

//Indices buffer after clipping operation
unsigned int *output_indices = NULL;
unsigned int output_indices_size;

//Current vertex state
vec3 current_normal;
vec4 current_tex_coord;
vec4 current_color;
float current_fog_coord;

//Begin/End calls
static unsigned int batch_size;
static void (*be_process_func)();

//Viewport data
unsigned int window_origin_x, window_origin_y, window_width, window_height;

/* Variables for select front or back face  */
er_PolygonOrientationEnum front_face_orientation;
er_PolygonModeEnum front_face_mode;
er_PolygonModeEnum back_face_mode;
er_Bool cull_face_enable;
er_PolygonFaceEnum cull_face;

/* Uniform vars */
er_UniVars global_variables;

/* Error messages */
const char* status_strings[] = {
    "No error",
    "Invalid Argument",
    "No vertex array has been set",
    "Out of memory",
    "Invalid operation on current state",
    "Matrix Stack Overflow",
    "Matrix Stack Underflow",
    "Invalid Argument: null pointer",
    "Texture size isn't power of two",
    "No program has been set"
};

const char* er_status_string(er_StatusEnum status){
    if(status >= 0 && status < sizeof(status_strings)/sizeof(const char*))
        return status_strings[status];
    return NULL;
}

er_StatusEnum er_init(){

    /* Init matrix stack */
    int i;
    for(i = 0; i < MODELVIEW_MATRIX_STACK_SIZE; i++){
        identity_mat4(mv_stack[i]);
        identity_mat4(inv_mv_stack[i]);
        mv_flag[i] = ER_TRUE;
    }
    mv_stack_counter = 0;
    for(i = 0; i < PROJECTION_MATRIX_STACK_SIZE; i++){
        identity_mat4(proj_stack[i]);
        identity_mat4(inv_proj_stack[i]);
        proj_flag[i] = ER_TRUE;
    }
    proj_stack_counter = 0;

    /* Init internal buffers of vertex and indices */
    input_buffer = (er_VertexInput*)malloc( TRIANGLES_BATCH_SIZE * 3 * sizeof(er_VertexInput));
    if(input_buffer == NULL){
        er_quit();
        return ER_OUT_OF_MEMORY;
    }
    input_buffer_size = 0;

    output_buffer = (OutputBufferRegister*)malloc( (TRIANGLES_BATCH_SIZE * 3 + TRIANGLES_BATCH_SIZE * 12) * sizeof(OutputBufferRegister));
    if(output_buffer == NULL){
        er_quit();
        return ER_OUT_OF_MEMORY;
    }
    output_buffer_size = 0;

    input_indices = (unsigned int*)malloc( TRIANGLES_BATCH_SIZE * 3 * sizeof(unsigned int) );
    if(input_indices == NULL){
        er_quit();
        return ER_OUT_OF_MEMORY;
    }
    input_indices_size = 0;

    output_indices = (unsigned int*)malloc( (TRIANGLES_BATCH_SIZE * 7 * 3) * sizeof(unsigned int) );
    if(output_indices == NULL){
        er_quit();
        return ER_OUT_OF_MEMORY;
    }
    output_indices_size = 0;

    /* Set current vertex array to null */
    current_vertex_array = NULL;

    /* Set current program to null */
    current_program = NULL;

    /* Set current process function to null */
    be_process_func = NULL;

    /* Point sprites settings */
    point_sprite_coord_origin = ER_POINT_SPRITE_LOWER_LEFT;
    point_sprite_enable = ER_FALSE;

    /* Orientation settings */
    front_face_orientation = ER_COUNTER_CLOCK_WISE;
    front_face_mode = ER_FILL;
    back_face_mode = ER_FILL;
    cull_face_enable = ER_FALSE;
    cull_face = ER_BACK;

    return ER_NO_ERROR;

}

void er_quit(){

    if(input_buffer != NULL){
        free(input_buffer);
        input_buffer = NULL;
    }
    if(output_buffer != NULL){
        free(output_buffer);
        output_buffer = NULL;
    }
    if(input_indices != NULL){
        free(input_indices);
        input_indices = NULL;
    }
    if(output_indices != NULL){
        free(output_indices);
        output_indices = NULL;
    }

}

er_StatusEnum er_enable(er_EnableSettingEnum param, er_Bool enable){

    switch( param ){

        case ER_CULL_FACE:
            cull_face_enable = enable;
            break;
        case ER_POINT_SPRITES:
            point_sprite_enable = enable;
            break;
        default:
            return ER_INVALID_ARGUMENT;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_point_parameteri(er_PointSpriteEnum param, er_PointSpriteEnum value){

    if( param == ER_POINT_SPRITE_COORD_ORIGIN){
        point_sprite_coord_origin = value;
    }else{
        return ER_INVALID_ARGUMENT;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_cull_face(er_PolygonFaceEnum face){
    if(face == ER_FRONT || face == ER_BACK || face == ER_FRONT_AND_BACK){
        cull_face = face;
    }else{
        return ER_INVALID_ARGUMENT;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_front_face(er_PolygonOrientationEnum orientation){
    if(orientation == ER_COUNTER_CLOCK_WISE || orientation == ER_CLOCK_WISE){
        front_face_orientation = orientation;
    }else{
        return ER_INVALID_ARGUMENT;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_viewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height){

    if(width == 0 || height == 0){
        return ER_INVALID_ARGUMENT;
    }
    window_origin_x = x;
    window_origin_y = y;
    window_width = width;
    window_height = height;
    return ER_NO_ERROR;
}

er_StatusEnum er_polygon_mode(er_PolygonFaceEnum face, er_PolygonModeEnum mode){

    if(face == ER_FRONT){
        front_face_mode = mode;
    }else if(face == ER_BACK){
        back_face_mode = mode;
    } else if(face == ER_FRONT_AND_BACK){
        front_face_mode = mode;
        back_face_mode = mode;
    }else{
        return ER_INVALID_ARGUMENT;
    }
    return ER_NO_ERROR;
}

static void clear_vertex_cache(){

    int i;
    for(i = 0; i < VERTEX_CACHE_SIZE; i++){
        vertex_cache[i].input_index = -1;
        vertex_cache[i].output_index = -1;
    }

}

static int hit_cache(unsigned int input_index, unsigned int *output_index ){

    unsigned int hash_index = input_index & (VERTEX_CACHE_SIZE - 1);
    if(vertex_cache[hash_index].input_index == input_index){
        *output_index = vertex_cache[hash_index].output_index;
        return ER_TRUE;
    }
    return ER_FALSE;
}

static void write_cache(unsigned int input_index, unsigned int output_index){

    unsigned int hash_index = input_index & (VERTEX_CACHE_SIZE - 1);
    vertex_cache[hash_index].input_index = input_index;
    vertex_cache[hash_index].output_index = output_index;

}

void reset_buffers_size(){

    input_buffer_size = 0;
    output_buffer_size = 0;
    input_indices_size = 0;
    output_indices_size = 0;

}

void update_uniform_vars(){

    global_variables.modelview = mv_stack[mv_stack_counter];
    global_variables.modelview_inverse = inv_mv_stack[mv_stack_counter];
    global_variables.normal = nm_stack[mv_stack_counter];
    global_variables.projection = proj_stack[proj_stack_counter];
    global_variables.projection_inverse = inv_proj_stack[proj_stack_counter];
    global_variables.modelview_projection = mv_proj_matrix;
    global_variables.origin_x = window_origin_x;
    global_variables.origin_y = window_origin_y;
    global_variables.width =window_width;
    global_variables.height = window_height;
    global_variables.uniform_integer = current_program->uniform_integer;
    global_variables.uniform_float = current_program->uniform_float;
    global_variables.uniform_ptr = current_program->uniform_ptr;
    global_variables.uniform_texture = current_program->uniform_texture;

}

er_StatusEnum er_begin(er_PrimitiveEnum primitive){
    //Update matrix data
    update_matrix_data();
    //Reset internal buffers size
    reset_buffers_size();
    //Update global state
    update_uniform_vars();
    if(primitive == ER_POINTS){
        be_process_func = process_points;
        batch_size = POINTS_BATCH_SIZE;
    }else if(primitive == ER_LINES){
        be_process_func = process_lines;
        batch_size = LINES_BATCH_SIZE * 2;
    }else if(primitive == ER_TRIANGLES){
        be_process_func = process_triangles;
        batch_size = TRIANGLES_BATCH_SIZE * 3;
    }else{
        be_process_func = NULL;
        return ER_INVALID_ARGUMENT;
    }
    return ER_NO_ERROR;
}

void er_end(){
    if(be_process_func != NULL && input_buffer_size > 0){
        be_process_func();
    }
    be_process_func = NULL;
}

void er_normal3f(float nx, float ny, float nz){
    current_normal[VAR_X] = nx;
    current_normal[VAR_Y] = ny;
    current_normal[VAR_Z] = nz;
}

er_StatusEnum er_normal3fv(float *normal){
    if(normal != NULL){
        current_normal[VAR_X] = normal[VAR_X];
        current_normal[VAR_Y] = normal[VAR_Y];
        current_normal[VAR_Z] = normal[VAR_Z];
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

void er_color3f(float r, float g, float b){
    current_color[VAR_R] = r;
    current_color[VAR_G] = g;
    current_color[VAR_B] = b;
    current_color[VAR_A] = 1.0f;
}

er_StatusEnum er_color3fv(float *color){
    if(color != NULL){
        current_color[VAR_R] = color[VAR_R];
        current_color[VAR_G] = color[VAR_G];
        current_color[VAR_B] = color[VAR_B];
        current_color[VAR_A] = 1.0f;
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

void er_color4f(float r, float g, float b, float a){
    current_color[VAR_R] = r;
    current_color[VAR_G] = g;
    current_color[VAR_B] = b;
    current_color[VAR_A] = a;
}

er_StatusEnum er_color4fv(float *color){
    if(color != NULL){
        current_color[VAR_R] = color[VAR_R];
        current_color[VAR_G] = color[VAR_G];
        current_color[VAR_B] = color[VAR_B];
        current_color[VAR_A] = color[VAR_A];
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

void er_tex_coord1f(float s){
    current_tex_coord[VAR_S] = s;
    current_tex_coord[VAR_T] = 0.0f;
    current_tex_coord[VAR_P] = 0.0f;
    current_tex_coord[VAR_Q] = 1.0f;
}

er_StatusEnum er_tex_coord1fv(float *tex_coord){
    if(tex_coord != NULL){
        current_tex_coord[VAR_S] = tex_coord[VAR_S];
        current_tex_coord[VAR_T] = 0.0f;
        current_tex_coord[VAR_P] = 0.0f;
        current_tex_coord[VAR_Q] = 1.0f;
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

void er_tex_coord2f(float s, float t){
    current_tex_coord[VAR_S] = s;
    current_tex_coord[VAR_T] = t;
    current_tex_coord[VAR_P] = 0.0f;
    current_tex_coord[VAR_Q] = 1.0f;
}

er_StatusEnum er_tex_coord2fv(float *tex_coord){
    if(tex_coord != NULL){
        current_tex_coord[VAR_S] = tex_coord[VAR_S];
        current_tex_coord[VAR_T] = tex_coord[VAR_T];
        current_tex_coord[VAR_P] = 0.0f;
        current_tex_coord[VAR_Q] = 1.0f;
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

void er_tex_coord3f(float s, float t, float r){
    current_tex_coord[VAR_S] = s;
    current_tex_coord[VAR_T] = t;
    current_tex_coord[VAR_P] = r;
    current_tex_coord[VAR_Q] = 1.0f;
}

er_StatusEnum er_tex_coord3fv(float *tex_coord){
    if(tex_coord != NULL){
        current_tex_coord[VAR_S] = tex_coord[VAR_S];
        current_tex_coord[VAR_T] = tex_coord[VAR_T];
        current_tex_coord[VAR_P] = tex_coord[VAR_P];
        current_tex_coord[VAR_Q] = 1.0f;
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

void er_tex_coord4f(float s, float t, float r, float q){
    current_tex_coord[VAR_S] = s;
    current_tex_coord[VAR_T] = t;
    current_tex_coord[VAR_P] = r;
    current_tex_coord[VAR_Q] = q;
}

er_StatusEnum er_tex_coord4fv(float *tex_coord){
    if(tex_coord != NULL){
        current_tex_coord[VAR_S] = tex_coord[VAR_S];
        current_tex_coord[VAR_T] = tex_coord[VAR_T];
        current_tex_coord[VAR_P] = tex_coord[VAR_P];
        current_tex_coord[VAR_Q] = tex_coord[VAR_Q];
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

void er_fog_coordf(float fog_coord){
    current_fog_coord = fog_coord;
}

er_StatusEnum er_fog_coordfv(float *fog_coord){
    if(fog_coord != NULL){
        current_fog_coord = *fog_coord;
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

void er_vertex2f(float x, float y){
    er_vertex4f(x, y, 0.0f, 1.0f);
}

er_StatusEnum er_vertex2fv(float *point){
    if(point != NULL){
        er_vertex4f(point[VAR_X], point[VAR_Y], 0.0f, 1.0f);
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

void er_vertex3f(float x, float y, float z){
    er_vertex4f(x, y, z, 1.0f);
}

er_StatusEnum er_vertex3fv(float *point){
    if(point != NULL){
        er_vertex4f(point[VAR_X], point[VAR_Y], point[VAR_Z], 1.0f);
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

void er_vertex4f(float x, float y, float z, float w){

    if(be_process_func == NULL) {
        return;
    }

    er_VertexInput* vertex;
    unsigned int output_index;

    /* Get index of new vertex on input buffer*/
    output_index = input_buffer_size++;
    /* Add index to array of indices */
    input_indices[input_indices_size++] = output_index;
    /* Add new vertex to input buffer */
    vertex = &input_buffer[output_index];
    vertex->position[VAR_X] = x;
    vertex->position[VAR_Y] = y;
    vertex->position[VAR_Z] = z;
    vertex->position[VAR_W] = w;
    vertex->color[VAR_R] = current_color[VAR_R];
    vertex->color[VAR_G] = current_color[VAR_G];
    vertex->color[VAR_B] = current_color[VAR_B];
    vertex->color[VAR_A] = current_color[VAR_A];
    vertex->normal[VAR_X] = current_normal[VAR_X];
    vertex->normal[VAR_Y] = current_normal[VAR_Y];
    vertex->normal[VAR_Z] = current_normal[VAR_Z];
    vertex->tex_coord[VAR_S] = current_tex_coord[VAR_S];
    vertex->tex_coord[VAR_T] = current_tex_coord[VAR_T];
    vertex->tex_coord[VAR_P] = current_tex_coord[VAR_P];
    vertex->tex_coord[VAR_Q] = current_tex_coord[VAR_Q];
    vertex->fog_coord = current_fog_coord;

    /* Verify batch size and process geometry */
    if(input_buffer_size >= batch_size){
        be_process_func();
    }

}

er_StatusEnum er_vertex4fv(float *point){
    if(point != NULL){
        er_vertex4f(point[VAR_X], point[VAR_Y], point[VAR_Z], point[VAR_W]);
    }else{
        return ER_NULL_POINTER;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_draw_elements(er_PrimitiveEnum primitive, unsigned int indices_size, unsigned int *index){

    if(current_program == NULL){
        return ER_NO_PROGRAM_SET;
    }
    if(current_vertex_array == NULL){
        return ER_NO_VERTEX_ARRAY_SET;
    }
    if(indices_size == 0){
        return ER_INVALID_ARGUMENT;
    }
    if(index == NULL){
        return ER_INVALID_ARGUMENT;
    }
    if(primitive != ER_POINTS && primitive != ER_LINES && primitive != ER_TRIANGLES){
        return ER_INVALID_ARGUMENT;
    }

    update_matrix_data();
    update_uniform_vars();
    reset_buffers_size();
    be_process_func = NULL;

    unsigned int output_index, i, b, begin, end, batch_number, batch_size;
    void (*process_func)(void) = NULL;

    if(primitive == ER_POINTS){
        batch_size = POINTS_BATCH_SIZE;
        process_func = process_points;
    }else if(primitive == ER_LINES){
        batch_size = LINES_BATCH_SIZE * 2;
        process_func = process_lines;
    }else if(primitive == ER_TRIANGLES){
        batch_size = TRIANGLES_BATCH_SIZE * 3;
        process_func = process_triangles;
    }

    batch_number = indices_size / batch_size;

    for(b = 0; b < batch_number; b++){
        /* Clear cache */
        clear_vertex_cache();
        /* Fill input buffer with vertex data */
        begin = b * batch_size;
        end = begin + batch_size;
        for(i = begin; i < end; i++){
            if(hit_cache(index[i], &output_index) == ER_FALSE){
                output_index = input_buffer_size++;
                vertex_assembly(current_vertex_array, &input_buffer[output_index], index[i]);
                write_cache(index[i], output_index);
            }
            input_indices[input_indices_size++] = output_index;
        }
        /* Process batch of primitives */
        process_func();
    }

    if(indices_size > batch_number * batch_size){
        /* Clear cache */
        clear_vertex_cache();
        /* Fill input buffer with vertex data */
        begin = batch_number * batch_size;
        end = indices_size;
        for(i = begin; i < end; i++){
            if(hit_cache(index[i], &output_index) == ER_FALSE){
                output_index = input_buffer_size++;
                vertex_assembly(current_vertex_array, &input_buffer[output_index], index[i]);
                write_cache(index[i], output_index);
            }
            input_indices[input_indices_size++] = output_index;
        }
        /* Process batch of primitives */
        process_func();
    }

    return ER_NO_ERROR;

}

er_StatusEnum er_draw_arrays(er_PrimitiveEnum primitive, unsigned int first, unsigned int count){

    if(current_program == NULL){
        return ER_NO_PROGRAM_SET;
    }
    if(current_vertex_array == NULL){
        return ER_INVALID_ARGUMENT;
    }
    if(count == 0){
        return ER_INVALID_ARGUMENT;
    }
    if(primitive != ER_POINTS && primitive != ER_LINES && primitive != ER_TRIANGLES){
        return ER_INVALID_ARGUMENT;
    }

    update_matrix_data();
    update_uniform_vars();
    reset_buffers_size();
    be_process_func = NULL;

    unsigned int output_index, i, b, begin, end, batch_number, batch_size;
    void (*process_func)(void) = NULL;

    if(primitive == ER_POINTS){
        batch_size = POINTS_BATCH_SIZE;
        process_func = process_points;
    }else if(primitive == ER_LINES){
        batch_size = LINES_BATCH_SIZE * 2;
        process_func = process_lines;
    }else if(primitive == ER_TRIANGLES){
        batch_size = TRIANGLES_BATCH_SIZE * 3;
        process_func = process_triangles;
    }

    batch_number = count / batch_size;

    for(b = 0; b < batch_number; b++){
        /* Fill input buffer with vertex data */
        begin = b * batch_size;
        end = begin + batch_size;
        for(i = begin; i < end; i++){
            output_index = input_buffer_size++;
            vertex_assembly(current_vertex_array, &input_buffer[output_index], first + i);
            input_indices[input_indices_size++] = output_index;
        }
        /* Process batch of primitives */
        process_func();
    }

    if(count > batch_number * batch_size){
        /* Fill input buffer with vertex data */
        begin = batch_number * batch_size;
        end = count;
        for(i = begin; i < end; i++){
            output_index = input_buffer_size++;
            vertex_assembly(current_vertex_array, &input_buffer[output_index], first + i);
            input_indices[input_indices_size++] = output_index;
        }
        /* Process batch of primitives */
        process_func();
    }
    return ER_NO_ERROR;
}

void post_clipping_operations(er_VertexOutput *vertex){

    /* Homogeneous division  */
    if(current_program->homogeneous_division != NULL){
        current_program->homogeneous_division(vertex);
    }

    /* Window to viewport transformation, and conversion of additional parameters */
    vertex->position[VAR_X] = - 0.5f + ( window_width - 0.001f) * ( vertex->position[VAR_X] + 1.0f ) / 2.0f;
    vertex->position[VAR_Y] = - 0.5f + ( window_height - 0.001f) * ( vertex->position[VAR_Y] + 1.0f ) / 2.0f;
    vertex->position[VAR_Z] = ( -vertex->position[VAR_Z] + 1.0f) / 2.0f;

}

void process_points(){

    unsigned int i;

    /* Vertex Pipeline and outcodes */
    for(i = 0; i < input_buffer_size; i++){
        current_program->vertex_shader(&input_buffer[i], &(output_buffer[i].vertex), &global_variables);
        output_buffer[i].outcode = calculate_outcode(&(output_buffer[i].vertex));
        output_buffer[i].processed = ER_FALSE;
    }
    output_buffer_size = input_buffer_size;

    /* Clipping */
    for(i = 0; i < input_indices_size; i++){
        clip_point(input_indices[i]);
    }
    if(output_indices_size == 0){
        reset_buffers_size();
        return;
    }

    /* Homogeneous Division, Window to viewport transformation, and depth conversion */
    for(i = 0; i < output_indices_size; i++){
        if(output_buffer[ output_indices[i] ].processed == ER_FALSE){
            post_clipping_operations( &(output_buffer[ output_indices[i] ].vertex) );
            output_buffer[ output_indices[i] ].processed = ER_TRUE;
        }
    }

    /* Rasterize Points */
    if(point_sprite_enable == ER_TRUE){
        for(i = 0; i < output_indices_size; i++){
            draw_point_sprite(&(output_buffer[output_indices[i]].vertex), ER_FRONT);
        }
    }else{
        for(i = 0; i < output_indices_size; i++){
            draw_point(&(output_buffer[output_indices[i]].vertex), ER_FRONT);
        }
    }

    reset_buffers_size();

}

void process_lines(){

    unsigned int i, size;

    /* Vertex Pipeline and outcodes */
    for(i = 0; i < input_buffer_size; i++){
        current_program->vertex_shader(&input_buffer[i], &(output_buffer[i].vertex), &global_variables );
        output_buffer[i].outcode = calculate_outcode(&(output_buffer[i].vertex));
        output_buffer[i].processed = ER_FALSE;
    }
    output_buffer_size = input_buffer_size;

    /* Clipping */
    size = input_indices_size / 2 * 2;
    for(i = 0; i < size; i+=2){
        clip_line(input_indices[i], input_indices[i+1]);
    }
    if(output_indices_size == 0){
        reset_buffers_size();
        return;
    }

    /* Homogeneous Division, Window to viewport transformation, and depth conversion */
    size = output_indices_size / 2 * 2;
    for(i = 0; i < size; i++){
        if(output_buffer[ output_indices[i] ].processed == ER_FALSE){
            post_clipping_operations( &(output_buffer[ output_indices[i] ].vertex) );
            output_buffer[ output_indices[i] ].processed = ER_TRUE;
        }
    }

    /* Rasterize lines */
    for(i = 0; i < size; i+=2){
        draw_line(&(output_buffer[ output_indices[i] ].vertex), &(output_buffer[ output_indices[i+1] ].vertex), ER_FRONT);
    }

    reset_buffers_size();

}

void process_triangles(){

    unsigned int i, size;

    /* Vertex Pipeline and outcodes */
    for(i = 0; i < input_buffer_size; i++){
        current_program->vertex_shader(&input_buffer[i], &(output_buffer[i].vertex), &global_variables);
        output_buffer[i].outcode = calculate_outcode(&(output_buffer[i].vertex));
        output_buffer[i].processed = ER_FALSE;
    }
    output_buffer_size = input_buffer_size;

    /* Clipping */
    size = input_indices_size / 3 * 3;
    for(i = 0; i < size; i+=3){
        clip_triangle(input_indices[i], input_indices[i+1], input_indices[i+2]);
    }
    if(output_indices_size == 0){
        reset_buffers_size();
        return;
    }

    /* Homogeneous Division, Window to viewport transformation, and depth conversion */
    size = output_indices_size / 3 * 3;
    for(i = 0; i < size; i++){
        if(output_buffer[ output_indices[i] ].processed == ER_FALSE){
            post_clipping_operations( &(output_buffer[ output_indices[i] ].vertex) );
            output_buffer[ output_indices[i] ].processed = ER_TRUE;
        }
    }
    er_VertexOutput *vertex0;
    er_VertexOutput *vertex1;
    er_VertexOutput *vertex2;
    float triangle_area = 0.0f;
    unsigned int orientation, face;

    for(i = 0; i < size; i+=3){

        vertex0 = &(output_buffer[ output_indices[i] ].vertex);
        vertex1 = &(output_buffer[ output_indices[i+1] ].vertex);
        vertex2 = &(output_buffer[ output_indices[i+2] ].vertex);

        /* Polygon orientation and face determination */
        triangle_area = (vertex1->position[VAR_X] - vertex0->position[VAR_X]) * (vertex2->position[VAR_Y] - vertex0->position[VAR_Y]) - 
        (vertex2->position[VAR_X] - vertex0->position[VAR_X]) * (vertex1->position[VAR_Y] - vertex0->position[VAR_Y]);

        if(triangle_area >= 0.0f){
            orientation = ER_COUNTER_CLOCK_WISE;
            if( front_face_orientation == ER_COUNTER_CLOCK_WISE){
                face = ER_FRONT;
            }else{
                face = ER_BACK;
            }
        }else{
            orientation = ER_CLOCK_WISE;
            if( front_face_orientation == ER_CLOCK_WISE){
                face = ER_FRONT;
            }else{
                face = ER_BACK;
            }
        }

        /* Backface culling */
        if(cull_face_enable == ER_TRUE && face == cull_face){
            continue;
        }

        /* Polygon mode */
        if(face == ER_FRONT ){
            if( front_face_mode == ER_LINE){
                draw_line(vertex0, vertex1, face);
                draw_line(vertex1, vertex2, face);
                draw_line(vertex2, vertex0, face);
                continue;
            }else if(front_face_mode == ER_POINT){
                if(point_sprite_enable == ER_TRUE){
                    draw_point_sprite(vertex0, face);
                    draw_point_sprite(vertex1, face);
                    draw_point_sprite(vertex2, face);
                }else{
                    draw_point(vertex0, face);
                    draw_point(vertex1, face);
                    draw_point(vertex2, face);
                }
                continue;
            }
        }else if(face == ER_BACK ){
            if( back_face_mode == ER_LINE){
                draw_line(vertex0, vertex1, face);
                draw_line(vertex1, vertex2, face);
                draw_line(vertex2, vertex0, face);
                continue;
            }else if(back_face_mode == ER_POINT){
                if(point_sprite_enable == ER_TRUE){
                    draw_point_sprite(vertex0, face);
                    draw_point_sprite(vertex1, face);
                    draw_point_sprite(vertex2, face);
                }else{
                    draw_point(vertex0, face);
                    draw_point(vertex1, face);
                    draw_point(vertex2, face);
                }
                continue;
            }
        }

        /* Raster triangle  */
        if( orientation == ER_COUNTER_CLOCK_WISE){
            draw_triangle(vertex0, vertex1, vertex2, face);
        }else{
            draw_triangle(vertex2, vertex1, vertex0, face);
        }

    }

    reset_buffers_size();

}
