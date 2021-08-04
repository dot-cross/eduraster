#include "pipeline.h"

er_Program *current_program;

er_Program* er_create_program(){
  
    er_Program *new_program = (er_Program*)malloc(sizeof(er_Program));
    if(new_program != NULL) {
        new_program->varying_attributes = 0;
        new_program->vertex_shader = NULL;
        new_program->fragment_shader = NULL;
        new_program->homogeneous_division = NULL;
    }
    return new_program;
}

er_StatusEnum er_delete_program(er_Program *p){

    if(p == NULL){
        return ER_NULL_POINTER;
    }
    free(p);
    return ER_NO_ERROR;
}

er_StatusEnum er_use_program(er_Program *p){

    if(p == NULL){
        return ER_NULL_POINTER;
    }
    current_program = p;
    return ER_NO_ERROR;
}

er_StatusEnum er_uniformi(er_Program *p, unsigned int index, int value){
    if(p == NULL){
        return ER_NULL_POINTER;
    }
    p->uniform_integer[index] = value;
    return ER_NO_ERROR;
}

er_StatusEnum er_uniformf(er_Program *p, unsigned int index, float value){
    if(p == NULL){
        return ER_NULL_POINTER;
    }
    p->uniform_float[index] = value;
    return ER_NO_ERROR;
}

er_StatusEnum er_uniform_ptr(er_Program *p, unsigned int index, void* pointer){
    if(p == NULL){
        return ER_NULL_POINTER;
    }
    p->uniform_ptr[index] = pointer;
    return ER_NO_ERROR;
}

er_StatusEnum er_uniform_texture_ptr(er_Program *p, unsigned int index, er_Texture* tex_ptr){
    if(p == NULL){
        return ER_NULL_POINTER;
    }
    if(tex_ptr == NULL){
        return ER_NULL_POINTER;
    }
    p->uniform_texture[index] = tex_ptr;
    return ER_NO_ERROR;
}

er_StatusEnum er_varying_attributes(er_Program *p, int number){
    if(p == NULL){
        return ER_NULL_POINTER;
    }
    p->varying_attributes = number;
    return ER_NO_ERROR;
}

er_StatusEnum er_load_fragment_shader(er_Program *p, void (*fragment_shader)(int, int, er_FragInput*, er_UniVars*)){

    if(p == NULL){
        return ER_NULL_POINTER;
    }
    if(fragment_shader == NULL){
        return ER_NULL_POINTER;
    }
    p->fragment_shader = fragment_shader;
    return ER_NO_ERROR;
}

er_StatusEnum er_load_vertex_shader(er_Program *p, void (*vertex_shader)(er_VertexInput*, er_VertexOutput*, er_UniVars*) ){

    if(p == NULL){
        return ER_NULL_POINTER;
    }
    if(vertex_shader == NULL){
        return ER_NULL_POINTER;
    }
    p->vertex_shader = vertex_shader;
    return ER_NO_ERROR;
}

er_StatusEnum er_load_homogeneous_division(er_Program *p, void (*homogeneous_division)(er_VertexOutput*) ){

    if(p == NULL){
        return ER_NULL_POINTER;
    }
    p->homogeneous_division = homogeneous_division;
    return ER_NO_ERROR;
}
