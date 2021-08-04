#ifndef __PROGRAM__
#define __PROGRAM__

struct er_Program {
    void (*fragment_shader)(int y, int x, struct er_FragInput *input, struct er_UniVars *vars);
    void (*vertex_shader)(struct er_VertexInput *input, struct er_VertexOutput *output, struct er_UniVars *vars);
    void (*homogeneous_division)(struct er_VertexOutput *vertex);
    int varying_attributes;
    int uniform_integer[32];
    float uniform_float[32];
    void* uniform_ptr[32];
    struct er_Texture* uniform_texture[32];
};

extern struct er_Program* current_program;

#endif
