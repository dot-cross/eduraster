#ifndef __RASTERIZATION__
#define __RASTERIZATION__

void draw_point_sprite(struct vertex_output *vertex, unsigned int face);

void draw_point(struct vertex_output *vertex, unsigned int face);

void draw_triangle(struct vertex_output *vertex0, struct vertex_output *vertex1, struct vertex_output *vertex2, unsigned int face);

void draw_line(struct vertex_output *vertex0, struct vertex_output *vertex1, unsigned int face);

#endif

