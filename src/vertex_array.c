#include "pipeline.h"

er_VertexArray *current_vertex_array;

er_VertexArray* er_create_vertex_array(){

    er_VertexArray *new_vertex_array = (struct er_VertexArray*)malloc(sizeof(struct er_VertexArray));
    if(new_vertex_array != NULL) {
        new_vertex_array->vertex.enabled = ER_TRUE;
        new_vertex_array->vertex.pointer = NULL;
        new_vertex_array->normal.enabled = ER_FALSE;
        new_vertex_array->normal.pointer = NULL;
        new_vertex_array->color.enabled = ER_FALSE;
        new_vertex_array->color.pointer = NULL;
        new_vertex_array->tex_coord.enabled = ER_FALSE;
        new_vertex_array->tex_coord.pointer = NULL;
        new_vertex_array->fog_coord.enabled = ER_FALSE;
        new_vertex_array->fog_coord.pointer = NULL;
    }
    return new_vertex_array;

}

er_StatusEnum er_delete_vertex_array(er_VertexArray *va){

    if(va == NULL){
        return ER_NULL_POINTER;
    }
    free(va);
    return ER_NO_ERROR;
}

er_StatusEnum er_use_vertex_array(er_VertexArray *va){

    if(va == NULL){
        return ER_NULL_POINTER;
    }
    current_vertex_array = va;
    return ER_NO_ERROR;
}

