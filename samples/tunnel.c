#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "eduraster.h"
#include "SDL.h"

/* Window dimensions */
static unsigned int window_width = 640, window_height = 480;
/* Texture */
static er_Texture *tex = NULL;
/* Program */
static er_Program *prog = NULL;
/* Time */
static float var_time = 0.0f;
/* Buffers */
static unsigned int *color_buffer = NULL;
/* SDL Structures */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *screen_texture = NULL;

/*
* Free resources and exit program
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
* Draw 2 triangles in NDC that cover the entire screen.
*/
static void draw(){
    er_uniformf(prog, 0, var_time);
    er_begin(ER_TRIANGLES);
    er_vertex3f(-1.0f, -1.0f, 0.0f);
    er_vertex3f(1.0f, 1.0f, 0.0f);
    er_vertex3f(-1.0f, 1.0f, 0.0f);
    er_vertex3f(-1.0f, -1.0f, 0.0f);
    er_vertex3f(1.0f, -1.0f, 0.0f);
    er_vertex3f(1.0f, 1.0f, 0.0f);
    er_end();
}

/*
* Draw and update screen.
*/
static void display (void) {
    var_time = SDL_GetTicks();
    draw();
    SDL_UpdateTexture(screen_texture, NULL, color_buffer, window_width*sizeof(unsigned int));
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

/*
* Vertex shader for tunnel effect.
*/
static void tunnel_vs(er_VertexInput *input, er_VertexOutput*output, er_UniVars *vars){
    output->position[VAR_X] = input->position[VAR_X];
    output->position[VAR_Y] = input->position[VAR_Y];
    output->position[VAR_Z] = input->position[VAR_Z];
    output->position[VAR_W] = input->position[VAR_W];
}

/*
* Fragment shader for tunnel effect.
*/
static void tunnel_fs(int y, int x, er_FragInput *input, er_UniVars *vars){

    float my, mx, time;
    er_Texture *tex = vars->uniform_texture[0];
    time = vars->uniform_float[0];
    my = -1.0f + 2.0f * (input->frag_coord[VAR_Y] + 0.5)/ vars->height;
    mx = -1.0f + 2.0f * (input->frag_coord[VAR_X] + 0.5)/ vars->width;

    float angle, radius;
    angle = atan2(my, mx);
    if(angle < 0.0f){
        angle += 2.0f*M_PI;
    }
    radius = sqrt(mx*mx+my*my);
    //radius = max(2*fabs(mx),fabs(my));
    vec2 coord;
    coord[VAR_S] = 2.0f * angle / M_PI;
    coord[VAR_T] = 0.5f / radius + 0.0005f*time;

    vec4 tex_color;
    er_texture_lod(tex, coord, 0.0f, tex_color);
    //Intensity
    float in = 1.6f /radius;
    tex_color[VAR_R] = clamp(tex_color[VAR_R] * in, 0.0f, 1.0f);
    tex_color[VAR_G] = clamp(tex_color[VAR_G] * in, 0.0f, 1.0f);
    tex_color[VAR_B] = clamp(tex_color[VAR_B] * in, 0.0f, 1.0f);
    write_color(y, x, tex_color[VAR_R], tex_color[VAR_G], tex_color[VAR_B], 1.0f);

}

/*
 * Fragment shader for tunnel effect. Analytical calculation of derivatives for trilinear filtering.
 */
static void tunnel_grad_fs(int y, int x, er_FragInput *input, er_UniVars *vars){

    float my, mx, time;
    er_Texture *tex = vars->uniform_texture[0];
    time = vars->uniform_float[0];
    my = -1.0f + 2.0f * (input->frag_coord[VAR_Y] + 0.5)/ vars->height;
    mx = -1.0f + 2.0f * (input->frag_coord[VAR_X] + 0.5)/ vars->width;

    float angle, radius;
    angle = atan2(my, mx);
    if(angle < 0.0f){
        angle += 2.0f*M_PI;
    }
    radius = sqrt(mx*mx+my*my);
    vec2 coord;
    coord[VAR_S] = 2.0f * angle / M_PI;
    coord[VAR_T] = 0.5f / radius + 0.0005f*time;

    vec2 ddx, ddy;
    float dmx_dx = 2.0f / vars->width;
    float dmy_dy = 2.0f / vars->height;
    float one_over_mx = 1.0f / mx;
    float one_over_mx_square = one_over_mx * one_over_mx;
    float f = (2.0f/M_PI);
    float d_atan = 1.0f/(1.0f+my*my*one_over_mx_square);
    float one_over_radio_pow_3 = 1.0f/(radius*radius*radius);
    ddx[VAR_S] = -f*d_atan*my*one_over_mx_square*dmx_dx;
    ddx[VAR_T] = -0.5*mx*one_over_radio_pow_3*dmx_dx;

    ddy[VAR_S] = f*d_atan*one_over_mx*dmy_dy;
    ddy[VAR_T] = -0.5*my*one_over_radio_pow_3*dmy_dy;

    vec4 tex_color;
    er_texture_grad(tex, coord, ddx, ddy, tex_color);
    float in = 1.6f /radius;
    tex_color[VAR_R] = clamp(tex_color[VAR_R] * in, 0.0f, 1.0f);
    tex_color[VAR_G] = clamp(tex_color[VAR_G] * in, 0.0f, 1.0f);
    tex_color[VAR_B] = clamp(tex_color[VAR_B] * in, 0.0f, 1.0f);
    write_color(y, x, tex_color[VAR_R], tex_color[VAR_G], tex_color[VAR_B], 1.0f);

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
    /* Init EduRaster */
    if(er_init() != ER_NO_ERROR) {
        fprintf(stderr, "Unable to init eduraster\n");
        quit();
    }
    er_viewport(0, 0, window_width, window_height);
    er_matrix_mode(ER_PROJECTION);
    er_load_identity();
    er_matrix_mode(ER_MODELVIEW);
    er_load_identity();
    /* Load image and create texture */
    const char *image_path = "textures/lava.bmp";
    SDL_Surface *image = SDL_LoadBMP(image_path);
    if(image == NULL){
        fprintf(stderr, "Unable to load image. Error: %s\n", SDL_GetError());
        quit();
    }

    SDL_PixelFormat *format = image->format;
    er_StatusEnum status = er_create_texture2D(&tex, image->w, image->h, ER_RGB32F);
    if(status != ER_NO_ERROR || tex == NULL){
        fprintf(stderr, "Unable to create texture: %s\n", er_status_string(status));
        quit();
    }
    er_texture_filtering(tex, ER_MAGNIFICATION_FILTER, ER_NEAREST);
    er_texture_filtering(tex, ER_MINIFICATION_FILTER, ER_NEAREST);
    er_texture_wrap_mode(tex, ER_WRAP_S, ER_REPEAT);
    er_texture_wrap_mode(tex, ER_WRAP_T, ER_REPEAT);
    
    SDL_LockSurface(image);
    unsigned char *src_data = image->pixels;
    float *dst_data = NULL;
    status = er_texture_ptr(tex, ER_TEXTURE_2D, 0, &dst_data);
    if(status != ER_NO_ERROR || dst_data == NULL){
        fprintf(stderr, "Unable to retrieve pointer to texture pixels: %s\n", er_status_string(status));
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
        fprintf(stderr, "Cound't generate mipmaps\n");
        quit();
    }
    /* Create program for plane deformation */
    prog = er_create_program();
    if(prog == NULL){
        fprintf(stderr, "Unable to create eduraster program\n");
        quit();
    }
    er_use_program(prog);
    er_varying_attributes(prog, 0);
    er_load_vertex_shader(prog, tunnel_vs);
    er_load_homogeneous_division(prog, NULL);
    er_load_fragment_shader(prog, tunnel_fs);
    er_uniform_texture_ptr(prog, 0, tex);
}

/*
* Show help
*/
static void show_help(){
    printf("\nHelp:\n");
    printf("Press t: Trilinear filtering\n");
    printf("Press n: Nearest neighbout\n");
    printf("Press h: Show this help\n");
    printf("Press Esc: Quit the program\n");
    printf("\n");
}

/*
* Process mouse and keyboard events.
*/
static void process_event (SDL_Event *event){
    if(event->type == SDL_KEYDOWN){
        int key = event->key.keysym.sym;
        switch(key) {
            case SDLK_t:
                er_texture_filtering(tex, ER_MAGNIFICATION_FILTER, ER_LINEAR);
                er_texture_filtering(tex, ER_MINIFICATION_FILTER, ER_LINEAR_MIPMAP_LINEAR);
                er_load_fragment_shader(prog, tunnel_grad_fs);
                break;
            case SDLK_n:
                er_texture_filtering(tex, ER_MAGNIFICATION_FILTER, ER_NEAREST);
                er_texture_filtering(tex, ER_MINIFICATION_FILTER, ER_NEAREST);
                er_load_fragment_shader(prog, tunnel_fs);
                break;
            case SDLK_h:
                show_help();
                break;
            case SDLK_ESCAPE:
                quit();
                break;
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
    window = SDL_CreateWindow("Tunnel Effect", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, 0);
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
    /* Program Initialization */
    setup();
    /* Show Help */
    show_help();
    /* Main Loop */
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
