#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eduraster.h"
#include "SDL.h"

#define SKYBOX_CUBE_LENGTH 15.0f
#define SAMPLES_S 50
#define SAMPLES_T 50

/* Window size*/
static unsigned int window_width = 640;
static unsigned int window_height = 480;
/* SDL structures */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *screen_texture = NULL;
/* Textures */
static struct texture *texture_px = NULL;
static struct texture *texture_nx = NULL;
static struct texture *texture_py = NULL;
static struct texture *texture_ny = NULL;
static struct texture *texture_pz = NULL;
static struct texture *texture_nz = NULL;
static struct texture *texture_cubemap = NULL;
/* Vertex and index data */
static struct vertex_array* va = NULL;
static unsigned int index_size;
static float *points = NULL;
static unsigned int *index_data = NULL;
/* Buffers */
static unsigned int *color_buffer = NULL;
static float *depth_buffer = NULL;
/* Programs */
static struct program *prog_torus = NULL;
static struct program *prog_skybox = NULL;
/* Scene settings */
enum fill_mode {wireframe = 0, solid = 1};
enum fill_mode wireframe_flag = solid;
/* Camera settings */
static float camera_theta = 30.0f, camera_phi = -20.0f, camera_offset = 7.0f;

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
    depth_buffer[y*window_width + x] = depth;
}

/*
* Free resources and exit program.
*/
static void quit(){
    if(texture_px != NULL){
        er_delete_texture(texture_px);
        texture_px = NULL;
    }
    if(texture_nx != NULL){
        er_delete_texture(texture_nx);
        texture_nx = NULL;
    }
    if(texture_py != NULL){
        er_delete_texture(texture_py);
        texture_py = NULL;
    }
    if(texture_ny != NULL){
        er_delete_texture(texture_ny);
        texture_ny = NULL;
    }
    if(texture_pz != NULL){
        er_delete_texture(texture_pz);
        texture_pz = NULL;
    }
    if(texture_nz != NULL){
        er_delete_texture(texture_nz);
        texture_nz = NULL;
    }
    if(texture_cubemap != NULL){
        er_delete_texture(texture_cubemap);
        texture_cubemap = NULL;
    }
    if(va != NULL){
        er_delete_vertex_array(va);
        va = NULL;
    }
    if(prog_torus != NULL){
        er_delete_program(prog_torus);
        prog_torus = NULL;
    }
    if(prog_skybox != NULL){
        er_delete_program(prog_skybox);
        prog_skybox = NULL;
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
    /* Free SDL structures */
    if(screen_texture != NULL){
        SDL_DestroyTexture(screen_texture);
        screen_texture = NULL;
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
    exit(0);
}

/*
* Draw skybox.
*/
static void draw_skybox(){

    vec3 LBF = {-SKYBOX_CUBE_LENGTH, -SKYBOX_CUBE_LENGTH, SKYBOX_CUBE_LENGTH};
    vec3 RBF = {SKYBOX_CUBE_LENGTH, -SKYBOX_CUBE_LENGTH, SKYBOX_CUBE_LENGTH};
    vec3 RTF = {SKYBOX_CUBE_LENGTH, SKYBOX_CUBE_LENGTH, SKYBOX_CUBE_LENGTH};
    vec3 LTF = {-SKYBOX_CUBE_LENGTH, SKYBOX_CUBE_LENGTH, SKYBOX_CUBE_LENGTH};
    vec3 LBB = {-SKYBOX_CUBE_LENGTH, -SKYBOX_CUBE_LENGTH, -SKYBOX_CUBE_LENGTH};
    vec3 RBB = {SKYBOX_CUBE_LENGTH, -SKYBOX_CUBE_LENGTH, -SKYBOX_CUBE_LENGTH};
    vec3 RTB = {SKYBOX_CUBE_LENGTH, SKYBOX_CUBE_LENGTH, -SKYBOX_CUBE_LENGTH};
    vec3 LTB = {-SKYBOX_CUBE_LENGTH, SKYBOX_CUBE_LENGTH, -SKYBOX_CUBE_LENGTH};

    er_use_program(prog_skybox);
    
    /* Positive X */
    er_uniform_texture_ptr(prog_skybox, 0, texture_px);
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(RBB);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(RBF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTF);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(RBB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTF);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(RTB);
    er_end();
    
    /* Negative X */
    er_uniform_texture_ptr(prog_skybox, 0, texture_nx);
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBF);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(LBB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(LTB);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(LTB);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(LTF);
    er_end();
    
    /* Positive Y */
    er_uniform_texture_ptr(prog_skybox, 0, texture_py);
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LTB);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(RTB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTF);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LTB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTF);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(LTF);
    er_end();
    
    /* Negative Y */
    er_uniform_texture_ptr(prog_skybox, 0, texture_ny);
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBF);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(RBF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RBB);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RBB);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(LBB);
    er_end();
    
    /* Positive Z */
    er_uniform_texture_ptr(prog_skybox, 0, texture_pz);
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(RBF);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(LBF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(LTF);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(RBF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(LTF);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(RTF);
    er_end();
    
    /* Negative Z */
    er_uniform_texture_ptr(prog_skybox, 0, texture_nz);
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBB);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(RBB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTB);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTB);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(LTB);
    er_end();
}

