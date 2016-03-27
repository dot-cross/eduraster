#ifndef __VERTEX_ARRAY__
#define __VERTEX_ARRAY__

struct attribute_array{
    int stride;
    int components;
    int enabled;
    float *pointer;
};

struct vertex_array{
  struct attribute_array vertex;
  struct attribute_array normal;
  struct attribute_array color;
  struct attribute_array tex_coord;
  struct attribute_array fog_coord;
};

extern struct vertex_array *current_vertex_array;

void vertex_assembly(struct vertex_array* vertex_array, struct vertex_input *vertex, unsigned int vertex_index);

#endif
