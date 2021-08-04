#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "eduraster.h"
#include "SDL.h"

/* Window dimensions */
static unsigned int window_width = 640, window_height = 480;
/* Texture */
static struct er_Texture *tex = NULL;
/* Program */
static struct er_Program *prog = NULL;
/* Buffers */
static unsigned int *color_buffer = NULL;
static float *depth_buffer = NULL;
/* Camera settings */
static float camera_theta = 30.0f, camera_phi = -20.0f, camera_offset = 7.0f;
/* SDL Structures */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *screen_texture = NULL;

/*
* Free resources and exit program.
*/
static void quit(){
    if(tex != NULL){
        er_delete_texture(tex);
        tex = NULL;
    }
    if(prog != NULL){
        er_delete_program(prog);
        prog = NULL;
    }
    er_quit();
    if(color_buffer != NULL){
        free(color_buffer);
        color_buffer = NULL;
    }
    if(depth_buffer != NULL){
        free(depth_buffer);
        depth_buffer = NULL;
    }
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
    int inv_y = window_height- 1 - y;
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
* Draw cube.
*/
static void draw_cube(float size){

    vec3 LBF = {-size, -size, size};
    vec3 RBF = {size, -size, size};
    vec3 RTF = {size, size, size};
    vec3 LTF = {-size, size, size};
    vec3 LBB = {-size, -size, -size};
    vec3 RBB = {size, -size, -size};
    vec3 RTB = {size, size, -size};
    vec3 LTB = {-size, size, -size};
    
    //Positive X
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(RBF);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(RBB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTB);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(RBF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTB);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(RTF);
    er_end();
    
    //Negative X
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBB);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(LBF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(LTF);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(LTF);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(LTB);
    er_end();
    
    //Positive Y
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LTF);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(RTF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTB);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LTF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTB);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(LTB);
    er_end();
    
    //Negative Y   
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBB);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(RBB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RBF);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RBF);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(LBF);
    er_end();
    
    //Positive Z
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBF);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(RBF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTF);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(LBF);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(RTF);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(LTF);
    er_end();
    
    //Negative Z
    er_begin(ER_TRIANGLES);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(RBB);
    er_tex_coord2f(1.0f, 0.0f);
    er_vertex3fv(LBB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(LTB);
    er_tex_coord2f(0.0f, 0.0f);
    er_vertex3fv(RBB);
    er_tex_coord2f(1.0f, 1.0f);
    er_vertex3fv(LTB);
    er_tex_coord2f(0.0f, 1.0f);
    er_vertex3fv(RTB);
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
    draw_cube(1.5f);
}

