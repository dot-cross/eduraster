#ifndef __TEXTURE_MAPPING__
#define __TEXTURE_MAPPING__

typedef struct Mipmap{
    float *texels;
    int width;
    int height;
    int depth;
} Mipmap;

struct er_Texture{
    er_TextureTargetEnum texture_target;
    er_TextureFormatEnum texture_format;
    int components;
    int (*wrap_s)(int, int);
    int (*wrap_t)(int, int);
    int (*wrap_r)(int, int);
    void (*texture_size)(er_Texture *tex, int lod, int *dimension);
    void (*texel_fetch)(er_Texture *tex, int *coord, int lod, float *color);
    void (*texture_lod)(er_Texture *tex, float *coord, float lod, float *color);
    void (*texture_grad)(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color);
    void (*write_texture)(er_Texture *tex, int *coord, int lod, float *color);
    er_TextureFilterEnum magnification_filter;
    er_TextureFilterEnum minification_filter;
    Mipmap **mipmaps;
    int mipmap_stack_size;
    int lod_max_level;
};

extern er_PointSpriteEnum point_sprite_coord_origin;
extern er_Bool point_sprite_enable;

#endif
