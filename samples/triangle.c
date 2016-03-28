#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "eduraster.h"
#include "SDL.h"

/* window dimensions */
static int window_width = 600, window_height = 600;
/* color buffer */
static unsigned int *color_buffer = NULL;
/* SDL structures */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *screen_texture = NULL;
/* EduRaster program */
static struct program *prog = NULL;

/*
* Free resources and exit program
*/
static void quit(){
    if(color_buffer != NULL){
        free(color_buffer);
        color_buffer = NULL;
    }
    if(prog != NULL){
        er_delete_program(prog);
        prog = NULL;
    }
    er_quit();
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
* Clear color buffer
*/
static void clear_buffer(){
    int i;
    int length = window_width * window_height;
    for(i = 0; i < length; i++){
        color_buffer[i] = 0xff000000;
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
* EduRaster drawing routines.
*/
static void draw(){
    clear_buffer();
    float rot_angle = 20.0f * SDL_GetTicks() * 0.001f;
    er_load_identity();
    er_rotate_z(rot_angle);
    er_begin(ER_TRIANGLES);
    er_color4f(1.0f, 0.0f, 0.0f, 0.0f);
    er_vertex3f( -3.0f, -2.0f, -3.0f);
    er_color4f(0.0f, 0.0f, 1.0f, 0.0f);
    er_vertex3f( 2.5f, -1.0f, -3.0f);
    er_color4f(0.0f, 1.0f, 0.0f, 0.0f);
    er_vertex3f( 0.0f, 3.0f, -3.0f);
    er_end();
}

/*
* Draw and update screen.
*/
static void display (void) {
    draw();
    SDL_UpdateTexture(screen_texture, NULL, color_buffer, window_width * sizeof(int));
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

/*
* Vertex shader for triangle
*/
static void vertex_shader(struct vertex_input* input, struct vertex_output* output, struct uniform_variables* vars){
    vec4 eye_position;
    multd_mat4_vec4(vars->modelview, input->position, eye_position);
    multd_mat4_vec4(vars->projection, eye_position, output->position);
    output->attributes[0] = input->color[VAR_R];
    output->attributes[1] = input->color[VAR_G];
    output->attributes[2] = input->color[VAR_B];
}

/* Fragment shader for triangle */
static void fragment_shader(int y, int x, struct fragment_input* input, struct uniform_variables* vars){
    write_color(y, x, input->attributes[0], input->attributes[1], input->attributes[2], 1.0f);
}

/*
* Process SDL events
*/
static void process_event(SDL_Event *event){
    if(event->type == SDL_QUIT){
        quit();
    }else if(event->type == SDL_KEYDOWN){
        int key = event->key.keysym.sym;
        if(key == SDLK_ESCAPE){
            quit();
        }
    }
}

int main(int argc, char *argv[]){
    /* SDL initialization */
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
        quit();
    }
    SDL_Window *window = SDL_CreateWindow("Single Triangle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, 0);
    if(window == NULL){
        fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
        quit();
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if(renderer == NULL){
        fprintf(stderr, "Unable to create renderer for windows: %s\n", SDL_GetError());
        quit();
    }
    color_buffer = (unsigned int*)malloc(window_width*window_height*sizeof(unsigned int));
    if(color_buffer == NULL){
        fprintf(stderr, "Unable to create color buffer. Out of memory\n");
        quit();
    }
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
    if(screen_texture == NULL){
        fprintf(stderr, "Unable to create texture for renderer %s\n", SDL_GetError());
        quit();
    }
    /* EduRaster initialization */
    if(er_init() != 0){
        fprintf(stderr, "Unable to init eduraster: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_viewport(0, 0, window_width, window_height);
    er_matrix_mode(ER_PROJECTION);
    er_load_identity();
    er_orthographic(-5.0f, 5.0f, -5.0f, 5.0f, 2.0f, 8.0f);
    er_matrix_mode(ER_MODELVIEW);
    prog = er_create_program();
    if(prog == NULL){
        fprintf(stderr, "Unable to create eduraster program: %s\n", er_get_error_string(er_get_error()));
        quit();
    }
    er_use_program(prog);
    er_varying_attributes(prog, 3);
    er_load_vertex_shader(prog, vertex_shader);
    er_load_homogeneous_division(prog, NULL);
    er_load_fragment_shader(prog, fragment_shader);
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
