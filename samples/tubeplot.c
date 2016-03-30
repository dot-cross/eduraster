#include <stdio.h>
#include <stdlib.h>
#include "eduraster.h"
#include "SDL.h"
#include "simple_parser.h"

struct curve_frame{
    vec3 tangent;
    vec3 normal;
    vec3 binormal;
    vec3 position;
};

enum fill_mode {wireframe = 0, solid = 1};
enum frame_technique {frenet_frame = 0, rotation_minimizing_frame = 1};

/* Window dimensions */
static unsigned int window_width = 1024, window_height = 768;
/* Axis drawing constants */
static const float axis_length = 6.0f, arrow_length = 1.0f, arrow_radius = 0.3f;
static const float lut_cos[]={1.000000f, 0.951057f, 0.809017f, 0.587785f, 0.309017f, -0.000000f, -0.309017f, -0.587785f, -0.809017f, -0.951057f, -1.000000f, -0.951057f, -0.809017f, -0.587785f, -0.309017f, 0.000000f, 0.309017f, 0.587785f, 0.809017f, 0.951057f};
static const float lut_sin[]={0.000000f, 0.309017f, 0.587785f, 0.809017f, 0.951057f, 1.000000f, 0.951057f, 0.809017f, 0.587785f, 0.309017f, -0.000000f, -0.309017f, -0.587785f, -0.809017f, -0.951056f, -1.000000f, -0.951056f, -0.809017f, -0.587785f, -0.309017f};
static const unsigned int lut_samples = 20;
/* Curve data */
static struct expression *exp_x = NULL, *exp_y = NULL, *exp_z = NULL;
static struct var var_t = {"t", 0.0f};
static float min_t, max_t;
unsigned int curve_points_size;
static struct curve_frame *curve_frames;
/* Surface data */
static struct vertex_array *va = NULL;
static float *points = NULL, *normals = NULL;
static unsigned int *index_data = NULL, index_size;
static unsigned int cross_section_size;
static float cross_section_radius = 1.0f;
/* Programs */
static struct program *program_surface_light = NULL;
static struct program *program_axis = NULL;
static struct program *program_surface_color = NULL;
static struct program *program_lines = NULL;
static struct program *current_surface_program;
/* Scene settings */
static unsigned int light_enable = ER_TRUE;
static unsigned int show_curve = ER_FALSE;
static enum fill_mode wireframe_flag = solid;
static enum frame_technique current_frame = rotation_minimizing_frame;
/* Camera */
static float camera_theta = 30.0f, camera_phi = -20.0f, camera_offset = 15.0f;
/* Color and Depth buffer */
static unsigned int *color_buffer = NULL;
static float *depth_buffer = NULL;
/* SDL structures */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

/* Free parsing data */
static void free_parsing_data(){
    free_exp(exp_x);
    free_exp(exp_y); 
    free_exp(exp_z);
    exp_x = NULL;
    exp_y = NULL;
    exp_z = NULL;
}

