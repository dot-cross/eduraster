#ifndef __CLIPPING__
#define __CLIPPING__

// Trivial accept / reject
#define PRIMITIVE_TRIVIALLY_ACCEPTED 0x2
#define PRIMITIVE_VISIBLE 0x1
#define PRIMITIVE_NO_VISIBLE 0x0

int calculate_outcode(struct er_VertexOutput *vertex);

int clip_point(unsigned int input_index);

int clip_line(unsigned int vertex0_index, unsigned int vertex1_index);

int clip_triangle(unsigned int vertex0_index, unsigned int vertex1_index, unsigned int vertex2_index);

#endif