/*
* Draw and update screen.
*/
static void display (void) {
    draw();
    SDL_UpdateTexture(screen_texture, NULL, color_buffer, window_width*sizeof(unsigned int));
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

/*
* Vertex shader for texture mapped cube.
*/
static void vs_cube(struct er_VertexInput *input, struct er_VertexOutput *output, struct er_UniVars *vars){
    vec4 eye_position;
    multd_mat4_vec4(vars->modelview, input->position, eye_position);
    multd_mat4_vec4(vars->projection, eye_position, output->position);
    output->attributes[0] = input->tex_coord[VAR_S];
    output->attributes[1] = input->tex_coord[VAR_T];
}

/*
* Homogeneous division for texture mapped cube.
*/
static void hd_cube(struct er_VertexOutput *vertex){
    vertex->position[VAR_X] = vertex->position[VAR_X] / vertex->position[VAR_W];
    vertex->position[VAR_Y] = vertex->position[VAR_Y] / vertex->position[VAR_W];
    vertex->position[VAR_Z] = vertex->position[VAR_Z] / vertex->position[VAR_W];
    vertex->attributes[0] = vertex->attributes[0] / vertex->position[VAR_W];
    vertex->attributes[1] = vertex->attributes[1] / vertex->position[VAR_W];
    vertex->position[VAR_W] = 1.0f / vertex->position[VAR_W];
}

/*
* Fragment shader for texture mapped cube.
*/
static void fs_cube(int y, int x, struct er_FragInput *input, struct er_UniVars *vars){

    if(input->frag_coord[VAR_Z] >= read_depth(y, x)){
        return;
    }
    struct er_Texture* tex = vars->uniform_texture[0];
    vec2 tex_coord;
    vec4 tex_color;
    tex_coord[VAR_S] = input->attributes[0] / input->frag_coord[VAR_W];
    tex_coord[VAR_T] = input->attributes[1] / input->frag_coord[VAR_W];
    er_texture_lod(tex, tex_coord, 0.0f, tex_color);

    write_color(y, x, tex_color[0], tex_color[1], tex_color[2], 1.0f);
    write_depth(y, x, input->frag_coord[VAR_Z]);
}

/*
* Fragment shader for texture mapped cube. Calculation of perspective correct derivatives for trilinear filtering.
*/
static void fs_cube_grad(int y, int x, struct er_FragInput *input, struct er_UniVars *vars){
    
    if(input->frag_coord[VAR_Z] >= read_depth(y, x)){
        return;
    }

    struct er_Texture *tex = vars->uniform_texture[0];
    vec2 tex_coord;
    vec4 tex_color;
    tex_coord[VAR_S] = input->attributes[0] / input->frag_coord[VAR_W];
    tex_coord[VAR_T] = input->attributes[1] / input->frag_coord[VAR_W];

    vec2 ddx, ddy;
    float one_over_w2 = 1.0f / (input->frag_coord[VAR_W] * input->frag_coord[VAR_W]);
    ddx[VAR_S] = (input->ddx[0] * input->frag_coord[VAR_W] - input->attributes[0] * input->dw_dx) * one_over_w2;
    ddx[VAR_T] = (input->ddx[1] * input->frag_coord[VAR_W] - input->attributes[1] * input->dw_dx) * one_over_w2;
    ddy[VAR_S] = (input->ddy[0] * input->frag_coord[VAR_W] - input->attributes[0] * input->dw_dy) * one_over_w2;
    ddy[VAR_T] = (input->ddy[1] * input->frag_coord[VAR_W] - input->attributes[1] * input->dw_dy) * one_over_w2;
    er_texture_grad(tex, tex_coord, ddx, ddy, tex_color);

    write_color(y, x, tex_color[0], tex_color[1], tex_color[2], 1.0f);
    write_depth(y, x, input->frag_coord[VAR_Z]);
}

/*
* Initialization of buffers and EduRaster structures.
*/
static void setup(){
    /* Allocate color buffer */
    color_buffer = (unsigned int*)malloc(window_width*window_height*sizeof(unsigned int));
    if(color_buffer == NULL){
        fprintf(stderr, "Unable to allocate color buffer. Out of memory\n");
        quit();
    }
    /* Allocate depth buffer */
    depth_buffer = (float* )malloc(window_width*window_height*sizeof(float));
    if(depth_buffer == NULL){
        fprintf(stderr, "Unable to allocate depth buffer. Out of memory\n");
        quit();
    }
    /* Init EduRaster */
    if(er_init() != 0) {
        fprintf(stderr, "Unable to init eduraster\n");
        quit();
    }
    er_viewport(0 , 0 , window_width, window_height);
    /* Set perspective projection */
    er_matrix_mode(ER_PROJECTION);
    er_load_identity();
    er_perspective(60.0f, (float)window_width / (float)window_height, 2.0f, 40.0f);
    er_matrix_mode(ER_MODELVIEW);
    /* Load image and create texture */
    const char *image_path = "textures/lenna.bmp";
    SDL_Surface *image = SDL_LoadBMP(image_path);
    if(image == NULL){
        fprintf(stderr, "Unable to load image %s. Error: %s\n", SDL_GetError());
        quit();
    }
    er_StatusEnum status = er_create_texture2D(&tex, image->w, image->h, ER_RGB32F);
    if(status != ER_NO_ERROR || tex == NULL){
        fprintf(stderr, "Unable to create texture\n");
        quit();
    }
    er_texture_filtering(tex, ER_MAGNIFICATION_FILTER, ER_LINEAR);
    er_texture_filtering(tex, ER_MINIFICATION_FILTER, ER_LINEAR_MIPMAP_LINEAR);
    er_texture_wrap_mode(tex, ER_WRAP_S, ER_REPEAT);
    er_texture_wrap_mode(tex, ER_WRAP_T, ER_REPEAT);
    
    SDL_PixelFormat *format = image->format;
    SDL_LockSurface(image);
    unsigned char *src_data = image->pixels;
    float *dst_data = NULL;
    status = er_texture_ptr(tex, ER_TEXTURE_2D, 0, &dst_data);
    if(status != ER_NO_ERROR || dst_data == NULL){
        fprintf(stderr, "Unable to retrieve pointer to texture pixels\n");
        quit();
    }
    int i, j;
    unsigned int src_color = 0;
    unsigned char r, g, b;
    for(i = 0; i < image->h;i++){
        for(j = 0; j < image->w; j++){
            memcpy(&src_color, &src_data[(image->h-1-i)*image->pitch+j*format->BytesPerPixel], format->BytesPerPixel);
            SDL_GetRGB(src_color, format, &r, &g, &b);
            dst_data[i*image->w*3+j*3] = (float)r / 255.0f;
            dst_data[i*image->w*3+j*3+1] = (float)g / 255.0f;
            dst_data[i*image->w*3+j*3+2] = (float)b / 255.0f;
        }
    }
    SDL_UnlockSurface(image);
    SDL_FreeSurface(image);
    /* Generate mipmaps */
    if(er_generate_mipmaps(tex) != ER_NO_ERROR){
        fprintf(stderr, "Unable to generate mipmaps\n");
        quit();
    }
    /* Create program for texture */
    prog = er_create_program();
    if(prog == NULL){
        fprintf(stderr, "Unable to create eduraster program\n");
        quit();
    }
    er_use_program(prog);
    er_varying_attributes(prog, 2);
    er_load_vertex_shader(prog, vs_cube);
    er_load_homogeneous_division(prog, hd_cube);
    er_load_fragment_shader(prog, fs_cube);
    er_uniform_texture_ptr(prog, 0, tex);
    /* Enable backface culling */
    er_cull_face(ER_BACK);
    er_enable(ER_CULL_FACE, ER_TRUE);

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
        fprintf(stderr, "Unable to allocate depth buffer. Out of memory\n");
        quit();
    }
    if(color_buffer != NULL){
        free(color_buffer);
    }
    color_buffer = (unsigned int*)malloc(window_width*window_height*sizeof(unsigned int));
    if(color_buffer == NULL){
        fprintf(stderr, "Unable to allocate color buffer. Out of memory\n");
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
    er_perspective(60.0f, (float)window_width / (float)window_height, 2.0f, 40.0f);
    er_matrix_mode(ER_MODELVIEW);
}

/*
* Process mouse and keyboard events.
*/
static void process_event(SDL_Event* event){

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
        if(key == SDLK_ESCAPE){
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

int main(int argc, char * argv[]){
    /* SDL initialization */
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        fprintf(stderr, "Unable to initialize SDL, %s\n", SDL_GetError());
        quit();
    }
    window = SDL_CreateWindow("Textured Cube", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_RESIZABLE);
    if(window == NULL){
        fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
        quit();
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if(renderer == NULL){
        fprintf(stderr, "Unable to create renderer for window: %s\n", SDL_GetError());
        quit();
    }
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
    if(screen_texture == NULL){
        fprintf(stderr, "Unable to create texture for renderer: %s\n", SDL_GetError());
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
};
