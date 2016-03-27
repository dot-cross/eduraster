#include "pipeline.h"

struct program *current_program;

struct program *er_create_program(){
  
    struct program *new_program = (struct program*)malloc(sizeof(struct program));
    new_program->varying_attributes = 0;
    new_program->vertex_shader = NULL;
    new_program->fragment_shader = NULL;
    new_program->homogeneous_division = NULL;
    return new_program;
}

void er_delete_program(struct program *p){

    if(p == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    free(p);

}

void er_use_program(struct program *p){

    if(p == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    current_program = p;

}

void er_uniformi(struct program *p, int index, int value){
    if(p == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    p->uniform_integer[index] = value;
}

void er_uniformf(struct program *p, int index, float value){
    if(p == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    p->uniform_float[index] = value;
}

void er_uniform_ptr(struct program *p, int index, void* pointer){
    if(p == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    p->uniform_ptr[index] = pointer;
}

void er_uniform_texture_ptr(struct program *p, int index, struct texture* tex_ptr){
    if(p == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    if(tex_ptr == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    p->uniform_texture[index] = tex_ptr;
}

void er_varying_attributes(struct program *p, int number){
    if(p == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    p->varying_attributes = number;
}

void er_load_fragment_shader(struct program *p, void (*fragment_shader)(int, int, struct fragment_input*, struct uniform_variables*)){

    if(p == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    if(fragment_shader == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    p->fragment_shader = fragment_shader;

}

void er_load_vertex_shader(struct program *p, void (*vertex_shader)(struct vertex_input*, struct vertex_output*, struct uniform_variables*) ){

    if(p == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    if(vertex_shader == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    p->vertex_shader = vertex_shader;

}

void er_load_homogeneous_division(struct program *p, void (*homogeneous_division)(struct vertex_output*) ){

    if(p == NULL){
        set_error(ER_NULL_POINTER);
        return;
    }
    p->homogeneous_division = homogeneous_division;

}
