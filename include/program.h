#ifndef __PROGRAM__
#define __PROGRAM__

struct program {
    void (*fragment_shader)(int y, int x, struct fragment_input *input, struct uniform_variables *vars);
    void (*vertex_shader)(struct vertex_input *input, struct vertex_output *output, struct uniform_variables *vars);
    void (*homogeneous_division)(struct vertex_output *vertex);
    int varying_attributes;
    int uniform_integer[32];
    float uniform_float[32];
    void* uniform_ptr[32];
    struct texture* uniform_texture[32];
};

extern struct program* current_program;

#endif
