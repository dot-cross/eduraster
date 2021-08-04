#ifndef __RASTERIZATION__
#define __RASTERIZATION__

void draw_point_sprite(er_VertexOutput *vertex, er_PolygonFaceEnum face);

void draw_point(er_VertexOutput *vertex, er_PolygonFaceEnum face);

void draw_triangle(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_VertexOutput *vertex2, er_PolygonFaceEnum face);

void draw_line(er_VertexOutput *vertex0, er_VertexOutput *vertex1, er_PolygonFaceEnum face);

#endif