er_StatusEnum er_enable_attribute_array(er_VertexArray *va, er_VertexArrayEnum array_enum, er_Bool enable){

    if(va == NULL){
        return ER_NULL_POINTER;
    }
    if(array_enum == ER_VERTEX_ARRAY){
        va->vertex.enabled = enable;
    }else if(array_enum == ER_NORMAL_ARRAY){
        va->normal.enabled = enable;
    }else if(array_enum == ER_COLOR_ARRAY){
        va->color.enabled = enable;
    }else if(array_enum == ER_FOG_COORD_ARRAY){
        va->fog_coord.enabled = enable;
    }else if(array_enum == ER_TEX_COORD_ARRAY){
        va->tex_coord.enabled = enable;
    }else{
        return ER_INVALID_ARGUMENT;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_vertex_pointer(er_VertexArray *va, unsigned int stride, unsigned int components, float *pointer){

    if(va == NULL){
        return ER_NULL_POINTER;
    }
    if(components != 2 && components != 3 && components != 4){
        return ER_INVALID_ARGUMENT;
    }
    if(pointer == NULL){
        return ER_NULL_POINTER;
    }
    va->vertex.pointer = pointer;
    va->vertex.stride = stride;
    va->vertex.components = components;
    return ER_NO_ERROR;
}

er_StatusEnum er_color_pointer(er_VertexArray *va, unsigned int stride, unsigned int components, float *pointer){

    if(va == NULL){
        return ER_NULL_POINTER;
    }
    if(components != 3 && components != 4){
        return ER_INVALID_ARGUMENT;
    }
    if(pointer == NULL){
        return ER_NULL_POINTER;
    }
    va->color.pointer = pointer;
    va->color.stride = stride;
    va->color.components = components;
    return ER_NO_ERROR;
}

er_StatusEnum er_normal_pointer(er_VertexArray* va, unsigned int stride, float* pointer){

    if(va == NULL){
        return ER_NULL_POINTER;
    }
    if(pointer == NULL){
        return ER_NULL_POINTER;
    }
    va->normal.pointer = pointer;
    va->normal.stride = stride;
    return ER_NO_ERROR;
}

er_StatusEnum er_fog_coord_pointer(er_VertexArray *va, unsigned int stride, float *pointer){

    if(va == NULL){
        return ER_NULL_POINTER;
    }
    if(pointer == NULL){
        return ER_NULL_POINTER;
    }
    va->fog_coord.pointer = pointer;
    va->fog_coord.stride = stride;
    return ER_NO_ERROR;
}

er_StatusEnum er_tex_coord_pointer(er_VertexArray *va, unsigned int stride, unsigned int components, float *pointer){

    if(va == NULL){
        return ER_NULL_POINTER;
    }
    if(components == 0 || components > 4){
        return ER_INVALID_ARGUMENT;
    }
    if(pointer == NULL){
        return ER_NULL_POINTER;
    }
    va->tex_coord.pointer = pointer;
    va->tex_coord.stride = stride;
    va->tex_coord.components = components;
    return ER_NO_ERROR;
}

void vertex_assembly(er_VertexArray *vertex_array, er_VertexInput *vertex, unsigned int vertex_index){

    float *vertex_data;
    float *normal_data;
    float *color_data;
    float *fog_coord_data;
    float *tex_coord_data;

    /* Vertex data */
    if(vertex_array->vertex.enabled == ER_TRUE){
        vertex_data = (vertex_array->vertex.pointer + vertex_index * vertex_array->vertex.stride);
        if(vertex_array->vertex.components == 2){
            vertex->position[VAR_X] = vertex_data[VAR_X];
            vertex->position[VAR_Y] = vertex_data[VAR_Y];
            vertex->position[VAR_Z] = 0.0f;
            vertex->position[VAR_W] = 1.0f;
        }else if(vertex_array->vertex.components == 3){
            vertex->position[VAR_X] = vertex_data[VAR_X];
            vertex->position[VAR_Y] = vertex_data[VAR_Y];
            vertex->position[VAR_Z] = vertex_data[VAR_Z];
            vertex->position[VAR_W] = 1.0f;
        }else if(vertex_array->vertex.components == 4){
            vertex->position[VAR_X] = vertex_data[VAR_X];
            vertex->position[VAR_Y] = vertex_data[VAR_Y];
            vertex->position[VAR_Z] = vertex_data[VAR_Z];
            vertex->position[VAR_W] = vertex_data[VAR_W];
        }
    }
    /* Normal data */
    if(vertex_array->normal.enabled == ER_TRUE){
        normal_data = (vertex_array->normal.pointer + vertex_index * vertex_array->normal.stride);
        vertex->normal[VAR_X] = normal_data[VAR_X];
        vertex->normal[VAR_Y] = normal_data[VAR_Y];
        vertex->normal[VAR_Z] = normal_data[VAR_Z];
    }else{
        vertex->normal[VAR_X] = current_normal[VAR_X];
        vertex->normal[VAR_Y] = current_normal[VAR_Y];
        vertex->normal[VAR_Z] = current_normal[VAR_Z];
    }
    /* Color data */
    if(vertex_array->color.enabled == ER_TRUE){
        color_data = (vertex_array->color.pointer + vertex_index * vertex_array->color.stride);
        if(vertex_array->color.components == 3){
        vertex->color[VAR_R] = color_data[VAR_R];
        vertex->color[VAR_G] = color_data[VAR_G];
        vertex->color[VAR_B] = color_data[VAR_B];
        vertex->color[VAR_A] = 1.0f;
        }else if(vertex_array->color.components == 4){
        vertex->color[VAR_R] = color_data[VAR_R];
        vertex->color[VAR_G] = color_data[VAR_G];
        vertex->color[VAR_B] = color_data[VAR_B];
        vertex->color[VAR_A] = color_data[VAR_A];
        }
    }else{
        vertex->color[VAR_R] = current_color[VAR_R];
        vertex->color[VAR_G] = current_color[VAR_G];
        vertex->color[VAR_B] = current_color[VAR_B];
        vertex->color[VAR_A] = current_color[VAR_A];
    }
    /* Fog data */
    if(vertex_array->fog_coord.enabled == ER_TRUE){
        fog_coord_data = (vertex_array->fog_coord.pointer + vertex_index * vertex_array->fog_coord.stride);
        vertex->fog_coord = *fog_coord_data;
    }else{
        vertex->fog_coord = current_fog_coord;
    }
    /* Texture coord data */
    if(vertex_array->tex_coord.enabled == ER_TRUE){
        tex_coord_data = (vertex_array->tex_coord.pointer + vertex_index * vertex_array->tex_coord.stride);
        if(vertex_array->tex_coord.components == 1){
            vertex->tex_coord[VAR_S] = tex_coord_data[VAR_S];
            vertex->tex_coord[VAR_T] = 0.0f;
            vertex->tex_coord[VAR_P] = 0.0f;
            vertex->tex_coord[VAR_Q] = 1.0f;
        }else if(vertex_array->tex_coord.components == 2){
            vertex->tex_coord[VAR_S] = tex_coord_data[VAR_S];
            vertex->tex_coord[VAR_T] = tex_coord_data[VAR_T];
            vertex->tex_coord[VAR_P] = 0.0f;
            vertex->tex_coord[VAR_Q] = 1.0f;
        }else if(vertex_array->tex_coord.components == 3){
            vertex->tex_coord[VAR_S] = tex_coord_data[VAR_S];
            vertex->tex_coord[VAR_T] = tex_coord_data[VAR_T];
            vertex->tex_coord[VAR_P] = tex_coord_data[VAR_P];
            vertex->tex_coord[VAR_Q] = 1.0f;
        }else if(vertex_array->tex_coord.components == 4){
            vertex->tex_coord[VAR_S] = tex_coord_data[VAR_S];
            vertex->tex_coord[VAR_T] = tex_coord_data[VAR_T];
            vertex->tex_coord[VAR_P] = tex_coord_data[VAR_P];
            vertex->tex_coord[VAR_Q] = tex_coord_data[VAR_Q];
        }
    }else{
        vertex->tex_coord[VAR_S] = current_tex_coord[VAR_S];
        vertex->tex_coord[VAR_T] = current_tex_coord[VAR_T];
        vertex->tex_coord[VAR_P] = current_tex_coord[VAR_P];
        vertex->tex_coord[VAR_Q] = current_tex_coord[VAR_Q];
    }

}
