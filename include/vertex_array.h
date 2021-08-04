#ifndef __VERTEX_ARRAY__
#define __VERTEX_ARRAY__

struct attribute_array{
    unsigned int stride;
    unsigned int components;
    er_Bool enabled;
    float *pointer;
};

struct er_VertexArray{
  struct attribute_array vertex;
  struct attribute_array normal;
  struct attribute_array color;
  struct attribute_array tex_coord;
  struct attribute_array fog_coord;
};

extern struct er_VertexArray *current_vertex_array;

void vertex_assembly(struct er_VertexArray* vertex_array, struct er_VertexInput *vertex, unsigned int vertex_index);

#endif
