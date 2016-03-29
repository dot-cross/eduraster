#include <stdio.h>
#include <stdlib.h>
#include "eduraster.h"
#include "SDL.h"
#include "simple_parser.h"

enum fill_mode {wireframe = 0, solid = 1};

/* Window dimensions */
static int window_width = 1024, window_height = 768;
/* Axis drawing constants */
static const float axis_length = 6.0f;
static const float arrow_length = 1.0f;
static const float arrow_radius = 0.3f;
static const float lut_cos[]={1.000000f, 0.951057f, 0.809017f, 0.587785f, 0.309017f, -0.000000f, -0.309017f, -0.587785f, -0.809017f, -0.951057f, -1.000000f, -0.951057f, -0.809017f, -0.587785f, -0.309017f, 0.000000f, 0.309017f, 0.587785f, 0.809017f, 0.951057f};
static const float lut_sin[]={0.000000f, 0.309017f, 0.587785f, 0.809017f, 0.951057f, 1.000000f, 0.951057f, 0.809017f, 0.587785f, 0.309017f, -0.000000f, -0.309017f, -0.587785f, -0.809017f, -0.951056f, -1.000000f, -0.951056f, -0.809017f, -0.587785f, -0.309017f};
static const unsigned int lut_samples = 20;
/* Parse variables */
static struct var parser_vars[] = {
    {"s", 0.0f},
    {"t", 0.0f}
};
/* Surface data */
static struct expression *exp_x = NULL;
static struct expression *exp_y = NULL;
static struct expression *exp_z = NULL;
static float min_t, max_t;
static float min_s, max_s;
static unsigned int samples_s, samples_t;
static struct vertex_array *va = NULL;
static unsigned int index_size;
static unsigned int *index_data = NULL;
static float *points = NULL;
static float *normals = NULL;
/* Programs */
static struct program *program_surface_light = NULL;
static struct program *program_axis = NULL;
static struct program *program_surface_color = NULL;
static struct program *current_surface_program;
/* Scene settings */
static unsigned int light_enable = ER_TRUE;
enum fill_mode wireframe_flag = solid;
/* Camera */
static float camera_theta = 30.0f, camera_phi = -20.0f, camera_offset = 15.0f;
/* SDL Structures */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
/* Color and depth buffer */
static unsigned int *color_buffer = NULL;
static float *depth_buffer = NULL;