/*
 * Calculates points and normals for torus
*/
static void calculate_torus(float* data, unsigned int samples_s, unsigned int samples_t){

    int i = 0, j = 0;
    float var_s, var_t;
    for(i = 0; i < samples_s; i++){
        var_s = ((float)i / (float)(samples_s-1));
        for(j = 0; j < samples_t; j++){
            var_t = ((float)j / (float)(samples_t-1));
            /* Calculate points */
            data[i*samples_t*6 + j*6] = ( 2.0f + cos(2 * M_PI * var_s) ) * cos( 2 * M_PI * var_t);
            data[i*samples_t*6 + j*6 + 1] = sin(2 * M_PI * var_s);
            data[i*samples_t*6 + j*6 + 2] = ( 2.0f + cos(2 * M_PI * var_s) ) * sin( 2 * M_PI * var_t);
            /* Calculate normals */
            data[i*samples_t*6 + j*6 + 3] = cos(2 * M_PI * var_s) * cos( 2 * M_PI * var_t);
            data[i*samples_t*6 + j*6 + 4] = sin(2 * M_PI * var_s);
            data[i*samples_t*6 + j*6 + 5] = cos(2 * M_PI * var_s) * sin( 2 * M_PI * var_t);
        }
    }
}

/*
* EduRaster drawing routines.
*/
static void draw(){
    /* Clear framebuffer */
    clear_buffer();
    /* Set viewing transformation */
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
    /* Draw skybox */
    draw_skybox();
    /* Draw torus */
    er_use_program(prog_torus);
    er_draw_elements(ER_TRIANGLES, index_size, index_data);
}

