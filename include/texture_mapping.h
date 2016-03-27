#ifndef __TEXTURE_MAPPING__
#define __TEXTURE_MAPPING__

struct mipmap{
    float *texels;
    int width;
    int height;
    int depth;
};

struct texture{
    int texture_target;
    int texture_format;
    int components;
    int (*wrap_s)(int, int);
    int (*wrap_t)(int, int);
    int (*wrap_r)(int, int);
    void (*texture_size)(struct texture *tex, int lod, int *dimension);
    void (*texel_fetch)(struct texture *tex, int *coord, int lod, float *color);
    void (*texture_lod)(struct texture *tex, float *coord, float lod, float *color);
    void (*texture_grad)(struct texture *tex, float *coord, float *ddx, float *ddy, float *color);
    void (*write_texture)(struct texture *tex, int  *coord, int lod, float *color);
    int magnification_filter;
    int minification_filter;
    struct mipmap **mipmaps;
    int mipmap_stack_size;
    int lod_max_level;
};

extern int point_sprite_coord_origin;
extern int point_sprite_enable;

#endif