/*
* Free resources and exit program.
*/
static void quit(){
    /* Free eduraster resources */
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
    if(va != NULL){
        er_delete_vertex_array(va);
        va = NULL;
    }
    if(color_buffer != NULL){
        free(color_buffer);
        color_buffer = NULL;
    }
    if(depth_buffer != NULL){
        free(depth_buffer);
        depth_buffer = NULL;
    }
    if(points != NULL){
        free(points);
        points = NULL;
    }
    if(index_data != NULL){
        free(index_data);
        index_data = NULL;
    }
    er_quit();
    /* Free parsing structures */
    if(exp_x != NULL){
        free_exp(exp_x);
        exp_x = NULL;
    }
    if(exp_y != NULL){
        free_exp(exp_y);
        exp_y = NULL;
    }
    if(exp_z != NULL){
        free_exp(exp_z);
        exp_z = NULL;
    }
    /* Free SDL structures */
    if(texture != NULL){
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if(renderer != NULL){
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if(window != NULL){
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();
    /* Exit */
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
    color_buffer[inv_y * window_width + x] = 255 < 24 | r << 16 | g << 8 | b;
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
    depth_buffer[y*window_width + x] = depth;
}

/*
* Evaluate surface equations and calculate surface normals.
*/
static void calculate_surface(){
    int i = 0, j = 0;
    float range_s = max_s - min_s;
    float range_t = max_t - min_t;
    struct var *var_s = &parser_vars[0], *var_t = &parser_vars[1];
    var_s->value = 0.0f;
    var_t->value = 0.0f;
    int height = samples_s;
    int width = samples_t;
    float step_s, step_t;
    step_s = range_s / (float)(samples_s-1);
    step_t = range_t / (float)(samples_t-1);
    float dFds[3], dFdt[3];
    float next[3], prev[3], normal[3];
    for(i = 0; i < height;i++){
        for(j = 0; j < width;j++){
            var_s->value = min_s + (float)i * step_s;
            var_t->value = min_t + (float)j * step_t;
            points[i*width*6 + j*6    ] = eval(exp_x);
            points[i*width*6 + j*6 + 1] = eval(exp_y);
            points[i*width*6 + j*6 + 2] = eval(exp_z);
            // DFds
            var_s->value = min_s + (float)(i+1) * step_s;
            next[VAR_X] = eval(exp_x);
            next[VAR_Y] = eval(exp_y);
            next[VAR_Z] = eval(exp_z);
            var_s->value = min_s + (float)(i-1) * step_s;
            prev[VAR_X] = eval(exp_x);
            prev[VAR_Y] = eval(exp_y);
            prev[VAR_Z] = eval(exp_z);
            dFds[VAR_X] = (next[VAR_X] - prev[VAR_X]) / (2.0f * step_s);
            dFds[VAR_Y] = (next[VAR_Y] - prev[VAR_Y]) / (2.0f * step_s);
            dFds[VAR_Z] = (next[VAR_Z] - prev[VAR_Z]) / (2.0f * step_s);
            // DFdt
            var_s->value = min_s + (float)i * step_s;
            var_t->value = min_t + (float)(j+1) * step_t;
            next[VAR_X] = eval(exp_x);
            next[VAR_Y] = eval(exp_y);
            next[VAR_Z] = eval(exp_z);
            var_t->value = min_t + (float)(j-1) * step_t;
            prev[VAR_X] = eval(exp_x);
            prev[VAR_Y] = eval(exp_y);
            prev[VAR_Z] = eval(exp_z);
            dFdt[VAR_X] = (next[VAR_X] - prev[VAR_X]) / (2.0f * step_t);
            dFdt[VAR_Y] = (next[VAR_Y] - prev[VAR_Y]) / (2.0f * step_t);
            dFdt[VAR_Z] = (next[VAR_Z] - prev[VAR_Z]) / (2.0f * step_t);
            // dFds x dFdt
            cross_product(dFds, dFdt, normal);
            normalize_vec3(normal);
            points[i*width*6 + j*6 + 3] = normal[VAR_X];
            points[i*width*6 + j*6 + 4] = normal[VAR_Y];
            points[i*width*6 + j*6 + 5] = normal[VAR_Z];
        }
    }
}

/*
* Draw axis and arrows.
*/
static void draw_axis(){
    int i;
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
    /* Z axis */
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
    er_use_program(current_surface_program);
    er_draw_elements(ER_TRIANGLES, index_size, index_data);
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
* Vertex shader for surface color based on surface normals.
*/
static void vertex_shader_color(struct vertex_input *input, struct vertex_output *output, struct uniform_variables *vars){
    multd_mat4_vec4(vars->modelview_projection, input->position, output->position);
    output->attributes[0] = input->normal[VAR_X];
    output->attributes[1] = input->normal[VAR_Y];
    output->attributes[2] = input->normal[VAR_Z];
}

/*
* Homogeneous division for surface color based on surface normals.
*/
static void homogeneous_division_color(struct vertex_output *vertex){
    vertex->position[VAR_X] = vertex->position[VAR_X] / vertex->position[VAR_W];
    vertex->position[VAR_Y] = vertex->position[VAR_Y] / vertex->position[VAR_W];
    vertex->position[VAR_Z] = vertex->position[VAR_Z] / vertex->position[VAR_W];
    vertex->attributes[0] = vertex->attributes[0] / vertex->position[VAR_W];
    vertex->attributes[1] = vertex->attributes[1] / vertex->position[VAR_W];
    vertex->attributes[2] = vertex->attributes[2] / vertex->position[VAR_W];
    vertex->position[VAR_W] = 1.0f / vertex->position[VAR_W];
}

/*
* Fragment shader for surface color based on surface normals.
*/
static void fragment_shader_color(int y, int x, struct fragment_input *input, struct uniform_variables *vars){
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
static void vertex_shader_axis(struct vertex_input *input, struct vertex_output *output, struct uniform_variables *vars){
    multd_mat4_vec4(vars->modelview_projection, input->position, output->position);
    output->attributes[0] = input->color[VAR_R];
    output->attributes[1] = input->color[VAR_G];
    output->attributes[2] = input->color[VAR_B];
    output->attributes[3] = input->color[VAR_A];
}

/*
* Homogeneous division for axis and arrows.
*/
static void homogeneous_division_axis(struct vertex_output *vertex){
    vertex->position[VAR_X] = vertex->position[VAR_X] / vertex->position[VAR_W];
    vertex->position[VAR_Y] = vertex->position[VAR_Y] / vertex->position[VAR_W];
    vertex->position[VAR_Z] = vertex->position[VAR_Z] / vertex->position[VAR_W];
    vertex->position[VAR_W] = 1.0f / vertex->position[VAR_W];
}

/*
* Fragment shader for axis and arrows.
*/
static void fragment_shader_axis(int y, int x, struct fragment_input *input, struct uniform_variables *vars){
    if(input->frag_coord[VAR_Z] >= read_depth(y, x)){
        return;
    }
    write_color(y, x, input->attributes[0], input->attributes[1], input->attributes[2], 1.0f);
    write_depth(y, x, input->frag_coord[VAR_Z]);
}

/*
* Vertex shader for surface lighting
*/
static void vertex_shader_light(struct vertex_input *input, struct vertex_output *output, struct uniform_variables *vars){
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
static void homogeneous_division_light(struct vertex_output *vertex){
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
static void fragment_shader_light(int y, int x, struct fragment_input *input, struct uniform_variables *vars){

    if(input->frag_coord[VAR_Z] >= read_depth(y, x)){
        return;
    }
    float *light_dir = &(vars->uniform_float[0]);
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

/*
* Reads range and number of samples for parameters s and t.
*/
static void input_equation_parameters(){
    printf("\tEquation parameters:\n\n");
    printf("Parameter S\n");
    do{
        printf("Samples ? ");
        scanf("%d",&samples_s);
        printf("\n");
    }while(samples_s <= 0);
    printf("Min value ? ");
    scanf("%f",&min_s);
    printf("\n");
    do{
        printf("Max value ? ");
        scanf("%f",&max_s);
        printf("\n");
    }while(max_s <= min_s);

    printf("Parameter T\n");
    do{
        printf("Samples ? ");
        scanf("%d",&samples_t);
        printf("\n");
    }while(samples_t <= 0);
    printf("Min value ? ");
    scanf("%f",&min_t);
    printf("\n");
    do{
        printf("Max value ? ");
        scanf("%f",&max_t);
        printf("\n");
    }while(max_t <= min_t);
}

/*
* Initialization of buffers and EduRaster structures.
*/
static void setup(){
    /* Allocate depth buffer */
    depth_buffer = (float*)malloc(window_width*window_height*sizeof(float));
    if(depth_buffer == NULL){
        fprintf(stderr, "Unable to create depth buffer. Out of memory\n");
        quit();
    }
    /* Allocate color buffer */
    color_buffer = (unsigned int*)malloc(window_width*window_height*sizeof(unsigned int));
    if(color_buffer == NULL){
        fprintf(stderr, "Unable to create color buffer. Out of memory\n");
        quit();
    }
    /* Init EduRaster */
    if(er_init() != 0) {
        fprintf(stderr, "Unable to init eduraster: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_viewport(0, 0, window_width, window_height);
    /* Set Perspective Projection */
    er_matrix_mode(ER_PROJECTION);
    er_load_identity();
    er_perspective(60.0f, (float)window_width / (float)window_height, 2.0f, 50.0f);
    er_matrix_mode(ER_MODELVIEW);

    /* Allocate buffer for vertex data */
    points = (float *)malloc(sizeof(float) * 6 * samples_t * samples_s);
    if(points == NULL){
        fprintf(stderr, "Unable to create vertex data for surface. Out of memory\n");
        quit();
    }
    /* Allocate buffer for indices */
    index_size = (samples_s - 1) * (samples_t - 1) * 6;
    index_data = (unsigned int*)malloc( index_size * sizeof(unsigned int));
    if(index_data == NULL){
        fprintf(stderr, "Unable to create indices for surface. Out of memory\n");
        quit();
    }
    /* Calculate indices */
    int width = (samples_t-1) * 6, i ,j;
    for(i = 0; i < samples_s - 1; i++){
        for(j = 0; j < samples_t - 1; j++){
            // Up Triangle
            index_data[i*width + j*6    ] = i * samples_t + j;
            index_data[i*width + j*6 + 1] = (i+1) * samples_t + j;
            index_data[i*width + j*6 + 2] = i * samples_t + (j + 1);
            // Down Triangle
            index_data[i*width + j*6 + 3] = i * samples_t + (j+1);
            index_data[i*width + j*6 + 4] = (i+1) * samples_t + j;
            index_data[i*width + j*6 + 5] = (i+1) * samples_t + (j+1);
        }
    }
    /* Evaluate surface equations */
    calculate_surface();
    /* Create vertex array for surface */
    va = er_create_vertex_array();
    if(va == NULL){
        fprintf(stderr, "Unable to create vertex array: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_use_vertex_array(va);
    er_vertex_pointer(va, 6, 3, points );
    er_normal_pointer(va, 6, &points[3] );
    er_enable_attribute_array(va, ER_VERTEX_ARRAY, ER_TRUE);
    er_enable_attribute_array(va, ER_NORMAL_ARRAY, ER_TRUE);
    /* Create program for surface lighting */
    program_surface_light = er_create_program();
    if(program_surface_light == NULL){
        fprintf(stderr, "Unable to create eduraster program: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_use_program(program_surface_light);
    er_varying_attributes(program_surface_light, 6);
    er_load_fragment_shader(program_surface_light, fragment_shader_light);
    er_load_vertex_shader(program_surface_light, vertex_shader_light);
    er_load_homogeneous_division(program_surface_light, homogeneous_division_light);
    er_uniformf(program_surface_light, 0, 0.5773503f);
    er_uniformf(program_surface_light, 1, 0.5773503f);
    er_uniformf(program_surface_light, 2, 0.5773503f);
    /* Create program for axis */
    program_axis = er_create_program();
    if(program_axis == NULL){
        fprintf(stderr, "Unable to create eduraster program: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_use_program(program_axis);
    er_varying_attributes(program_axis, 4);
    er_load_fragment_shader(program_axis, fragment_shader_axis);
    er_load_vertex_shader(program_axis, vertex_shader_axis);
    er_load_homogeneous_division(program_axis, homogeneous_division_axis);
    /* Create program for surface color */
    program_surface_color = er_create_program();
    if(program_surface_color == NULL){
        fprintf(stderr, "Unable to create eduraster program: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_use_program(program_surface_color);
    er_varying_attributes(program_surface_color, 3);
    er_load_fragment_shader(program_surface_color, fragment_shader_color);
    er_load_vertex_shader(program_surface_color, vertex_shader_color);
    er_load_homogeneous_division(program_surface_color, homogeneous_division_color);
    /* Default program */
    current_surface_program = program_surface_light;
}

/*
* Resizes depth buffer, color buffer and SDL texture. Updates viewport and perspective matrix.
*/
static void resize(int width, int height){
    window_width = width;
    window_height = height;
    if(depth_buffer != NULL){
        free(depth_buffer);
    }
    depth_buffer = (float*)malloc(window_width*window_height*sizeof(float));
    if(depth_buffer == NULL){
        fprintf(stderr, "Unable to create new depth buffer. Out of memory\n");
        quit();
    }
    if(color_buffer != NULL){
        free(color_buffer);
    }
    color_buffer = (unsigned int*)malloc(window_width*window_height*sizeof(unsigned int));
    if(color_buffer == NULL){
        fprintf(stderr, "Unable to create new color buffer. Out of memory\n");
        quit();
    }
    if(texture != NULL){
        SDL_DestroyTexture(texture);
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
    if(texture == NULL){
        fprintf(stderr, "Unable to create new SDL texture %s\n", SDL_GetError());
        quit();
    }
    er_viewport(0, 0, window_width, window_height);
    er_matrix_mode(ER_PROJECTION);
    er_load_identity();
    er_perspective(60.0f, (float)window_width / (float)window_height, 2.0f, 50.0f);
    er_matrix_mode(ER_MODELVIEW);
}

/*
* Process mouse and keyboard events.
*/
static void process_event (SDL_Event *event){
    if(event->type == SDL_MOUSEBUTTONDOWN){
        SDL_CaptureMouse(SDL_TRUE);
    }else if(event->type == SDL_MOUSEBUTTONUP){
        SDL_CaptureMouse(SDL_FALSE);
    }else if(event->type == SDL_MOUSEMOTION){
        int y = window_height -1 - event->motion.y;
        int x = event->motion.x;
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
            case SDLK_ESCAPE:
                quit();
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

int main(int argc, char *argv[]){
    if(argc < 4){
        fprintf(stderr, "parameters: \"X(s,t)\"  \"Y(s,t)\" \"Z(s,t)\"\n");
        exit(1);
    }
    /* Parse input equations */
    char *error;
    exp_x =  parse(argv[1], parser_vars, 2, &error);
    if(exp_x == NULL ){
        fprintf(stderr, "Error: %s\n", error);
        quit();
    }
    exp_y =  parse(argv[2], parser_vars, 2, &error);
    if(exp_y == NULL ){
        fprintf(stderr, "Error: %s\n", error);
        quit();
    }
    exp_z =  parse(argv[3], parser_vars, 2, &error);
    if(exp_z == NULL ){
        fprintf(stderr, "Error: %s\n", error);
        quit();
    }
    /* Enter samples and range for parameters */
    input_equation_parameters();
    /* SDL initialization */
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        fprintf(stderr, "Unable to initialize SDL, %s\n", SDL_GetError());
        quit();
    }
    window = SDL_CreateWindow("Surface plot", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_RESIZABLE);
    if(window == NULL){
        fprintf(stderr, "Unable to create window %s\n", SDL_GetError());
        quit();
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if(renderer == NULL){
        fprintf(stderr, "Unable to create renderer for window %s\n", SDL_GetError());
        quit();
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
    if(texture == NULL){
        fprintf(stderr, "Unable to create texture %s\n", SDL_GetError());
        quit();
    }
    /* Program initialization */
    setup();
    /* Main loop */
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