/*
* Draw and update screen.
*/
static void display() {
    draw();
    SDL_UpdateTexture(screen_texture, NULL, color_buffer, window_width*sizeof(unsigned int));
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

/*
* Vertex shader for skybox.
*/
static void skybox_vs(struct vertex_input *input, struct vertex_output *output, struct uniform_variables *vars){
    multd_mat4_vec4(vars->modelview_projection, input->position, output->position);
    output->attributes[0] = input->tex_coord[VAR_S];
    output->attributes[1] = input->tex_coord[VAR_T];
}

/*
* Homogeneous division for skybox.
*/
static void skybox_hd(struct vertex_output *vertex){
    vertex->position[VAR_X] = vertex->position[VAR_X] / vertex->position[VAR_W];
    vertex->position[VAR_Y] = vertex->position[VAR_Y] / vertex->position[VAR_W];
    vertex->position[VAR_Z] = vertex->position[VAR_Z] / vertex->position[VAR_W];
    vertex->attributes[0] = vertex->attributes[0] / vertex->position[VAR_W];
    vertex->attributes[1] = vertex->attributes[1] / vertex->position[VAR_W];
    vertex->position[VAR_W] = 1.0f / vertex->position[VAR_W];
}

/*
* Fragment shader for skybox.
*/
static void skybox_fs(int y, int x, struct fragment_input *input, struct uniform_variables *vars){
    if(input->frag_coord[VAR_Z] >= read_depth(y, x)){
        return;
    }
    struct texture* tex = vars->uniform_texture[0];
    vec2 tex_coord;
    vec4 tex_color;
    tex_coord[VAR_S] = input->attributes[0] / input->frag_coord[VAR_W];
    tex_coord[VAR_T] = input->attributes[1] / input->frag_coord[VAR_W];
    texture_lod(tex, tex_coord, 0, tex_color);
    write_color(y, x, tex_color[0], tex_color[1], tex_color[2], 1.0f);
    write_depth(y, x, input->frag_coord[VAR_Z]);
}

/*
* Vertex shader for torus.
*/
void torus_vs(struct vertex_input *input, struct vertex_output *output, struct uniform_variables *vars){
    multd_mat4_vec4(vars->modelview_projection, input->position, output->position);
    output->attributes[0] = input->normal[VAR_X];
    output->attributes[1] = input->normal[VAR_Y];
    output->attributes[2] = input->normal[VAR_Z];
}

/*
* Homogeneous division for torus.
*/
void torus_hd(struct vertex_output *vertex){
    vertex->position[VAR_X] = vertex->position[VAR_X] / vertex->position[VAR_W];
    vertex->position[VAR_Y] = vertex->position[VAR_Y] / vertex->position[VAR_W];
    vertex->position[VAR_Z] = vertex->position[VAR_Z] / vertex->position[VAR_W];
    vertex->attributes[0] = vertex->attributes[0] / vertex->position[VAR_W];
    vertex->attributes[1] = vertex->attributes[1] / vertex->position[VAR_W];
    vertex->attributes[2] = vertex->attributes[2] / vertex->position[VAR_W];
    vertex->position[VAR_W] = 1.0f / vertex->position[VAR_W];
}

/*
* Fragment shader for torus.
*/
void torus_fs(int y, int x, struct fragment_input *input, struct uniform_variables *vars){
    if(input->frag_coord[VAR_Z] >= read_depth(y, x)){
        return;
    }
    struct texture *tex = vars->uniform_texture[0];
    vec3 tex_coord;
    vec4 tex_color;
    tex_coord[VAR_S] = input->attributes[0];
    tex_coord[VAR_T] = input->attributes[1];
    tex_coord[VAR_P] = input->attributes[2];
    //texture_lod(texture_id, tex_coord, 0.0f, tex_color);
    texture_grad(tex, tex_coord, &(input->ddx[0]), &(input->ddy[0]), tex_color);
    write_color(y, x, tex_color[0], tex_color[1], tex_color[2], 1.0f);
    write_depth(y, x, input->frag_coord[VAR_Z]);
}

/*
* Load image from file.
*/
static SDL_Surface* load_image(const char *image_name){
    SDL_Surface *image = SDL_LoadBMP(image_name);
    if(image == NULL){
        fprintf(stderr, "Unable to load %s. Error: %s\n", image_name, SDL_GetError());
        return NULL;
    }
    if( (image->w & (image->w - 1)) || (image->h & (image->h - 1))){
        fprintf (stderr, "Image %s is invalid. Dimensions must be power of two\n", image_name);
        SDL_FreeSurface(image);
        return NULL;
    }
    if(image->w != image->h){
        fprintf(stderr, "Image %s is invalid. Non-square image\n", image_name);
        SDL_FreeSurface(image);
        return NULL;
    }
    return image;
}

/*
* Convert and copy pixel data from SDL surface to eduraster texture.
*/
static void set_texture_data(unsigned int tex_target, struct texture *tex, SDL_Surface *image){
    SDL_PixelFormat *format = image->format;
    SDL_LockSurface(image);
    unsigned char* src_data = image->pixels;
    float* dst_data = er_texture_ptr(tex, tex_target, 0);
    unsigned int src_color;
    unsigned char r, g, b;
    int i, j;
    for(i = 0; i < image->h; i++){
        for(j = 0; j < image->w; j++){
            memcpy(&src_color, &src_data[(image->h-1-i)*image->pitch+j*format->BytesPerPixel], format->BytesPerPixel);
            SDL_GetRGB(src_color, format, &r, &g, &b);
            dst_data[i*image->w*3+j*3  ] = (float)r / 255.0f;
            dst_data[i*image->w*3+j*3+1] = (float)g / 255.0f;
            dst_data[i*image->w*3+j*3+2] = (float)b / 255.0f;
        }
    }
    SDL_UnlockSurface(image);
}

/*
* Initialization.
*/
static void setup(){
    /* Load cubemap images */
    SDL_Surface *image_px = load_image("textures/cubemap/posx.bmp");
    SDL_Surface *image_nx = load_image("textures/cubemap/negx.bmp");
    SDL_Surface *image_py = load_image("textures/cubemap/posy.bmp");
    SDL_Surface *image_ny = load_image("textures/cubemap/negy.bmp");
    SDL_Surface *image_pz = load_image("textures/cubemap/posz.bmp");
    SDL_Surface *image_nz = load_image("textures/cubemap/negz.bmp");
    if(image_px == NULL || image_nx == NULL ||
        image_py == NULL || image_ny == NULL ||
        image_pz == NULL || image_nz == NULL){
        SDL_FreeSurface(image_px);
        SDL_FreeSurface(image_nx);
        SDL_FreeSurface(image_py);
        SDL_FreeSurface(image_ny);
        SDL_FreeSurface(image_pz);
        SDL_FreeSurface(image_nz);
        quit();
    }
    if(image_px->w != image_nx->w || image_nx->w != image_py->w ||
        image_py->w != image_ny->w || image_pz->w != image_nz->w){
        fprintf(stderr, "Error: The 6 images have not the same size\n");
        SDL_FreeSurface(image_px);
        SDL_FreeSurface(image_nx);
        SDL_FreeSurface(image_py);
        SDL_FreeSurface(image_ny);
        SDL_FreeSurface(image_pz);
        SDL_FreeSurface(image_nz);
        quit(); 
    }
    /* Allocate color buffer */
    color_buffer = (unsigned int*)malloc(window_width * window_height * sizeof(unsigned int));
    if(color_buffer == NULL){
        fprintf(stderr, "Cannot allocate color buffer. Out of memory\n");
        quit();
    }
     /* Allocate depth buffer */
    depth_buffer = (float* )malloc(window_width * window_height * sizeof(float));
    if(depth_buffer == NULL){
        fprintf(stderr, "Cannot allocate depth buffer. Out of memory\n");
        quit();
    }
     /* Allocate vertex data for torus */
    points = (float*)malloc(SAMPLES_S * SAMPLES_T * 6 * sizeof(float));
    if(points == NULL){
        fprintf(stderr, "Cannot allocate vertex buffer. Out of memory\n");
        quit();
    }
    /* Allocate index data for torus */
    index_size = (SAMPLES_S - 1) * (SAMPLES_T - 1) * 6;
    index_data = (unsigned int*)malloc( index_size * sizeof(unsigned int));
    if(index_data == NULL){
        fprintf(stderr, "Cannot allocate index buffer. Out of memory\n");
        quit();
    }
    int width = (SAMPLES_T-1) * 6, i ,j;
    for(i = 0; i < SAMPLES_S - 1; i++){
        for(j = 0; j < SAMPLES_T - 1; j++){
            //Up Triangle
            index_data[i*width+j*6] = i*SAMPLES_T+j;
            index_data[i*width+j*6+1] = (i+1)*SAMPLES_T+j;
            index_data[i*width+j*6+2] = i*SAMPLES_T+(j+1);
            //Down Triangle
            index_data[i*width+j*6+3] = i*SAMPLES_T+(j+1);
            index_data[i*width+j*6+4] = (i+1)*SAMPLES_T+j;
            index_data[i*width+j*6+5] = (i+1)*SAMPLES_T+(j+1);
        }
    }
    /* Init EduRaster */
    if(er_init() != 0) {
        fprintf(stderr, "Unable to init eduraster: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_viewport(0, 0, window_width, window_height);
    er_matrix_mode(ER_PROJECTION);
    er_load_identity();
    er_perspective(60.0f, (float)window_width / (float)window_height, 2.0f, 100.0f);
    er_matrix_mode(ER_MODELVIEW);
    er_load_identity();

    /* Create vertex array for torus */
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
    /* Generate torus */
    calculate_torus(points, SAMPLES_S, SAMPLES_T);

    /* Create textures for skybox and load texture data */
    /* Positive X */
    texture_px = er_create_texture2D(image_px->w, image_px->h, ER_RGB32F);
    if(texture_px == NULL){
        fprintf(stderr, "Unable to create texture: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_texture_filtering(texture_px, ER_MAGNIFICATION_FILTER, ER_LINEAR);
    er_texture_filtering(texture_px, ER_MINIFICATION_FILTER, ER_LINEAR);
    er_texture_wrap_mode(texture_px, ER_WRAP_S, ER_CLAMP_TO_EDGE);
    er_texture_wrap_mode(texture_px, ER_WRAP_T, ER_CLAMP_TO_EDGE);
    set_texture_data(ER_TEXTURE_2D, texture_px, image_px);
    /* Negative X */
    texture_nx = er_create_texture2D(image_nx->w, image_nx->h, ER_RGB32F);
    if(texture_nx == NULL){
        fprintf(stderr, "Unable to create texture: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_texture_filtering(texture_nx, ER_MAGNIFICATION_FILTER, ER_LINEAR);
    er_texture_filtering(texture_nx, ER_MINIFICATION_FILTER, ER_LINEAR);
    er_texture_wrap_mode(texture_nx, ER_WRAP_S, ER_CLAMP_TO_EDGE);
    er_texture_wrap_mode(texture_nx, ER_WRAP_T, ER_CLAMP_TO_EDGE);
    set_texture_data(ER_TEXTURE_2D, texture_nx, image_nx);
    /* Positive Y */
    texture_py = er_create_texture2D(image_py->w, image_py->h, ER_RGB32F);
    if(texture_py == NULL){
        fprintf(stderr, "Unable to create texture: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_texture_filtering(texture_py, ER_MAGNIFICATION_FILTER, ER_LINEAR);
    er_texture_filtering(texture_py, ER_MINIFICATION_FILTER, ER_LINEAR);
    er_texture_wrap_mode(texture_py, ER_WRAP_S, ER_CLAMP_TO_EDGE);
    er_texture_wrap_mode(texture_py, ER_WRAP_T, ER_CLAMP_TO_EDGE);
    set_texture_data(ER_TEXTURE_2D, texture_py, image_py);
    /* Negative Y */
    texture_ny = er_create_texture2D(image_ny->w, image_ny->h, ER_RGB32F);
    if(texture_ny == NULL){
        fprintf(stderr, "Unable to create texture: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_texture_filtering(texture_ny, ER_MAGNIFICATION_FILTER, ER_LINEAR);
    er_texture_filtering(texture_ny, ER_MINIFICATION_FILTER, ER_LINEAR);
    er_texture_wrap_mode(texture_ny, ER_WRAP_S, ER_CLAMP_TO_EDGE);
    er_texture_wrap_mode(texture_ny, ER_WRAP_T, ER_CLAMP_TO_EDGE);
    set_texture_data(ER_TEXTURE_2D, texture_ny, image_ny);
    /* Positive Z */
    texture_pz = er_create_texture2D(image_pz->w, image_pz->h, ER_RGB32F);
    if(texture_pz == NULL){
        fprintf(stderr, "Unable to create texture: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_texture_filtering(texture_pz, ER_MAGNIFICATION_FILTER, ER_LINEAR);
    er_texture_filtering(texture_pz, ER_MINIFICATION_FILTER, ER_LINEAR);
    er_texture_wrap_mode(texture_pz, ER_WRAP_S, ER_CLAMP_TO_EDGE);
    er_texture_wrap_mode(texture_pz, ER_WRAP_T, ER_CLAMP_TO_EDGE);
    set_texture_data(ER_TEXTURE_2D, texture_pz, image_pz);
    /* Negative Z */
    texture_nz = er_create_texture2D(image_nz->w, image_nz->h, ER_RGB32F);
    if(texture_nz == NULL){
        fprintf(stderr, "Unable to create texture: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_texture_filtering(texture_nz, ER_MAGNIFICATION_FILTER, ER_LINEAR);
    er_texture_filtering(texture_nz, ER_MINIFICATION_FILTER, ER_LINEAR);
    er_texture_wrap_mode(texture_nz, ER_WRAP_S, ER_CLAMP_TO_EDGE);
    er_texture_wrap_mode(texture_nz, ER_WRAP_T, ER_CLAMP_TO_EDGE);
    set_texture_data(ER_TEXTURE_2D, texture_nz, image_nz);
    /* Create texture for environment map */
    texture_cubemap = er_create_texture_cubemap(image_px->w, ER_RGB32F);
    if(texture_cubemap == NULL){
        fprintf(stderr, "Unable to create texture: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_texture_filtering(texture_cubemap, ER_MAGNIFICATION_FILTER, ER_LINEAR);
    er_texture_filtering(texture_cubemap, ER_MINIFICATION_FILTER, ER_LINEAR);
    er_texture_wrap_mode(texture_cubemap, ER_WRAP_S, ER_CLAMP_TO_EDGE);
    er_texture_wrap_mode(texture_cubemap, ER_WRAP_T, ER_CLAMP_TO_EDGE);
    er_texture_wrap_mode(texture_cubemap, ER_WRAP_R, ER_CLAMP_TO_EDGE);
    
    set_texture_data(ER_TEXTURE_CUBE_MAP_POSITIVE_X, texture_cubemap, image_px);
    set_texture_data(ER_TEXTURE_CUBE_MAP_NEGATIVE_X, texture_cubemap, image_nx);
    set_texture_data(ER_TEXTURE_CUBE_MAP_POSITIVE_Y, texture_cubemap, image_py);
    set_texture_data(ER_TEXTURE_CUBE_MAP_NEGATIVE_Y, texture_cubemap, image_ny);
    set_texture_data(ER_TEXTURE_CUBE_MAP_POSITIVE_Z, texture_cubemap, image_pz);
    set_texture_data(ER_TEXTURE_CUBE_MAP_NEGATIVE_Z, texture_cubemap, image_nz);
    er_generate_mipmaps(texture_cubemap);
    int error = er_get_error();
    if(error != ER_NO_ERROR){
        fprintf(stderr, "%s\n",er_get_error_string(error));
        quit();
    }
    SDL_FreeSurface(image_px);
    SDL_FreeSurface(image_nx);
    SDL_FreeSurface(image_py);
    SDL_FreeSurface(image_ny);
    SDL_FreeSurface(image_pz);
    SDL_FreeSurface(image_nz);
    /* Create program for torus */
    prog_torus = er_create_program();
    if(prog_torus == NULL){
        fprintf(stderr, "Unable to create eduraster program: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_use_program(prog_torus);
    er_varying_attributes(prog_torus, 3);
    er_load_vertex_shader(prog_torus, torus_vs);
    er_load_homogeneous_division(prog_torus, torus_hd);
    er_load_fragment_shader(prog_torus, torus_fs);
    er_uniform_texture_ptr(prog_torus, 0, texture_cubemap);
    /* Create program for skybox */
    prog_skybox = er_create_program();
    if(prog_skybox == NULL){
        fprintf(stderr, "Unable to create eduraster program: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_use_program(prog_skybox);
    er_varying_attributes(prog_skybox, 2);
    er_load_vertex_shader(prog_skybox, skybox_vs);
    er_load_homogeneous_division(prog_skybox, skybox_hd);
    er_load_fragment_shader(prog_skybox, skybox_fs);

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
    if(screen_texture != NULL){
        SDL_DestroyTexture(screen_texture);
    }
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
    if(screen_texture == NULL){
        fprintf(stderr, "Unable to create texture %s\n", SDL_GetError());
        quit();
    }
    er_viewport(0, 0, window_width, window_height);
    er_matrix_mode(ER_PROJECTION);
    er_load_identity();
    er_perspective(60.0f, (float)window_width / (float)window_height, 2.0f, 100.0f);
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
        camera_offset = max(camera_offset, 4.0f);
    }else if(event->type == SDL_KEYDOWN){
        int key = event->key.keysym.sym;
        switch(key){
            case SDLK_w:
                if(wireframe_flag == solid){
                    wireframe_flag = wireframe;
                    er_polygon_mode(ER_FRONT_AND_BACK, ER_LINE);
                }else{
                    wireframe_flag = solid;
                    er_polygon_mode(ER_FRONT_AND_BACK, ER_FILL);
                }
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

int main(int argc, char *argv[]){
    /* SDL initialization */
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        fprintf(stderr, "Unable to initialize SDL, %s\n", SDL_GetError());
        quit();
    }
    window = SDL_CreateWindow("Environment mapping", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_RESIZABLE);
    if(window == NULL){
        fprintf(stderr, "Unable to create window %s\n", SDL_GetError());
        quit();
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if(renderer == NULL){
        fprintf(stderr, "Unable to create renderer for window, %s\n", SDL_GetError());
        quit();
    }
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
    if(screen_texture == NULL){
        fprintf(stderr, "Unable to create texture, %s\n", SDL_GetError());
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