/*
* Free resources and exit program.
*/
static void quit(){
    if(program_surface_light != NULL){
        er_delete_program(program_surface_light);
        program_surface_light = NULL;
    }
    if(program_axis != NULL){
        er_delete_program(program_axis);
        program_axis = NULL;
    }
    if(program_surface_color != NULL){
        er_delete_program(program_surface_color);
        program_surface_color = NULL;
    }
    if(program_lines != NULL){
        er_delete_program(program_lines);
        program_lines = NULL;
    }
    if(va != NULL){
        er_delete_vertex_array(va);
        va = NULL;
    }
    er_quit();
    if(depth_buffer != NULL){
        free(depth_buffer);
        depth_buffer = NULL;
    }
    if(points != NULL){
        free(points);
        points = NULL;
    }
    if(normals != NULL){
        free(normals);
        normals = NULL;
    }
    if(index_data != NULL){
        free(index_data);
        index_data = NULL;
    }
    if(curve_frames != NULL){
        free(curve_frames);
        curve_frames = NULL;
    }
    free_parsing_data();
    if(window != NULL){
        SDL_DestroyWindow(window);
        window = NULL;
    }
    if(renderer != NULL){
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if(texture != NULL){
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    SDL_Quit();
    exit(0);
}

/* 
* Clear depth and color buffer
*/
static void clear_buffer(){
    int i;
    int length = window_width * window_height;
    for(i = 0; i < length; i++){
        color_buffer[i] = 0xff000000;
        depth_buffer[i] = 1.0f;
    }
}

/*
* Function to write color buffer
*/
static void write_color(int y, int x, float red, float green, float blue, float alpha){
    int inv_y = window_height - 1 - y;
    unsigned char r, g, b;
    r = uiround(255.0f * red);
    g = uiround(255.0f * green);
    b = uiround(255.0f * blue);
    color_buffer[inv_y * window_width + x] = 255 << 24 | r << 16 | g << 8 | b;
}

/*
* Function to read depth buffer
*/
static float read_depth(int y, int x){
    return depth_buffer[y*window_width + x];
}

/*
* Function to write depth buffer
*/
static void write_depth(int y, int x, float depth){
    depth_buffer[y*window_width+ x] = depth;
}

/*
* Draw axis and arrows.
*/
static void draw_axis(){
    unsigned int i;
    /* Lines */
    er_begin(ER_LINES);
    /* X axis */
    er_color3f(0.5f, 0.0f, 0.0f);
    er_vertex3f(0.0f, 0.0f, 0.0f);
    er_vertex3f(axis_length, 0.0f, 0.0f);
    er_vertex3f(0.0f, 0.0f, 0.0f);
    er_vertex3f(-axis_length, 0.0f, 0.0f);
    /* Y axis */
    er_color3f(0.0f, 0.5f, 0.0f);
    er_vertex3f(0.0f, 0.0f, 0.0f);
    er_vertex3f(0.0f, axis_length, 0.0f);
    er_vertex3f(0.0f, 0.0f, 0.0f);
    er_vertex3f(0.0f, -axis_length, 0.0f);
    /* Z axis*/
    er_color3f(0.0f, 0.0f, 0.5f);
    er_vertex3f(0.0f, 0.0f, 0.0f);
    er_vertex3f(0.0f, 0.0f, axis_length);
    er_vertex3f(0.0f, 0.0f, 0.0f);
    er_vertex3f(0.0f, 0.0f, -axis_length);
    er_end();
    /* Arrows */
    er_begin(ER_TRIANGLES);
    /* X axis */
    er_color3f(0.5f, 0.0f, 0.0f);
    er_vertex3f(axis_length, arrow_radius*lut_sin[lut_samples-1], -arrow_radius*lut_cos[lut_samples-1]);
    er_vertex3f(axis_length, arrow_radius*lut_sin[0], -arrow_radius*lut_cos[0]);
    er_vertex3f(axis_length+arrow_length, 0.0f, 0.0f);
    for(i = 0; i < lut_samples-1; i++){
        er_vertex3f(axis_length, arrow_radius*lut_sin[i], -arrow_radius*lut_cos[i]);
        er_vertex3f(axis_length, arrow_radius*lut_sin[i+1], -arrow_radius*lut_cos[i+1]);
        er_vertex3f(axis_length+arrow_length, 0.0f, 0.0f);
    }
    er_vertex3f(-axis_length, arrow_radius*lut_sin[lut_samples-1], -arrow_radius*lut_cos[lut_samples-1]);
    er_vertex3f(-axis_length, arrow_radius*lut_sin[0], -arrow_radius*lut_cos[0]);
    er_vertex3f(-(axis_length+arrow_length), 0.0f, 0.0f);
    for(i = 0; i < lut_samples-1; i++){
        er_vertex3f(-axis_length, arrow_radius*lut_sin[i], -arrow_radius*lut_cos[i]);
        er_vertex3f(-axis_length, arrow_radius*lut_sin[i+1], -arrow_radius*lut_cos[i+1]);
        er_vertex3f(-(axis_length+arrow_length), 0.0f, 0.0f);
    }
    /* Y axis */
    er_color3f(0.0f, 0.5f, 0.0f);
    er_vertex3f(arrow_radius*lut_sin[lut_samples-1], axis_length, arrow_radius*lut_cos[lut_samples-1]);
    er_vertex3f(arrow_radius*lut_sin[0],  axis_length, arrow_radius*lut_cos[0]);
    er_vertex3f(0.0f, axis_length+arrow_length, 0.0f);
    for(i = 0; i < lut_samples-1; i++){
        er_vertex3f(arrow_radius*lut_sin[i], axis_length, arrow_radius*lut_cos[i]);
        er_vertex3f( arrow_radius*lut_sin[i+1], axis_length, arrow_radius*lut_cos[i+1]);
        er_vertex3f(0.0f,axis_length+arrow_length, 0.0f);
    }
    er_color3f(0.0f, 0.5f, 0.0f);
    er_vertex3f(arrow_radius*lut_sin[lut_samples-1], -axis_length, arrow_radius*lut_cos[lut_samples-1]);
    er_vertex3f(arrow_radius*lut_sin[0],  -axis_length, arrow_radius*lut_cos[0]);
    er_vertex3f(0.0f, -(axis_length+arrow_length), 0.0f);
    for(i = 0; i < lut_samples-1; i++){
        er_vertex3f(arrow_radius*lut_sin[i], -axis_length, arrow_radius*lut_cos[i]);
        er_vertex3f( arrow_radius*lut_sin[i+1], -axis_length, arrow_radius*lut_cos[i+1]);
        er_vertex3f(0.0f,-(axis_length+arrow_length), 0.0f);
    }
    /* Z axis */
    er_color3f(0.0f, 0.0f, 0.5f);
    er_vertex3f(arrow_radius*lut_cos[lut_samples-1], arrow_radius*lut_sin[lut_samples-1], axis_length);
    er_vertex3f(arrow_radius*lut_cos[0], arrow_radius*lut_sin[0], axis_length);
    er_vertex3f(0.0f, 0.0f, axis_length+arrow_length);
    for(i = 0; i < lut_samples-1; i++){
        er_vertex3f(arrow_radius*lut_cos[i], arrow_radius*lut_sin[i], axis_length);
        er_vertex3f(arrow_radius*lut_cos[i+1], arrow_radius*lut_sin[i+1], axis_length);
        er_vertex3f(0.0f, 0.0f, axis_length+arrow_length);
    }
    er_vertex3f(arrow_radius*lut_cos[lut_samples-1], arrow_radius*lut_sin[lut_samples-1], -axis_length);
    er_vertex3f(arrow_radius*lut_cos[0], arrow_radius*lut_sin[0], -axis_length);
    er_vertex3f(0.0f, 0.0f, -(axis_length+arrow_length));
    for(i = 0; i < lut_samples-1; i++){
        er_vertex3f(arrow_radius*lut_cos[i], arrow_radius*lut_sin[i], -axis_length);
        er_vertex3f(arrow_radius*lut_cos[i+1], arrow_radius*lut_sin[i+1], -axis_length);
        er_vertex3f(0.0f, 0.0f, -(axis_length+arrow_length));
    }
    er_end();
}

/*
* Drawing of curve, normal and binormals.
*/
static void draw_curve(){

    unsigned int i;
    er_use_program(program_lines);
    /* Draw curve */
    er_color3f(1.0f, 1.0f, 1.0f);
    er_begin(ER_LINES);
    struct curve_frame *cframe, *nframe;
    cframe = &(curve_frames[0]);
    for(i = 0; i < curve_points_size-1; i++){
        nframe = &(curve_frames[i+1]);
        er_vertex3f(cframe->position[VAR_X], cframe->position[VAR_Y], cframe->position[VAR_Z]);
        er_vertex3f(nframe->position[VAR_X], nframe->position[VAR_Y], nframe->position[VAR_Z]);
        cframe = nframe;
    }
    er_end();
    /* Draw normals */
    er_color3f(1.0f, 0.0f, 0.0f);
    er_begin(ER_LINES);
    for(i = 0; i < curve_points_size; i++){
        cframe = &(curve_frames[i]);
        er_vertex3f(cframe->position[VAR_X], cframe->position[VAR_Y], cframe->position[VAR_Z]);
        er_vertex3f(cframe->position[VAR_X]+cframe->normal[VAR_X], cframe->position[VAR_Y]+cframe->normal[VAR_Y], cframe->position[VAR_Z]+cframe->normal[VAR_Z]);
        cframe = nframe;
    }
    er_end();
    /* Draw binormals */
    er_color3f(0.0f, 0.0f, 1.0f);
    er_begin(ER_LINES);
    for(i = 0; i < curve_points_size; i++){
        cframe = &(curve_frames[i]);
        er_vertex3f(cframe->position[VAR_X], cframe->position[VAR_Y], cframe->position[VAR_Z]);
        er_vertex3f(cframe->position[VAR_X]+cframe->binormal[VAR_X], cframe->position[VAR_Y]+cframe->binormal[VAR_Y], cframe->position[VAR_Z]+cframe->binormal[VAR_Z]);
        cframe = nframe;
    }
    er_end();
}

/*
* EduRaster drawing routines.
*/
static void draw(){
    clear_buffer();  
    er_load_identity();
    mat3 rotx, roty;
    rotatex_mat3(rotx, deg_to_rad(camera_phi));
    rotatey_mat3(roty, deg_to_rad(camera_theta));
    vec3 cam_pos = {0.0f, 0.0f, camera_offset};
    mult_mat3_vec3(rotx, cam_pos, cam_pos);
    mult_mat3_vec3(roty, cam_pos, cam_pos);
    vec3 up_vec = {0.0f, 1.0f, 0.0f};
    mult_mat3_vec3(rotx, up_vec, up_vec);
    mult_mat3_vec3(roty, up_vec, up_vec);
    er_look_at(cam_pos[0], cam_pos[1], cam_pos[2], 0.0f, 0.0f, 0.0f, up_vec[0], up_vec[1], up_vec[2]);
    er_use_program(program_axis);
    draw_axis();
    if(!show_curve){
        er_use_program(current_surface_program);
        er_draw_elements(ER_TRIANGLES, index_size, index_data);
    }else{
        draw_curve();
    }
}

/*
* Draw and update screen.
*/
static void display (void) {
    draw();
    SDL_UpdateTexture(texture, NULL, color_buffer, window_width*sizeof(unsigned int));
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

/*
* Vertex shader for surface color based on object space normals.
*/
static void normal_color_vs(struct vertex_input *input, struct vertex_output *output, struct uniform_variables *vars){
    multd_mat4_vec4(vars->modelview_projection, input->position, output->position);
    output->attributes[0] = input->normal[VAR_X];
    output->attributes[1] = input->normal[VAR_Y];
    output->attributes[2] = input->normal[VAR_Z];
}

/*
* Homogeneous division for surface color based on object space normals.
*/
static void normal_color_hd(struct vertex_output *vertex){
    vertex->position[VAR_X] = vertex->position[VAR_X] / vertex->position[VAR_W];
    vertex->position[VAR_Y] = vertex->position[VAR_Y] / vertex->position[VAR_W];
    vertex->position[VAR_Z] = vertex->position[VAR_Z] / vertex->position[VAR_W];
    vertex->attributes[0] = vertex->attributes[0] / vertex->position[VAR_W];
    vertex->attributes[1] = vertex->attributes[1] / vertex->position[VAR_W];
    vertex->attributes[2] = vertex->attributes[2] / vertex->position[VAR_W];
    vertex->position[VAR_W] = 1.0f / vertex->position[VAR_W];
}

/*
* Fragment shader for surface color based on object space normals.
*/
static void normal_color_fs(int y, int x, struct fragment_input *input, struct uniform_variables *vars){
    if(input->frag_coord[VAR_Z] >= read_depth(y, x)){
        return;
    }
    vec3 object_normal;
    object_normal[VAR_X] = input->attributes[0] / input->frag_coord[VAR_W];
    object_normal[VAR_Y] = input->attributes[1] / input->frag_coord[VAR_W];
    object_normal[VAR_Z] = input->attributes[2] / input->frag_coord[VAR_W];
    normalize_vec3(object_normal);
    if(input->front_facing == ER_FALSE){
        object_normal[VAR_X] = -object_normal[VAR_X];
        object_normal[VAR_Y] = -object_normal[VAR_Y];
        object_normal[VAR_Z] = -object_normal[VAR_Z];
    }
    float red, green, blue;
    red = (object_normal[VAR_X] * 0.5f + 0.5f);
    red = clamp(red, 0.0f, 1.0f);
    green = (object_normal[VAR_Y] * 0.5f + 0.5f);
    green = clamp(green, 0.0f, 1.0f);
    blue = (object_normal[VAR_X] * 0.5f + 0.5f);
    blue = clamp(blue, 0.0f, 1.0f);

    write_color(y, x, red, green, blue, 1.0f);
    write_depth(y, x, input->frag_coord[VAR_Z]);
}

/*
* Vertex shader for axis and arrows.
*/
static void axis_vs(struct vertex_input *input, struct vertex_output *output, struct uniform_variables *vars){
    multd_mat4_vec4(vars->modelview_projection, input->position, output->position);
    output->attributes[0] = input->color[VAR_R];
    output->attributes[1] = input->color[VAR_G];
    output->attributes[2] = input->color[VAR_B];
    output->attributes[3] = input->color[VAR_A];
}

/*
* Homogeneous division for axis and arrows.
*/
static void axis_hd(struct vertex_output *vertex){
    vertex->position[VAR_X] = vertex->position[VAR_X] / vertex->position[VAR_W];
    vertex->position[VAR_Y] = vertex->position[VAR_Y] / vertex->position[VAR_W];
    vertex->position[VAR_Z] = vertex->position[VAR_Z] / vertex->position[VAR_W];
    vertex->position[VAR_W] = 1.0f / vertex->position[VAR_W];
}

/*
* Fragment shader for axis and arrows.
*/
static void axis_fs(int y, int x, struct fragment_input *input, struct uniform_variables *vars){
    if(input->frag_coord[VAR_Z] >= read_depth(y, x)){
        return;
    }
    write_color(y, x, input->attributes[0], input->attributes[1], input->attributes[2], 1.0f);
    write_depth(y, x, input->frag_coord[VAR_Z]);
}

/*
* Vertex shader for surface lighting
*/
static void light_vs(struct vertex_input *input, struct vertex_output *output, struct uniform_variables *vars){
    vec3 eye_normal;
    multd_mat4_vec4(vars->modelview_projection, input->position, output->position);
    multd_mat3_vec3(vars->normal, input->normal, eye_normal);
    output->attributes[0] = input->normal[VAR_X];
    output->attributes[1] = input->normal[VAR_Y];
    output->attributes[2] = input->normal[VAR_Z];
    output->attributes[3] = eye_normal[VAR_X];
    output->attributes[4] = eye_normal[VAR_Y];
    output->attributes[5] = eye_normal[VAR_Z];
}

/*
* Homogeneous division for surface lighting.
*/
static void light_hd(struct vertex_output *vertex){
    vertex->position[VAR_X] = vertex->position[VAR_X] / vertex->position[VAR_W];
    vertex->position[VAR_Y] = vertex->position[VAR_Y] / vertex->position[VAR_W];
    vertex->position[VAR_Z] = vertex->position[VAR_Z] / vertex->position[VAR_W];
    vertex->attributes[0] = vertex->attributes[0] / vertex->position[VAR_W];
    vertex->attributes[1] = vertex->attributes[1] / vertex->position[VAR_W];
    vertex->attributes[2] = vertex->attributes[2] / vertex->position[VAR_W];
    vertex->attributes[3] = vertex->attributes[3] / vertex->position[VAR_W];
    vertex->attributes[4] = vertex->attributes[4] / vertex->position[VAR_W];
    vertex->attributes[5] = vertex->attributes[5] / vertex->position[VAR_W];
    vertex->position[VAR_W] = 1.0f / vertex->position[VAR_W];
}

/*
* Fragment shader for surface lighting. One directional light source, phong shading.
*/
static void light_fs(int y, int x, struct fragment_input *input, struct uniform_variables *vars){

    if(input->frag_coord[VAR_Z] >= read_depth(y, x)){
        return;
    }
    float* light_dir = &(vars->uniform_float[0]);
    vec3 eye_normal, object_normal;
    object_normal[VAR_X] = input->attributes[0] / input->frag_coord[VAR_W];
    object_normal[VAR_Y] = input->attributes[1] / input->frag_coord[VAR_W];
    object_normal[VAR_Z] = input->attributes[2] / input->frag_coord[VAR_W];
    eye_normal[VAR_X] = input->attributes[3] / input->frag_coord[VAR_W];
    eye_normal[VAR_Y] = input->attributes[4] / input->frag_coord[VAR_W];
    eye_normal[VAR_Z] = input->attributes[5] / input->frag_coord[VAR_W];
    normalize_vec3(eye_normal);
    normalize_vec3(object_normal);
    if(input->front_facing == ER_FALSE){
        eye_normal[VAR_X] = -eye_normal[VAR_X];
        eye_normal[VAR_Y] = -eye_normal[VAR_Y];
        eye_normal[VAR_Z] = -eye_normal[VAR_Z];
        object_normal[VAR_X] = -object_normal[VAR_X];
        object_normal[VAR_Y] = -object_normal[VAR_Y];
        object_normal[VAR_Z] = -object_normal[VAR_Z];
    }
    float diffuse_term = max(0.0f, dot_vec3(light_dir, eye_normal) );
    float specular_term = 0.0f;
    vec3 view_vector;
    vec3 half_vector;
    view_vector[VAR_X] = 0.0f;
    view_vector[VAR_Y] = 0.0f;
    view_vector[VAR_Z] = 1.0f;
    if(diffuse_term > 0.0f){
        half_vector[VAR_X] = light_dir[VAR_X] + view_vector[VAR_X];
        half_vector[VAR_Y] = light_dir[VAR_Y] + view_vector[VAR_Y];
        half_vector[VAR_Z] = light_dir[VAR_Z] + view_vector[VAR_Z];
        normalize_vec3(half_vector);
        specular_term = pow( max(0.0f, dot_vec3(half_vector, eye_normal)), 50.0f );
    }

    float red, green, blue, light_intensity;
    light_intensity = 0.6f * diffuse_term + 0.4f * specular_term;
    red = (object_normal[VAR_X] * 0.5f + 0.5f) * light_intensity;
    red = clamp(red, 0.0f, 1.0f);
    green = (object_normal[VAR_Y] * 0.5f + 0.5f) * light_intensity;
    green = clamp(green, 0.0f, 1.0f);
    blue = (object_normal[VAR_Z] * 0.5f + 0.5f) * light_intensity;
    blue = clamp(blue, 0.0f, 1.0f);

    write_color(y, x, red, green, blue, 1.0f);
    write_depth(y, x, input->frag_coord[VAR_Z]);

}

/**
 * Evaluate curve
 */
static void calculate_curve(){

    unsigned int i;
    float range = max_t-min_t;
    for(i = 0; i < curve_points_size; i++){
        var_t.value = min_t + range*i/(curve_points_size-1);
        curve_frames[i].position[VAR_X] = eval(exp_x);
        curve_frames[i].position[VAR_Y] = eval(exp_y);
        curve_frames[i].position[VAR_Z] = eval(exp_z);
    }

}

/**
 * Calculate reference frames along space curve.
 * Implementation of Rotation Minimizing Frames.
 */
static void calculate_rmf(){
    unsigned int i;
    struct curve_frame *pframe, *cframe, *nframe;
    /* First pass: Calculate tangent vectors */
    vec3 fdiff, bdiff;
    cframe = &(curve_frames[0]);
    nframe = &(curve_frames[1]);
    fdiff[VAR_X] = nframe->position[VAR_X]-cframe->position[VAR_X];
    fdiff[VAR_Y] = nframe->position[VAR_Y]-cframe->position[VAR_Y];
    fdiff[VAR_Z] = nframe->position[VAR_Z]-cframe->position[VAR_Z];
    normalize_vec3(fdiff);
    cframe->tangent[VAR_X] = fdiff[VAR_X];
    cframe->tangent[VAR_Y] = fdiff[VAR_Y];
    cframe->tangent[VAR_Z] = fdiff[VAR_Z];
    bdiff[VAR_X] = fdiff[VAR_X];
    bdiff[VAR_Y] = fdiff[VAR_Y];
    bdiff[VAR_Z] = fdiff[VAR_Z];
    cframe = nframe;
    for(i = 1; i < curve_points_size-1; i++){
        nframe = &(curve_frames[i+1]);
        fdiff[VAR_X] = nframe->position[VAR_X]-cframe->position[VAR_X];
        fdiff[VAR_Y] = nframe->position[VAR_Y]-cframe->position[VAR_Y];
        fdiff[VAR_Z] = nframe->position[VAR_Z]-cframe->position[VAR_Z];
        normalize_vec3(fdiff);
        cframe->tangent[VAR_X] = fdiff[VAR_X]+bdiff[VAR_X];
        cframe->tangent[VAR_Y] = fdiff[VAR_Y]+bdiff[VAR_Y];
        cframe->tangent[VAR_Z] = fdiff[VAR_Z]+bdiff[VAR_Z];
        normalize_vec3(cframe->tangent);
        cframe = nframe;
        bdiff[VAR_X] = fdiff[VAR_X];
        bdiff[VAR_Y] = fdiff[VAR_Y];
        bdiff[VAR_Z] = fdiff[VAR_Z];
    }
    cframe->tangent[VAR_X] = bdiff[VAR_X];
    cframe->tangent[VAR_Y] = bdiff[VAR_Y];
    cframe->tangent[VAR_Z] = bdiff[VAR_Z];
    /* Second pass: Calculate an initial frame, and rotate it along the curve */
    mat3 rotate_matrix;
    cframe = &(curve_frames[0]);
    normal_vec3(cframe->tangent, cframe->normal);
    cross_product(cframe->normal, cframe->tangent, cframe->binormal);
    normalize_vec3(cframe->binormal);
    pframe = cframe;
    for(i = 1; i < curve_points_size; i++){
        cframe = &(curve_frames[i]);
        rotate_align_mat3(rotate_matrix, pframe->tangent, cframe->tangent);
        mult_mat3_vec3(rotate_matrix, pframe->normal, cframe->normal);
        normalize_vec3(cframe->normal);
        mult_mat3_vec3(rotate_matrix, pframe->binormal, cframe->binormal);
        normalize_vec3(cframe->binormal);
        pframe = cframe;
    }
}

/**
 * Calculate reference frames along space curve.
 * Implementation of Frenet frames.
 */
static void calculate_fm(){
    unsigned int i;
    struct curve_frame *pframe, *cframe, *nframe;
    /* First pass: Calculate tangent vectors */
    vec3 fdiff, bdiff;
    cframe = &(curve_frames[0]);
    nframe = &(curve_frames[1]);
    fdiff[VAR_X] = nframe->position[VAR_X]-cframe->position[VAR_X];
    fdiff[VAR_Y] = nframe->position[VAR_Y]-cframe->position[VAR_Y];
    fdiff[VAR_Z] = nframe->position[VAR_Z]-cframe->position[VAR_Z];
    normalize_vec3(fdiff);
    cframe->tangent[VAR_X] = fdiff[VAR_X];
    cframe->tangent[VAR_Y] = fdiff[VAR_Y];
    cframe->tangent[VAR_Z] = fdiff[VAR_Z];
    bdiff[VAR_X] = fdiff[VAR_X];
    bdiff[VAR_Y] = fdiff[VAR_Y];
    bdiff[VAR_Z] = fdiff[VAR_Z];
    cframe = nframe;
    for(i = 1; i < curve_points_size-1; i++){
        nframe = &(curve_frames[i+1]);
        fdiff[VAR_X] = nframe->position[VAR_X]-cframe->position[VAR_X];
        fdiff[VAR_Y] = nframe->position[VAR_Y]-cframe->position[VAR_Y];
        fdiff[VAR_Z] = nframe->position[VAR_Z]-cframe->position[VAR_Z];
        normalize_vec3(fdiff);
        cframe->tangent[VAR_X] = fdiff[VAR_X]+bdiff[VAR_X];
        cframe->tangent[VAR_Y] = fdiff[VAR_Y]+bdiff[VAR_Y];
        cframe->tangent[VAR_Z] = fdiff[VAR_Z]+bdiff[VAR_Z];
        normalize_vec3(cframe->tangent);
        cframe = nframe;
        bdiff[VAR_X] = fdiff[VAR_X];
        bdiff[VAR_Y] = fdiff[VAR_Y];
        bdiff[VAR_Z] = fdiff[VAR_Z];
    }
    cframe->tangent[VAR_X] = bdiff[VAR_X];
    cframe->tangent[VAR_Y] = bdiff[VAR_Y];
    cframe->tangent[VAR_Z] = bdiff[VAR_Z];
    /* Second pass: Calculate normal and binormals vectors */
    cframe = &(curve_frames[0]);
    for(i = 0; i < curve_points_size-1; i++){
        nframe = &(curve_frames[i+1]);
        cframe->normal[VAR_X] = nframe->tangent[VAR_X]-cframe->tangent[VAR_X];
        cframe->normal[VAR_Y] = nframe->tangent[VAR_Y]-cframe->tangent[VAR_Y];
        cframe->normal[VAR_Z] = nframe->tangent[VAR_Z]-cframe->tangent[VAR_Z];
        float length = length_vec3(cframe->normal);
        if(length > 0.001f){
            cframe->normal[VAR_X] /= length;
            cframe->normal[VAR_Y] /= length;
            cframe->normal[VAR_Z] /= length;
        }
        cross_product(cframe->normal, cframe->tangent, cframe->binormal);
        length = length_vec3(cframe->binormal);
        if(length > 0.001f){
            cframe->binormal[VAR_X] /= length;
            cframe->binormal[VAR_Y] /= length;
            cframe->binormal[VAR_Z] /= length;
        }
        cframe = nframe;
    }
    cframe = &(curve_frames[curve_points_size-1]);
    pframe = &(curve_frames[curve_points_size-2]);
    cframe->tangent[VAR_X] = pframe->tangent[VAR_X];
    cframe->tangent[VAR_Y] = pframe->tangent[VAR_Y];
    cframe->tangent[VAR_Z] = pframe->tangent[VAR_Z];
    cframe->normal[VAR_X] = pframe->normal[VAR_X];
    cframe->normal[VAR_Y] = pframe->normal[VAR_Y];
    cframe->normal[VAR_Z] = pframe->normal[VAR_Z];
    cframe->binormal[VAR_X] = pframe->binormal[VAR_X];
    cframe->binormal[VAR_Y] = pframe->binormal[VAR_Y];
    cframe->binormal[VAR_Z] = pframe->binormal[VAR_Z];
}

/**
 * Calculate the generalized cylinder
 */
static void calculate_surface(){

    unsigned int i, j, raw_size = cross_section_size*3;
    float cx, cy, t;
    float *point, *normal;
    struct curve_frame *cframe;
    for(i = 0; i < curve_points_size; i++){
    cframe = &(curve_frames[i]);
        for(j = 0; j < cross_section_size; j++){
            t = (float)j/(float)cross_section_size;
            cx = cross_section_radius*cos(2.0f*M_PI*t);
            cy = cross_section_radius*sin(2.0f*M_PI*t);
            point = &(points[i*raw_size+j*3]);
            point[VAR_X] = cframe->normal[VAR_X]*cx + cframe->binormal[VAR_X]*cy + cframe->position[VAR_X];
            point[VAR_Y] = cframe->normal[VAR_Y]*cx + cframe->binormal[VAR_Y]*cy + cframe->position[VAR_Y];
            point[VAR_Z] = cframe->normal[VAR_Z]*cx + cframe->binormal[VAR_Z]*cy + cframe->position[VAR_Z];
            normal = &(normals[i*raw_size+j*3]);
            normal[VAR_X] = cframe->normal[VAR_X]*cx + cframe->binormal[VAR_X]*cy;
            normal[VAR_Y] = cframe->normal[VAR_Y]*cx + cframe->binormal[VAR_Y]*cy;
            normal[VAR_Z] = cframe->normal[VAR_Z]*cx + cframe->binormal[VAR_Z]*cy;
            normalize_vec3(normal);
        }
    }

}

/*
* Reads range and number of samples for parameter t, number of cross section points and tube radius.
*/
static void input_equation_parameters(){

    printf("\tEquation parameters:\n\n");
    do{
        printf("Curve points ? ");
        scanf("%d",&curve_points_size);
        printf("\n");
    }while( curve_points_size < 2);
    printf("Min T ? ");
    scanf("%f",&min_t);
    printf("\n");
    do{
        printf("Max T ? ");
        scanf("%f",&max_t);
        printf("\n");
    }while(max_t <= min_t);
    do{
        printf("Cross-section points ? ");
        scanf("%d",&cross_section_size);
        printf("\n");
    }while(cross_section_size < 3);
    do{
        printf("Cross-section radius ? ");
        scanf("%f",&cross_section_radius);
        printf("\n");
    }while(cross_section_size <= 0.0f);

}

/*
* Initialization of buffers and EduRaster structures.
*/
static void setup(){
    /* Allocate color buffer */
    color_buffer = (unsigned int*)malloc(window_width*window_height*sizeof(unsigned int));
    if(color_buffer == NULL){
        fprintf(stderr, "Unable to allocate color buffer, out of memory\n");
        quit();
    }
    /* Allocate depth buffer */
    depth_buffer = (float* )malloc(window_width * window_height * sizeof(float));
    if(depth_buffer == NULL){
        fprintf(stderr, "Unable to allocate depth buffer, out of memory\n");
        quit();
    }
    /* Allocate array of frames */
    curve_frames = (struct curve_frame*)malloc(sizeof(struct curve_frame)*curve_points_size);
    if(curve_frames == NULL){
        fprintf(stderr, "Unable to allocate curve data\n");
        quit();
    }
    /* Allocate buffer for vertex data */
    points = (float *)malloc(sizeof(float)*curve_points_size*cross_section_size*3);
    if(points == NULL){
        fprintf(stderr, "Unable to allocate surface data\n");
        quit();
    }
    normals = (float *)malloc(sizeof(float)*curve_points_size*cross_section_size*3);
    if(normals == NULL){
        fprintf(stderr, "Unable to allocate surface data\n");
        quit();
    }
    /* Allocate buffer for vertex data */
    index_size = (curve_points_size-1) * cross_section_size * 6;
    index_data = (unsigned int*)malloc( index_size * sizeof(unsigned int));
    if(index_data == NULL){
        fprintf(stderr, "Unable to allocate index buffer\n");
        quit();
    }
    unsigned int raw_size = cross_section_size * 6, i ,j;
    for(i = 0; i < curve_points_size-1; i++){
        for(j = 0; j < cross_section_size; j++){
            unsigned int j_n = (j+1) % cross_section_size;
            //Up Triangle
            index_data[i*raw_size + j*6    ] = i*cross_section_size + j;
            index_data[i*raw_size + j*6 + 1] = (i+1)*cross_section_size + j;
            index_data[i*raw_size + j*6 + 2] = i*cross_section_size + j_n;
            //Down Triangle
            index_data[i*raw_size + j*6 + 3] = i*cross_section_size + j_n;
            index_data[i*raw_size + j*6 + 4] = (i+1)*cross_section_size + j;
            index_data[i*raw_size + j*6 + 5] = (i+1)*cross_section_size + j_n;
        }
    }
    /* Evaluate curve points */
    calculate_curve();
    /* Calculate reference frames along space curve */
    calculate_rmf();
    /* Calculate generalized cylinder */
    calculate_surface();
    /* Eduraster setup */
    if(er_init() != 0) {
        fprintf(stderr, "Unable to init eduraster: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_viewport(0, 0, window_width, window_height);
    /* Set perspective projection */
    er_matrix_mode(ER_PROJECTION);
    er_load_identity();
    er_perspective(60.0f, (float)window_width / (float)window_height, 2.0f, 40.0f);
    er_matrix_mode(ER_MODELVIEW);
    /* Vertex array */
    va = er_create_vertex_array();
    if(va == NULL){
        fprintf(stderr, "Unable to create vertex array: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_use_vertex_array(va);
    er_vertex_pointer(va, 3, 3, points);
    er_normal_pointer(va, 3, normals);
    er_enable_attribute_array(va, ER_VERTEX_ARRAY, ER_TRUE);
    er_enable_attribute_array(va, ER_NORMAL_ARRAY, ER_TRUE);
    /* Program for surface lighting */
    program_surface_light = er_create_program();
    if(program_surface_light == NULL){
        fprintf(stderr, "Unable to create eduraster program: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_varying_attributes(program_surface_light, 6);
    er_load_vertex_shader(program_surface_light, light_vs);
    er_load_homogeneous_division(program_surface_light, light_hd);
    er_load_fragment_shader(program_surface_light, light_fs);
    er_uniformf(program_surface_light, 0, 0.5773503f);
    er_uniformf(program_surface_light, 1, 0.5773503f);
    er_uniformf(program_surface_light, 2, 0.5773503f);
    /* Program for axis */
    program_axis = er_create_program();
    if(program_axis == NULL){
        fprintf(stderr, "Unable to create eduraster program: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_varying_attributes(program_axis, 4);
    er_load_vertex_shader(program_axis, axis_vs);
    er_load_homogeneous_division(program_axis, axis_hd);
    er_load_fragment_shader(program_axis, axis_fs);
    /* Program for lines */
    program_lines = er_create_program();
    if(program_lines == NULL){
        fprintf(stderr, "Unable to create eduraster program: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_varying_attributes(program_lines, 4);
    er_load_vertex_shader(program_lines, axis_vs);
    er_load_homogeneous_division(program_lines, axis_hd);
    er_load_fragment_shader(program_lines, axis_fs);
    /* Program for surface color */
    program_surface_color = er_create_program();
    if(program_surface_color == NULL){
        fprintf(stderr, "Unable to create eduraster program: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_varying_attributes(program_surface_color, 3);
    er_load_vertex_shader(program_surface_color, normal_color_vs);
    er_load_homogeneous_division(program_surface_color, normal_color_hd);
    er_load_fragment_shader(program_surface_color, normal_color_fs);
    /* Default surface program */
    current_surface_program = program_surface_light;
}

static void show_help(){
    printf("\nHelp:\n");
    printf("Press f: Toggle between curve and cylinder\n");
    printf("Press r: Toggle between Frenet frames and Rotation Minimizing frames\n");
    printf("Press w: Enable/Disable wireframe\n");
    printf("Press l: Enable/Disable directional light\n");
    printf("Press h: Show this help\n");
    printf("Press Esc: Quit the program\n");
    printf("\n");
}

static void resize(int width, int height){
    window_width = width;
    window_height = height;
    if(depth_buffer != NULL){
        free(depth_buffer);
    }
    depth_buffer = (float*)malloc(window_width*window_height*sizeof(float));
    if(depth_buffer == NULL){
        fprintf(stderr, "Out of memory\n");
        quit();
    }
    if(color_buffer != NULL){
        free(color_buffer);
    }
    color_buffer = (unsigned int*)malloc(window_width*window_height*sizeof(unsigned int));
    if(color_buffer == NULL){
        fprintf(stderr, "Out of memory\n");
        quit();
    }
    if(texture != NULL){
        SDL_DestroyTexture(texture);
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
    if(texture == NULL){
        fprintf(stderr, "Unable to create texture %s\n", SDL_GetError());
        quit();
    }
    er_viewport(0, 0, window_width, window_height);
    er_matrix_mode(ER_PROJECTION);
    er_load_identity();
    er_perspective(60.0f, (float)window_width / (float)window_height, 2.0f, 40.0f);
    er_matrix_mode(ER_MODELVIEW);
}

static void process_event (SDL_Event* event){

    if(event->type == SDL_MOUSEBUTTONDOWN){
        SDL_CaptureMouse(SDL_TRUE);
    }else if(event->type == SDL_MOUSEBUTTONUP){
        SDL_CaptureMouse(SDL_FALSE);
    }else if(event->type == SDL_MOUSEMOTION){
        if(event->motion.state & SDL_BUTTON_LMASK){
            int diff_y = -event->motion.yrel;
            int diff_x = event->motion.xrel;
            float aspect_ratio = (float)window_width/(float)window_height;
            camera_phi += (float)diff_y / (float)window_height * 90.0f;
            camera_phi = clamp(camera_phi, -90.0f, 90.0f);
            camera_theta += (float)-diff_x / (float)window_width * aspect_ratio * 90.0f;
        }    
    }else if(event->type == SDL_MOUSEWHEEL ){
        camera_offset += -event->wheel.y;
        camera_offset = max(camera_offset, 5.0f);
    }else if(event->type == SDL_KEYDOWN){
        int key = event->key.keysym.sym;
        switch(key){
            case SDLK_w:
                if(wireframe_flag == wireframe){
                    er_polygon_mode(ER_FRONT_AND_BACK, ER_FILL);
                    wireframe_flag = solid;
                }else{
                    er_polygon_mode(ER_FRONT_AND_BACK, ER_LINE);
                    wireframe_flag = wireframe;
                }
                break;
            case SDLK_l:
                if(light_enable == ER_TRUE){
                    current_surface_program = program_surface_color;
                    light_enable = ER_FALSE;
                }else{
                    current_surface_program = program_surface_light;
                    light_enable = ER_TRUE;
                }
                break;
            case SDLK_r:
                if(current_frame == frenet_frame){
                    calculate_rmf();
                    current_frame = rotation_minimizing_frame;
                }else if(current_frame == rotation_minimizing_frame){
                    calculate_fm();
                    current_frame = frenet_frame;
                }
                calculate_surface();
                break;
            case SDLK_f:
                if(show_curve == ER_FALSE){
                    show_curve = ER_TRUE;
                }else{
                    show_curve = ER_FALSE;
                }
                break;
            case SDLK_h:
                show_help();
                break;
            case SDLK_ESCAPE:
                quit();
                break;
        }
    }else if(event->type == SDL_WINDOWEVENT){
        if(event->window.event == SDL_WINDOWEVENT_RESIZED){
            int new_width = event->window.data1;
            int new_height = event->window.data2;
            resize(new_width, new_height);
        }
    }else if(event->type == SDL_QUIT){
        quit();
    }
}

int main(int argc, char * argv[]){
    if(argc < 4){
        fprintf(stderr,"parameters: \"X(t)\" \"Y(t)\" \"Z(t)\"\n");
        exit(1);
    }
    char *error;
    exp_x = parse(argv[1], &var_t, 1, &error);
    if(exp_x == NULL ){
        fprintf(stderr, "Error: %s\n", error);
        free_parsing_data();
        exit(1);
    }
    exp_y = parse(argv[2], &var_t, 1, &error);
    if(exp_y == NULL ){
        fprintf(stderr, "Error: %s\n", error);
        free_parsing_data();
        exit(1);
    }
    exp_z = parse(argv[3], &var_t, 1, &error);
    if(exp_z == NULL ){
        fprintf(stderr, "Error: %s\n", error);
        free_parsing_data();
        exit(1);
    }
    input_equation_parameters();
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        fprintf(stderr, "Unable to initialize SDL, %s\n", SDL_GetError());
        quit();
    }
    window = SDL_CreateWindow("Tubeplot", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_RESIZABLE);
    if(window == NULL){
        fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
        quit();
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if(renderer == NULL){
        fprintf(stderr, "Unable to create renderer for window: %s\n", SDL_GetError());
        quit();
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
    if(texture == NULL){
        fprintf(stderr, "Unable to create texture: %s\n", SDL_GetError());
        quit();
    }
    setup();
    SDL_Event e;
    for(;;){
        /* Handle input events */
        while(SDL_PollEvent(&e)){
            process_event(&e);
        }
        /* Rendering */
        display();
    }
    return 0;
}
