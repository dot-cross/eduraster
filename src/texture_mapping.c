#include "pipeline.h"

#define LOG2_DOT_2 1.386294361
#define LOG2 0.693147181
#define POSITIVE_X 0
#define NEGATIVE_X 1
#define POSITIVE_Y 2
#define NEGATIVE_Y 3
#define POSITIVE_Z 4
#define NEGATIVE_Z 5

typedef struct Cubemap_uv{
    float u, v;
    float u_sign, v_sign;
    int cubemap_face;
    int u_index, v_index, max_axis_index;
    int (*wrap_u)(int, int);
    int (*wrap_v)(int, int);
} Cubemap_uv;

er_PointSpriteEnum point_sprite_coord_origin;
er_Bool point_sprite_enable;

static int repeat(int coord, int dimension){
    return coord & (dimension - 1);
}

static int clamp_to_edge(int coord, int dimension){
    return clamp(coord, 0, dimension - 1);
}

void er_texture_size(er_Texture *tex, int lod, int *dimension){
    tex->texture_size(tex, lod, dimension);
}

void er_texel_fetch(er_Texture *tex, int *coord, int lod, float *color){
    tex->texel_fetch(tex, coord, lod, color);
}

void er_texture_lod(er_Texture *tex, float *coord, float lod, float *color){
    tex->texture_lod(tex, coord, lod, color);
}

void er_texture_grad(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){
    tex->texture_grad(tex, coord, ddx, ddy, color);
}

void er_write_texture(er_Texture *tex, int *coord, int lod, float *color){
    tex->write_texture(tex, coord, lod, color);
}

static void texture1D_size(er_Texture *tex, int lod, int *dimension){
    dimension[0] = tex->mipmaps[lod]->width;
}

static void write_texture1D(er_Texture *tex, int *coord, int lod, float *color){

    float *texels = tex->mipmaps[lod]->texels;
    int components = tex->components;
    int c;
    for(c = 0; c < components; c++){
        texels[coord[VAR_S]*components + c] = color[c];
    }

}

static void texture1D_texel_fetch(er_Texture *tex, int *coord, int lod, float *color){

    float *texels = tex->mipmaps[lod]->texels;
    int components = tex->components;
    int c;
    for(c = 0; c < components; c++){
        color[c] = texels[coord[VAR_S]*components + c];
    }

}

static void sample_tex1D_nearest(float *texels, int width, int components, float u, int (*wrap_u)(int, int), float *color){

    float mu;
    int ru;

    //Map to texel coordinates
    mu = -0.5 + u * width;
    ru = iround(mu);
    ru = wrap_u(ru, width);

    int c;
    for(c = 0; c < components; c++){
        color[c] = texels[ru*components + c];
    }

}

static void sample_tex1D_linear(float *texels, int width, int components, float u, int (*wrap_u)(int, int), float *color){

    float mu;
    int u0, u1;
    float alpha;

    //Map to texel coordinates
    mu = -0.5 + u * width;
    u0 = (int)floor(mu);
    alpha = mu - u0;
    u0 = wrap_u(u0, width);
    u1 = wrap_u(u0+1, width);

    int c;
    for(c = 0; c < components; c++){
        color[c] = lerp( texels[u0*components + c], texels[u1*components + c], alpha);
    }

}

static float calculate_texture1D_lod_level(er_Texture *tex, float *ddx, float *ddy){
    float lod_level;
    int max_dimension = tex->mipmaps[0]->width;
    float max_derivative = max( ddx[VAR_S], ddy[VAR_S]);
    lod_level = log( max_derivative * max_dimension ) / LOG2;
    if(isnan(lod_level)){
        lod_level = tex->lod_max_level;
    }
    return lod_level;
}


static void texture1D_lod_mag_linear_min_linear(er_Texture *tex, float *coord, float lod_level, float *color){
    Mipmap *mip = tex->mipmaps[0];
    sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
}

static void texture1D_lod_mag_nearest_min_nearest(er_Texture *tex, float *coord, float lod_level, float *color){Mipmap *mip = tex->mipmaps[0];
    sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
}

static void texture1D_lod_mag_linear_min_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    Mipmap *mip = tex->mipmaps[0];
    if(lod_level <= 0){
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_lod_mag_nearest_min_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    Mipmap *mip = tex->mipmaps[0];
    if(lod_level <= 0){
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_lod_mag_linear_min_linear_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex1D_linear(lower_mip->texels, lower_mip->width, tex->components, coord[VAR_S], tex->wrap_s, lower_color);
        sample_tex1D_linear(upper_mip->texels, upper_mip->width, tex->components, coord[VAR_S], tex->wrap_s, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_lod_mag_linear_min_nearest_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex1D_nearest(lower_mip->texels, lower_mip->width, tex->components, coord[VAR_S], tex->wrap_s, lower_color);
        sample_tex1D_nearest(upper_mip->texels, upper_mip->width, tex->components, coord[VAR_S], tex->wrap_s, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_lod_mag_linear_min_linear_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_lod_mag_linear_min_nearest_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_lod_mag_nearest_min_linear_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex1D_linear(lower_mip->texels, lower_mip->width, tex->components, coord[VAR_S], tex->wrap_s, lower_color);
        sample_tex1D_linear(upper_mip->texels, upper_mip->width, tex->components, coord[VAR_S], tex->wrap_s, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_lod_mag_nearest_min_nearest_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex1D_nearest(lower_mip->texels, lower_mip->width, tex->components, coord[VAR_S], tex->wrap_s, lower_color);
        sample_tex1D_nearest(upper_mip->texels, upper_mip->width, tex->components, coord[VAR_S], tex->wrap_s, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_lod_mag_nearest_min_linear_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_lod_mag_nearest_min_nearest_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}


static void texture1D_grad_mag_linear_min_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){
    Mipmap *mip = tex->mipmaps[0];
    sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
}

static void texture1D_grad_mag_nearest_min_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){
    Mipmap *mip = tex->mipmaps[0];
    sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
}

static void texture1D_grad_mag_linear_min_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Mipmap *mip = tex->mipmaps[0];
    float lod_level = calculate_texture1D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_grad_mag_nearest_min_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){
  
    Mipmap *mip = tex->mipmaps[0];
    float lod_level = calculate_texture1D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_grad_mag_linear_min_linear_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture1D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex1D_linear(lower_mip->texels, lower_mip->width, tex->components, coord[VAR_S], tex->wrap_s, lower_color);
        sample_tex1D_linear(upper_mip->texels, upper_mip->width, tex->components, coord[VAR_S], tex->wrap_s, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_grad_mag_linear_min_nearest_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture1D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex1D_nearest(lower_mip->texels, lower_mip->width, tex->components, coord[VAR_S], tex->wrap_s, lower_color);
        sample_tex1D_nearest(upper_mip->texels, upper_mip->width, tex->components, coord[VAR_S], tex->wrap_s, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_grad_mag_linear_min_linear_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture1D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_grad_mag_linear_min_nearest_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture1D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_grad_mag_nearest_min_linear_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture1D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex1D_linear(lower_mip->texels, lower_mip->width, tex->components, coord[VAR_S], tex->wrap_s, lower_color);
        sample_tex1D_linear(upper_mip->texels, upper_mip->width, tex->components, coord[VAR_S], tex->wrap_s, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_grad_mag_nearest_min_nearest_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture1D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex1D_nearest(lower_mip->texels, lower_mip->width, tex->components, coord[VAR_S], tex->wrap_s, lower_color);
        sample_tex1D_nearest(upper_mip->texels, upper_mip->width, tex->components, coord[VAR_S], tex->wrap_s, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_grad_mag_nearest_min_linear_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture1D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_linear(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture1D_grad_mag_nearest_min_nearest_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture1D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex1D_nearest(mip->texels, mip->width, tex->components, coord[VAR_S], tex->wrap_s, color);
    }

}

static void texture2D_size(er_Texture *tex, int lod, int* dimension){
    dimension[0] = tex->mipmaps[lod]->width;
    dimension[1] = tex->mipmaps[lod]->height;
}

static void write_texture2D(er_Texture *tex, int *coord, int lod, float *color){

    float *texels = tex->mipmaps[lod]->texels;
    int width = tex->mipmaps[lod]->width;
    int components = tex->components;
    int c;
    for(c = 0; c < components; c++){
        texels[coord[VAR_T]*width*components + coord[VAR_S]*components + c] = color[c];
    }

}

static void texture2D_texel_fetch(er_Texture *tex, int *coord, int lod, float *color){

    float *texels = tex->mipmaps[lod]->texels;
    int width = tex->mipmaps[lod]->width;
    int components = tex->components;
    int c;
    for(c = 0; c < components; c++){
        color[c] = texels[coord[VAR_T]*width*components + coord[VAR_S]*components + c];
    }

}

static void sample_tex2D_nearest(float *texels, int w, int h, int components, float u, float v, int (*wrap_u)(int, int), int (*wrap_v)(int, int), float *color){

    float mu, mv;
    int ru, rv;

    //Map to texel coordinates
    mu = -0.5 + u * w;
    ru = iround(mu);
    ru = wrap_u(ru, w);

    mv = -0.5 + v * h;
    rv = iround(mv);
    rv = wrap_v(rv, h);

    int c;
    for(c = 0; c < components; c++){
        color[c] = texels[rv*w*components + ru*components + c];
    }

}

static void sample_tex2D_bilinear(float *texels, int w, int h, int components, float u, float v, int (*wrap_u)(int, int), int (*wrap_v)(int, int), float *color){

    float mu, mv;
    float value1, value2;
    int u0, u1, v0, v1;
    float alpha, betha;

    //Map to texel coordinates
    mu = -0.5 + u * w;
    u0 = (int)floor(mu);
    alpha = mu - u0;
    u0 = wrap_u(u0, w);
    u1 = wrap_u(u0+1, w);

    mv = -0.5 + v * h;
    v0 = (int)floor(mv);
    betha = mv - v0;
    v0 = wrap_v(v0, h);
    v1 = wrap_v(v0+1, h);

    int c;
    for(c = 0; c < components; c++){
        value1 = lerp( texels[v0*w*components + u0*components + c], texels[v1*w*components + u0*components + c], betha);
        value2 = lerp( texels[v0*w*components + u1*components + c], texels[v1*w*components + u1*components + c], betha);
        color[c] = lerp(value1, value2, alpha);
    }

}

static float calculate_texture2D_lod_level(er_Texture *tex, float *ddx, float *ddy){
    float lod_level;
    int max_dimension = max(tex->mipmaps[0]->width, tex->mipmaps[0]->height);
    float diameter = max( ddx[VAR_S] * ddx[VAR_S] + ddx[VAR_T] * ddx[VAR_T], ddy[VAR_S] * ddy[VAR_S] + ddy[VAR_T] * ddy[VAR_T] );
    lod_level = log( diameter * max_dimension * max_dimension ) / LOG2_DOT_2;
    if(isnan(lod_level)){
        lod_level = tex->lod_max_level;
    }
    return lod_level;
}

static void texture2D_lod_mag_linear_min_linear(er_Texture *tex, float *coord, float lod_level, float *color){
    Mipmap *mip = tex->mipmaps[0];
    sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
}

static void texture2D_lod_mag_nearest_min_nearest(er_Texture *tex, float *coord, float lod_level, float *color){
    Mipmap *mip = tex->mipmaps[0];
    sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
}

static void texture2D_lod_mag_linear_min_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    Mipmap *mip = tex->mipmaps[0];
    if(lod_level <= 0){
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_lod_mag_nearest_min_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    Mipmap *mip = tex->mipmaps[0];
    if(lod_level <= 0){
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_lod_mag_linear_min_linear_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex2D_bilinear(lower_mip->texels, lower_mip->width, lower_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, lower_color);
        sample_tex2D_bilinear(upper_mip->texels, upper_mip->width, upper_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_lod_mag_linear_min_nearest_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex2D_nearest(lower_mip->texels, lower_mip->width, lower_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, lower_color);
        sample_tex2D_nearest(upper_mip->texels, upper_mip->width, upper_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_lod_mag_linear_min_linear_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_lod_mag_linear_min_nearest_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_lod_mag_nearest_min_linear_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex2D_bilinear(lower_mip->texels, lower_mip->width, lower_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, lower_color);
        sample_tex2D_bilinear(upper_mip->texels, upper_mip->width, upper_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_lod_mag_nearest_min_nearest_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex2D_nearest(lower_mip->texels, lower_mip->width, lower_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, lower_color);
        sample_tex2D_nearest(upper_mip->texels, upper_mip->width, upper_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_lod_mag_nearest_min_linear_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_lod_mag_nearest_min_nearest_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_grad_mag_linear_min_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){
    Mipmap *mip = tex->mipmaps[0];
    sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
}

static void texture2D_grad_mag_nearest_min_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){
    Mipmap *mip = tex->mipmaps[0];
    sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
}

static void texture2D_grad_mag_linear_min_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture2D_lod_level(tex, ddx, ddy);
    Mipmap *mip = tex->mipmaps[0];
    if(lod_level <= 0){
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_grad_mag_nearest_min_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture2D_lod_level(tex, ddx, ddy);
    Mipmap *mip = tex->mipmaps[0];
    if(lod_level <= 0){
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_grad_mag_linear_min_linear_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture2D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex2D_bilinear(lower_mip->texels, lower_mip->width, lower_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, lower_color);
        sample_tex2D_bilinear(upper_mip->texels, upper_mip->width, upper_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_grad_mag_linear_min_nearest_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture2D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex2D_nearest(lower_mip->texels, lower_mip->width, lower_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, lower_color);
        sample_tex2D_nearest(upper_mip->texels, upper_mip->width, upper_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_grad_mag_linear_min_linear_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture2D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_grad_mag_linear_min_nearest_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture2D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_grad_mag_nearest_min_linear_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture2D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex2D_bilinear(lower_mip->texels, lower_mip->width, lower_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, lower_color);
        sample_tex2D_bilinear(upper_mip->texels, upper_mip->width, upper_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_grad_mag_nearest_min_nearest_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture2D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        sample_tex2D_nearest(lower_mip->texels, lower_mip->width, lower_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, lower_color);
        sample_tex2D_nearest(upper_mip->texels, upper_mip->width, upper_mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_grad_mag_nearest_min_linear_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture2D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_bilinear(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture2D_grad_mag_nearest_min_nearest_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    float lod_level = calculate_texture2D_lod_level(tex, ddx, ddy);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        sample_tex2D_nearest(mip->texels, mip->width, mip->height, tex->components, coord[VAR_S], coord[VAR_T], tex->wrap_s, tex->wrap_t, color);
    }

}

static void texture_cubemap_size(er_Texture *tex, int lod, int *dimension){

    dimension[0] = tex->mipmaps[lod]->width;
    dimension[1] = tex->mipmaps[lod]->height;

}

static void texture_cubemap_texel_fetch(er_Texture *tex, int *coord, int lod, float *color){

    float *texels;
    int cubemap_face = coord[2];
    int width = tex->mipmaps[lod]->width;
    int height = tex->mipmaps[lod]->height;
    int components = tex->components;
    texels = tex->mipmaps[lod]->texels + cubemap_face * width * height * components;

    int c;
    for(c = 0; c < components; c++){
        color[c] = texels[coord[VAR_T]*width*components + coord[VAR_S]*components + c];
    }

}

static void write_texture_cubemap(er_Texture *tex, int *coord, int lod, float *color){

    float *texels;
    int cubemap_face = coord[2];
    int width = tex->mipmaps[lod]->width;
    int height = tex->mipmaps[lod]->height;
    int components = tex->components;
    texels = tex->mipmaps[lod]->texels + cubemap_face * width * height * components;

    int c;
    for(c = 0; c < components; c++){
        texels[coord[VAR_T]*width*components + coord[VAR_S]*components + c] = color[c];
    }

}

static void calculate_cubemap_uv(er_Texture *tex, float* input_vector, Cubemap_uv* output){

    float abs_x, abs_y, abs_z;
    int x_index, y_index, z_index;
    float xu_sign, xv_sign;
    float yu_sign, yv_sign;
    float zu_sign, zv_sign;

    if( input_vector[VAR_S] >= 0.0f ){
        abs_x = input_vector[VAR_S];
        x_index = POSITIVE_X;
        xu_sign = 1.0f;
        xv_sign = 1.0f;
    }else{
        abs_x = - input_vector[VAR_S];
        x_index = NEGATIVE_X;
        xu_sign = -1.0f;
        xv_sign = 1.0f;
    }

    if( input_vector[VAR_T] >= 0.0f ){
        abs_y = input_vector[VAR_T];
        y_index = POSITIVE_Y;
        yu_sign = 1.0f;
        yv_sign = 1.0f;
    }else{
        abs_y = - input_vector[VAR_T];
        y_index = NEGATIVE_Y;
        yu_sign = 1.0f;
        yv_sign = -1.0f;
    }

    if( input_vector[VAR_P] >= 0.0f ){
        abs_z = input_vector[VAR_P];
        z_index = POSITIVE_Z;
        zu_sign = -1.0f;
        zv_sign = 1.0f;
    }else{
        abs_z = - input_vector[VAR_P];
        z_index = NEGATIVE_Z;
        zu_sign = 1.0f;
        zv_sign = 1.0f;
    }

    float max_axis = abs_x;
    output->cubemap_face = x_index;
    output->u_sign = xu_sign;
    output->v_sign = xv_sign;
    output->u_index = VAR_P;
    output->v_index = VAR_T;
    output->max_axis_index = VAR_S;
    output->wrap_u = tex->wrap_r;
    output->wrap_v = tex->wrap_t;

    if( abs_y > max_axis ){
        max_axis = abs_y;
        output->cubemap_face = y_index;
        output->u_sign = yu_sign;
        output->v_sign = yv_sign;
        output->u_index = VAR_S;
        output->v_index = VAR_P;
        output->max_axis_index = VAR_T;
        output->wrap_u = tex->wrap_s;
        output->wrap_v = tex->wrap_r;
    }
    if( abs_z > max_axis ){
        max_axis = abs_z;
        output->cubemap_face = z_index;
        output->u_sign = zu_sign;
        output->v_sign = zv_sign;
        output->u_index = VAR_S;
        output->v_index = VAR_T;
        output->max_axis_index = VAR_P;
        output->wrap_u = tex->wrap_s;
        output->wrap_v = tex->wrap_t;
    }

    output->u = 0.5f * output->u_sign * input_vector[output->u_index] / max_axis + 0.5f;
    output->v = 0.5f * output->v_sign * input_vector[output->v_index] / max_axis + 0.5f;
}

static float calculate_cubemap_lod_level(er_Texture *tex, float* input_vector, float *ddx, float *ddy, Cubemap_uv* output_uv){

    float du_dx, dv_dx, du_dy, dv_dy;
    int ui = output_uv->u_index;
    int vi = output_uv->v_index;
    int mai = output_uv->max_axis_index;
    float one_over_denom = 1.0f / (2.0f * input_vector[mai] * input_vector[mai]);
    du_dx = output_uv->u_sign * (ddx[ui] * input_vector[mai] - input_vector[ui] * ddx[mai]) * one_over_denom;
    du_dy = output_uv->u_sign * (ddy[ui] * input_vector[mai] - input_vector[ui] * ddy[mai]) * one_over_denom;
    dv_dx = output_uv->v_sign * (ddx[vi] * input_vector[mai] - input_vector[vi] * ddx[mai]) * one_over_denom;
    dv_dy = output_uv->v_sign * (ddy[vi] * input_vector[mai] - input_vector[vi] * ddy[mai]) * one_over_denom;
    float lod_level, diameter;
    diameter = max(du_dx * du_dx + dv_dx * dv_dx, du_dy * du_dy + dv_dy * dv_dy);
    lod_level = log( diameter * tex->mipmaps[0]->width * tex->mipmaps[0]->width ) / LOG2_DOT_2;
    if(isnan(lod_level)){
        lod_level = tex->lod_max_level;
    }
    return lod_level;
}

static void texture_cubemap_lod_mag_linear_min_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    int w = tex->mipmaps[0]->width;
    int h = tex->mipmaps[0]->height;
    float *texels = tex->mipmaps[0]->texels + output.cubemap_face * w * h * tex->components;
    sample_tex2D_bilinear(texels, w, h, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);

}

static void texture_cubemap_lod_mag_nearest_min_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    int w = tex->mipmaps[0]->width;
    int h = tex->mipmaps[0]->height;
    float *texels = tex->mipmaps[0]->texels + output.cubemap_face * w * h * tex->components;
    sample_tex2D_nearest(texels, w, h, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);

}

static void texture_cubemap_lod_mag_linear_min_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    Mipmap *mip = tex->mipmaps[0];
    float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
    if(lod_level == 0){
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_lod_mag_nearest_min_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    Mipmap *mip = tex->mipmaps[0];
    float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
    if(lod_level == 0){
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_lod_mag_linear_min_linear_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        float *lower_texels = lower_mip->texels + output.cubemap_face * lower_mip->width * lower_mip->height * tex->components;
        float *upper_texels = upper_mip->texels + output.cubemap_face * upper_mip->width * upper_mip->height * tex->components;
        sample_tex2D_bilinear(lower_texels, lower_mip->width, lower_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, lower_color);
        sample_tex2D_bilinear(upper_texels, upper_mip->width, upper_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_lod_mag_linear_min_nearest_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        float *lower_texels = lower_mip->texels + output.cubemap_face * lower_mip->width * lower_mip->height * tex->components;
        float *upper_texels = upper_mip->texels + output.cubemap_face * upper_mip->width * upper_mip->height * tex->components;
        sample_tex2D_nearest(lower_texels, lower_mip->width, lower_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, lower_color);
        sample_tex2D_nearest(upper_texels, upper_mip->width, upper_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
          color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_lod_mag_linear_min_linear_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_lod_mag_linear_min_nearest_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_lod_mag_nearest_min_linear_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        float *lower_texels = lower_mip->texels + output.cubemap_face * lower_mip->width * lower_mip->height * tex->components;
        float *upper_texels = upper_mip->texels + output.cubemap_face * upper_mip->width * upper_mip->height * tex->components;
        sample_tex2D_bilinear(lower_texels, lower_mip->width, lower_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, lower_color);
        sample_tex2D_bilinear(upper_texels, upper_mip->width, upper_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_lod_mag_nearest_min_nearest_mip_linear(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        float *lower_texels = lower_mip->texels + output.cubemap_face * lower_mip->width * lower_mip->height * tex->components;
        float *upper_texels = upper_mip->texels + output.cubemap_face * upper_mip->width * upper_mip->height * tex->components;
        sample_tex2D_nearest(lower_texels, lower_mip->width, lower_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, lower_color);
        sample_tex2D_nearest(upper_texels, upper_mip->width, upper_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_lod_mag_nearest_min_linear_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_lod_mag_nearest_min_nearest_mip_nearest(er_Texture *tex, float *coord, float lod_level, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_grad_mag_linear_min_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    int w = tex->mipmaps[0]->width;
    int h = tex->mipmaps[0]->height;
    float *texels = tex->mipmaps[0]->texels + output.cubemap_face * w * h * tex->components;
    sample_tex2D_bilinear(texels, w, h, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);

}

static void texture_cubemap_grad_mag_nearest_min_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    int w = tex->mipmaps[0]->width;
    int h = tex->mipmaps[0]->height;
    float *texels = tex->mipmaps[0]->texels + output.cubemap_face * w * h * tex->components;
    sample_tex2D_nearest(texels, w, h, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);

}

static void texture_cubemap_grad_mag_linear_min_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    float lod_level = calculate_cubemap_lod_level(tex, coord, ddx, ddy, &output);
    Mipmap *mip = tex->mipmaps[0];
    float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
    if(lod_level == 0){
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_grad_mag_nearest_min_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    float lod_level = calculate_cubemap_lod_level(tex, coord, ddx, ddy, &output);
    Mipmap *mip = tex->mipmaps[0];
    float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
    if(lod_level == 0){
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_grad_mag_linear_min_linear_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    float lod_level = calculate_cubemap_lod_level(tex, coord, ddx, ddy, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        float *lower_texels = lower_mip->texels + output.cubemap_face * lower_mip->width * lower_mip->height * tex->components;
        float *upper_texels = upper_mip->texels + output.cubemap_face * upper_mip->width * upper_mip->height * tex->components;
        sample_tex2D_bilinear(lower_texels, lower_mip->width, lower_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, lower_color);
        sample_tex2D_bilinear(upper_texels, upper_mip->width, upper_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_grad_mag_linear_min_nearest_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    float lod_level = calculate_cubemap_lod_level(tex, coord, ddx, ddy, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        float *lower_texels = lower_mip->texels + output.cubemap_face * lower_mip->width * lower_mip->height * tex->components;
        float *upper_texels = upper_mip->texels + output.cubemap_face * upper_mip->width * upper_mip->height * tex->components;
        sample_tex2D_nearest(lower_texels, lower_mip->width, lower_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, lower_color);
        sample_tex2D_nearest(upper_texels, upper_mip->width, upper_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_grad_mag_linear_min_linear_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    float lod_level = calculate_cubemap_lod_level(tex, coord, ddx, ddy, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_grad_mag_linear_min_nearest_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    float lod_level = calculate_cubemap_lod_level(tex, coord, ddx, ddy, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_grad_mag_nearest_min_linear_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    float lod_level = calculate_cubemap_lod_level(tex, coord, ddx, ddy, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        float *lower_texels = lower_mip->texels + output.cubemap_face * lower_mip->width * lower_mip->height * tex->components;
        float *upper_texels = upper_mip->texels + output.cubemap_face * upper_mip->width * upper_mip->height * tex->components;
        sample_tex2D_bilinear(lower_texels, lower_mip->width, lower_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, lower_color);
        sample_tex2D_bilinear(upper_texels, upper_mip->width, upper_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_grad_mag_nearest_min_nearest_mip_linear(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    float lod_level = calculate_cubemap_lod_level(tex, coord, ddx, ddy, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int lower_level = (int)lod_level;
        int upper_level = lower_level+1;
        float lod_blend_factor = lod_level - lower_level;
        vec4 lower_color;
        vec4 upper_color;
        Mipmap *lower_mip = tex->mipmaps[lower_level];
        Mipmap *upper_mip = tex->mipmaps[upper_level];
        float *lower_texels = lower_mip->texels + output.cubemap_face * lower_mip->width * lower_mip->height * tex->components;
        float *upper_texels = upper_mip->texels + output.cubemap_face * upper_mip->width * upper_mip->height * tex->components;
        sample_tex2D_nearest(lower_texels, lower_mip->width, lower_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, lower_color);
        sample_tex2D_nearest(upper_texels, upper_mip->width, upper_mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, upper_color);
        int c;
        for(c = 0; c < tex->components; c++){
            color[c] = lerp(lower_color[c], upper_color[c], lod_blend_factor);
        }
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_grad_mag_nearest_min_linear_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    float lod_level = calculate_cubemap_lod_level(tex, coord, ddx, ddy, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_bilinear(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

static void texture_cubemap_grad_mag_nearest_min_nearest_mip_nearest(er_Texture *tex, float *coord, float *ddx, float *ddy, float *color){

    Cubemap_uv output;
    calculate_cubemap_uv(tex, coord, &output);
    float lod_level = calculate_cubemap_lod_level(tex, coord, ddx, ddy, &output);
    if(lod_level <= 0){
        Mipmap *mip = tex->mipmaps[0];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else if(lod_level < tex->lod_max_level){
        int round_level = uiround(lod_level);
        Mipmap *mip = tex->mipmaps[round_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }else{
        Mipmap *mip = tex->mipmaps[tex->lod_max_level];
        float *texels = mip->texels + output.cubemap_face * mip->width * mip->height * tex->components;
        sample_tex2D_nearest(texels, mip->width, mip->height, tex->components, output.u, output.v, output.wrap_u, output.wrap_v, color);
    }

}

er_StatusEnum er_delete_texture(er_Texture *tex){

    if(tex == NULL){
        return ER_NULL_POINTER;
    }
    if(tex->mipmaps != NULL){
        int i;
        for(i = 0; i < tex->mipmap_stack_size; i++){
            if(tex->mipmaps[i] != NULL){
                if(tex->mipmaps[i]->texels != NULL){
                    free(tex->mipmaps[i]->texels);
                }
                free(tex->mipmaps[i]);
            }
        }
        free(tex->mipmaps);
    }
    free(tex);
    return ER_NO_ERROR;
}

er_StatusEnum er_create_texture1D(er_Texture **tex, int width, er_TextureFormatEnum internal_format){

    if(tex == NULL){
        return ER_NULL_POINTER;
    }
    *tex = NULL;

    if( width == 0){
        return ER_INVALID_ARGUMENT;
    }
    if(width & (width-1)){
        return ER_NO_POWER_OF_TWO;
    }
    int components;
    if(internal_format == ER_R32F){
        components = 1;
    }else if(internal_format == ER_RG32F){
        components = 2;
    }else if(internal_format == ER_RGB32F){
        components = 3;
    }else if(internal_format == ER_RGBA32F){
        components = 4;
    }else if(internal_format == ER_DEPTH32F){
        components = 1;
    }else{
        return ER_INVALID_ARGUMENT;
    }

    er_Texture *new_texture = (er_Texture*)malloc(sizeof(er_Texture));
    if(new_texture == NULL){
        return ER_OUT_OF_MEMORY;
    }
    new_texture->texture_target = ER_TEXTURE_1D;
    new_texture->texture_format = internal_format;
    new_texture->components = components;
    new_texture->wrap_s = clamp_to_edge;
    new_texture->magnification_filter = ER_NEAREST;
    new_texture->minification_filter = ER_NEAREST;
    new_texture->texture_size = texture1D_size;
    new_texture->texel_fetch = texture1D_texel_fetch;
    new_texture->texture_lod = texture1D_lod_mag_nearest_min_nearest;
    new_texture->texture_grad = texture1D_grad_mag_nearest_min_nearest;
    new_texture->write_texture = write_texture1D;
    int max_level = (int)(log( (float)width) / log(2.0f));
    int mip_levels = max_level+1;
    new_texture->mipmaps = (Mipmap**)malloc(mip_levels*sizeof(Mipmap*));
    if(new_texture->mipmaps == NULL){
        er_delete_texture(new_texture);
        return ER_OUT_OF_MEMORY;
    }
    new_texture->mipmap_stack_size = mip_levels;
    new_texture->lod_max_level = max_level;
    int j;
    for(j = 0; j < mip_levels; j++){
        new_texture->mipmaps[j] = NULL; 
    }
    Mipmap *mip0 = (Mipmap*)malloc(sizeof(Mipmap));
    if(mip0 == NULL){
        er_delete_texture(new_texture);
        return ER_OUT_OF_MEMORY;
    }
    new_texture->mipmaps[0] = mip0;
    mip0->width = width;
    mip0->texels = (float*)malloc(width * new_texture->components * sizeof(float));
    if(mip0->texels == NULL){
        er_delete_texture(new_texture);
        return ER_OUT_OF_MEMORY;
    }
    *tex = new_texture;

    return ER_NO_ERROR;

}

er_StatusEnum er_create_texture2D(er_Texture** tex, int width, int height, er_TextureFormatEnum internal_format){

    if(tex == NULL){
        return ER_NULL_POINTER;
    }
    *tex = NULL;

    if( width == 0 || height == 0){
        return ER_INVALID_ARGUMENT;
    }
    if(width & (width-1) || height & (height-1)){
        return ER_NO_POWER_OF_TWO;
    }
    int components;
    if(internal_format == ER_R32F){
        components = 1;
    }else if(internal_format == ER_RG32F){
        components = 2;
    }else if(internal_format == ER_RGB32F){
        components = 3;
    }else if(internal_format == ER_RGBA32F){
        components = 4;
    }else if(internal_format == ER_DEPTH32F){
        components = 1;
    }else{
        return ER_INVALID_ARGUMENT;
    }

    er_Texture *new_texture = (er_Texture*)malloc(sizeof(er_Texture));
    if(new_texture == NULL){
        return ER_OUT_OF_MEMORY;
    }
    new_texture->texture_target = ER_TEXTURE_2D;
    new_texture->texture_format = internal_format;
    new_texture->components = components;
    new_texture->wrap_s = clamp_to_edge;
    new_texture->wrap_t = clamp_to_edge;
    new_texture->wrap_r = clamp_to_edge;
    new_texture->magnification_filter = ER_NEAREST;
    new_texture->minification_filter = ER_NEAREST;
    new_texture->texture_size = texture2D_size;
    new_texture->texel_fetch = texture2D_texel_fetch;
    new_texture->texture_lod = texture2D_lod_mag_nearest_min_nearest;
    new_texture->texture_grad = texture2D_grad_mag_nearest_min_nearest;
    new_texture->write_texture = write_texture2D;
    int max_dimension = max(width, height);
    int max_level = (int)(log( (float)max_dimension) / log(2.0f));
    int mip_levels = max_level+1;
    new_texture->mipmaps = (Mipmap**)malloc(mip_levels*sizeof(Mipmap*));
    if(new_texture->mipmaps == NULL){
        er_delete_texture(new_texture);
        return ER_OUT_OF_MEMORY;
    }
    new_texture->mipmap_stack_size = mip_levels;
    new_texture->lod_max_level = max_level;
    int j;
    for(j = 0; j < mip_levels; j++){
        new_texture->mipmaps[j] = NULL; 
    }
    Mipmap *mip0 = (Mipmap*)malloc(sizeof(Mipmap));
    if(mip0 == NULL){
        er_delete_texture(new_texture);
        return ER_OUT_OF_MEMORY;
    }
    new_texture->mipmaps[0] = mip0;
    mip0->width = width;
    mip0->height = height;
    mip0->texels = (float*)malloc(width * height * new_texture->components * sizeof(float));
    if(mip0->texels == NULL){
        er_delete_texture(new_texture);
        return ER_OUT_OF_MEMORY;
    }
    *tex = new_texture;
    
    return ER_NO_ERROR;
    
}

er_StatusEnum er_create_texture_cubemap(er_Texture **tex, int size, er_TextureFormatEnum internal_format){

    if(tex == NULL) {
        return ER_NULL_POINTER;
    }
    *tex = NULL;

    if( size == 0){
        return ER_INVALID_ARGUMENT;
    }
    if(size & (size-1)){
        return ER_NO_POWER_OF_TWO;
    }
    int components;
    if(internal_format == ER_R32F){
        components = 1;
    }else if(internal_format == ER_RG32F){
        components = 2;
    }else if(internal_format == ER_RGB32F){
        components = 3;
    }else if(internal_format == ER_RGBA32F){
        components = 4;
    }else if(internal_format == ER_DEPTH32F){
        components = 1;
    }else{
        return ER_INVALID_ARGUMENT;
    }

    er_Texture *new_texture = (er_Texture*)malloc(sizeof(er_Texture));
    if(new_texture == NULL){
        return ER_OUT_OF_MEMORY;
    }
    new_texture->texture_target = ER_TEXTURE_CUBE_MAP;
    new_texture->texture_format = internal_format;
    new_texture->components = components;
    new_texture->wrap_s = clamp_to_edge;
    new_texture->wrap_t = clamp_to_edge;
    new_texture->wrap_r = clamp_to_edge;
    new_texture->magnification_filter = ER_NEAREST;
    new_texture->minification_filter = ER_NEAREST;
    new_texture->texture_size = texture_cubemap_size;
    new_texture->texel_fetch = texture_cubemap_texel_fetch;
    new_texture->texture_lod = texture_cubemap_lod_mag_nearest_min_nearest;
    new_texture->texture_grad = texture_cubemap_grad_mag_nearest_min_nearest;
    new_texture->write_texture = write_texture_cubemap;
    int max_level = (int)(log( (float)size) / log(2.0f));
    int mip_levels = max_level+1;
    new_texture->mipmaps = (Mipmap**)malloc(mip_levels*sizeof(Mipmap*));
    if(new_texture->mipmaps == NULL){
        er_delete_texture(new_texture);
        return ER_OUT_OF_MEMORY;
    }
    new_texture->mipmap_stack_size = mip_levels;
    new_texture->lod_max_level = max_level;
    int j;
    for(j = 0; j < mip_levels; j++){
        new_texture->mipmaps[j] = NULL; 
    }
    Mipmap *mip0 = (Mipmap*)malloc(sizeof(Mipmap));
    if(mip0 == NULL){
        er_delete_texture(new_texture);
        return ER_OUT_OF_MEMORY;
    }
    new_texture->mipmaps[0] = mip0;
    mip0->width = size;
    mip0->height = size;
    mip0->texels = (float*)malloc(6 * size * size * new_texture->components * sizeof(float));
    if(mip0->texels == NULL){
        er_delete_texture(new_texture);
        return ER_OUT_OF_MEMORY;
    }
    *tex = new_texture;
    return ER_NO_ERROR;

}

er_StatusEnum er_texture_ptr(er_Texture *tex, er_TextureTargetEnum texture_target, int level, float **data){

    if(tex == NULL){
        return ER_NULL_POINTER;
    }
    
    if(data == NULL) {
        return ER_NULL_POINTER;
    }
    *data = NULL;
    
    if(texture_target != ER_TEXTURE_1D && texture_target != ER_TEXTURE_2D && 
        texture_target != ER_TEXTURE_CUBE_MAP_POSITIVE_X && texture_target != ER_TEXTURE_CUBE_MAP_NEGATIVE_X && 
        texture_target != ER_TEXTURE_CUBE_MAP_POSITIVE_Y && texture_target != ER_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
        texture_target != ER_TEXTURE_CUBE_MAP_POSITIVE_Z && texture_target != ER_TEXTURE_CUBE_MAP_NEGATIVE_Z){
        return ER_INVALID_ARGUMENT;
    }
    
    if(tex->texture_target == ER_TEXTURE_1D && texture_target != ER_TEXTURE_1D){
        return ER_INVALID_OPERATION;
    }
    if(tex->texture_target == ER_TEXTURE_2D && texture_target != ER_TEXTURE_2D){
        return ER_INVALID_OPERATION;
    }
    if(tex->texture_target == ER_TEXTURE_CUBE_MAP && texture_target != ER_TEXTURE_CUBE_MAP_POSITIVE_X && texture_target != ER_TEXTURE_CUBE_MAP_NEGATIVE_X && texture_target != ER_TEXTURE_CUBE_MAP_POSITIVE_Y && texture_target != ER_TEXTURE_CUBE_MAP_NEGATIVE_Y && texture_target != ER_TEXTURE_CUBE_MAP_POSITIVE_Z && texture_target != ER_TEXTURE_CUBE_MAP_NEGATIVE_Z ){
        return ER_INVALID_OPERATION;
    }
    
    level = clamp(level, 0, tex->lod_max_level);

    if(texture_target == ER_TEXTURE_1D){
        *data = tex->mipmaps[level]->texels;
    }else if(texture_target == ER_TEXTURE_2D){
        *data = tex->mipmaps[level]->texels;
    }else if(texture_target >= ER_TEXTURE_CUBE_MAP_POSITIVE_X && texture_target <= ER_TEXTURE_CUBE_MAP_NEGATIVE_Z){
        int cubemap_face = texture_target - ER_TEXTURE_CUBE_MAP_POSITIVE_X;
        *data = (tex->mipmaps[level]->texels + cubemap_face * tex->mipmaps[level]->width * tex->mipmaps[level]->width * tex->components);
    }

    return ER_NO_ERROR;

}

static void update_filter_functions(er_Texture *tex){

    if(tex->minification_filter == ER_LINEAR){
        if(tex->magnification_filter == ER_LINEAR){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_linear_min_linear;
                tex->texture_grad = texture1D_grad_mag_linear_min_linear;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_linear_min_linear;
                tex->texture_grad = texture2D_grad_mag_linear_min_linear;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_linear_min_linear;
                tex->texture_grad = texture_cubemap_grad_mag_linear_min_linear;
            }
        }else if(tex->magnification_filter == ER_NEAREST){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_nearest_min_linear;
                tex->texture_grad = texture1D_grad_mag_nearest_min_linear;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_nearest_min_linear;
                tex->texture_grad = texture2D_grad_mag_nearest_min_linear;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_nearest_min_linear;
                tex->texture_grad = texture_cubemap_grad_mag_nearest_min_linear;
            }
        }
    }else if(tex->minification_filter == ER_NEAREST){
        if(tex->magnification_filter == ER_LINEAR){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_linear_min_nearest;
                tex->texture_grad = texture1D_grad_mag_linear_min_nearest;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_linear_min_nearest;
                tex->texture_grad = texture2D_grad_mag_linear_min_nearest;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_linear_min_nearest;
                tex->texture_grad = texture_cubemap_grad_mag_linear_min_nearest;
            }
        }else if(tex->magnification_filter == ER_NEAREST){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_nearest_min_nearest;
                tex->texture_grad = texture1D_grad_mag_nearest_min_nearest;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_nearest_min_nearest;
                tex->texture_grad = texture2D_grad_mag_nearest_min_nearest;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_nearest_min_nearest;
                tex->texture_grad = texture_cubemap_grad_mag_nearest_min_nearest;
            }
        }
    }else if(tex->minification_filter == ER_NEAREST_MIPMAP_NEAREST){
        if(tex->magnification_filter == ER_LINEAR){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_linear_min_nearest_mip_nearest;
                tex->texture_grad = texture1D_grad_mag_linear_min_nearest_mip_nearest;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_linear_min_nearest_mip_nearest;
                tex->texture_grad = texture2D_grad_mag_linear_min_nearest_mip_nearest;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_linear_min_nearest_mip_nearest;
                tex->texture_grad = texture_cubemap_grad_mag_linear_min_nearest_mip_nearest;
            }
        }else if(tex->magnification_filter == ER_NEAREST){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_nearest_min_nearest_mip_nearest;
                tex->texture_grad = texture1D_grad_mag_nearest_min_nearest_mip_nearest;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_nearest_min_nearest_mip_nearest;
                tex->texture_grad = texture2D_grad_mag_nearest_min_nearest_mip_nearest;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_nearest_min_nearest_mip_nearest;
                tex->texture_grad = texture_cubemap_grad_mag_nearest_min_nearest_mip_nearest;
            }
        }
    }else if(tex->minification_filter == ER_NEAREST_MIPMAP_LINEAR){
        if(tex->magnification_filter == ER_LINEAR){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_linear_min_nearest_mip_linear;
                tex->texture_grad = texture1D_grad_mag_linear_min_nearest_mip_linear;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_linear_min_nearest_mip_linear;
                tex->texture_grad = texture2D_grad_mag_linear_min_nearest_mip_linear;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_linear_min_nearest_mip_linear;
                tex->texture_grad = texture_cubemap_grad_mag_linear_min_nearest_mip_linear;
            }
        }else if(tex->magnification_filter == ER_NEAREST){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_nearest_min_nearest_mip_linear;
                tex->texture_grad = texture1D_grad_mag_nearest_min_nearest_mip_linear;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_nearest_min_nearest_mip_linear;
                tex->texture_grad = texture2D_grad_mag_nearest_min_nearest_mip_linear;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_nearest_min_nearest_mip_linear;
                tex->texture_grad = texture_cubemap_grad_mag_nearest_min_nearest_mip_linear;
            }
        }
    }else if(tex->minification_filter == ER_LINEAR_MIPMAP_NEAREST){
        if(tex->magnification_filter == ER_LINEAR){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_linear_min_linear_mip_nearest;
                tex->texture_grad = texture1D_grad_mag_linear_min_linear_mip_nearest;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_linear_min_linear_mip_nearest;
                tex->texture_grad = texture2D_grad_mag_linear_min_linear_mip_nearest;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_linear_min_linear_mip_nearest;
                tex->texture_grad = texture_cubemap_grad_mag_linear_min_linear_mip_nearest;
            }
        }else if(tex->magnification_filter == ER_NEAREST){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_nearest_min_linear_mip_nearest;
                tex->texture_grad = texture1D_grad_mag_nearest_min_linear_mip_nearest;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_nearest_min_linear_mip_nearest;
                tex->texture_grad = texture2D_grad_mag_nearest_min_linear_mip_nearest;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_nearest_min_linear_mip_nearest;
                tex->texture_grad = texture_cubemap_grad_mag_nearest_min_linear_mip_nearest;
            }
        }
    }else if(tex->minification_filter == ER_LINEAR_MIPMAP_LINEAR){
        if(tex->magnification_filter == ER_LINEAR){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_linear_min_linear_mip_linear;
                tex->texture_grad = texture1D_grad_mag_linear_min_linear_mip_linear;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_linear_min_linear_mip_linear;
                tex->texture_grad = texture2D_grad_mag_linear_min_linear_mip_linear;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_linear_min_linear_mip_linear;
                tex->texture_grad = texture_cubemap_grad_mag_linear_min_linear_mip_linear;
            }
        }else if(tex->magnification_filter == ER_NEAREST){
            if(tex->texture_target == ER_TEXTURE_1D){
                tex->texture_lod = texture1D_lod_mag_nearest_min_linear_mip_linear;
                tex->texture_grad = texture1D_grad_mag_nearest_min_linear_mip_linear;
            }else if(tex->texture_target == ER_TEXTURE_2D){
                tex->texture_lod = texture2D_lod_mag_nearest_min_linear_mip_linear;
                tex->texture_grad = texture2D_grad_mag_nearest_min_linear_mip_linear;
            }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
                tex->texture_lod = texture_cubemap_lod_mag_nearest_min_linear_mip_linear;
                tex->texture_grad = texture_cubemap_grad_mag_nearest_min_linear_mip_linear;
            }
        }
    }

}

er_StatusEnum er_texture_filtering(er_Texture *tex, er_TextureParamEnum parameter, er_TextureFilterEnum value){

    if(tex == NULL){
        return ER_NULL_POINTER;
    }

    if(parameter == ER_MINIFICATION_FILTER){
        if(value == ER_LINEAR || value == ER_NEAREST || value == ER_LINEAR_MIPMAP_LINEAR || ER_LINEAR_MIPMAP_NEAREST || ER_NEAREST_MIPMAP_LINEAR || ER_NEAREST_MIPMAP_NEAREST){
            tex->minification_filter = value;
            update_filter_functions(tex);
        }else{
            return ER_INVALID_ARGUMENT;
        }
    }else if(parameter == ER_MAGNIFICATION_FILTER){
        if(value == ER_LINEAR || value == ER_NEAREST){
            tex->magnification_filter = value;
            update_filter_functions(tex);
        }else{
            return ER_INVALID_ARGUMENT;
        }
    }else{
        return ER_INVALID_ARGUMENT;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_texture_wrap_mode(er_Texture *tex, er_TextureParamEnum parameter, er_TextureWrapModeEnum value){

    if(tex == NULL){
        return ER_NULL_POINTER;
    }
    if(parameter == ER_WRAP_S){
        if(value == ER_REPEAT){
            tex->wrap_s = repeat;
        }else if (value == ER_CLAMP_TO_EDGE){
            tex->wrap_s = clamp_to_edge;
        }else{
            return ER_INVALID_ARGUMENT;
        }
    }else if(parameter == ER_WRAP_T){
        if(value == ER_REPEAT){
            tex->wrap_t = repeat;
        }else if (value == ER_CLAMP_TO_EDGE){
            tex->wrap_t = clamp_to_edge;
        }else{
            return ER_INVALID_ARGUMENT;
        }
    }else if(parameter == ER_WRAP_R){
        if(value == ER_REPEAT){
            tex->wrap_r = repeat;
        }else if (value == ER_CLAMP_TO_EDGE){
            tex->wrap_r = clamp_to_edge;
        }else{
            return ER_INVALID_ARGUMENT;
        }
    }else{
        return ER_INVALID_ARGUMENT;
    }
    return ER_NO_ERROR;
}

/*
 * Generate mipmap stack for a texture 1D.
 * Used box filtering.
 */
static er_StatusEnum generate_mipmaps_texture1D(er_Texture *tex){

    int cur_width;
    int compsize = tex->components;
    cur_width = tex->mipmaps[0]->width;
    cur_width = cur_width >> 1;
    int l;
    for(l = 1; l <= tex->lod_max_level; l++){
        if(tex->mipmaps[l] == NULL){
            tex->mipmaps[l] = (Mipmap*)malloc(sizeof(Mipmap));
            if(tex->mipmaps[l] == NULL){
                return ER_OUT_OF_MEMORY;
            }
            tex->mipmaps[l]->texels = (float*)malloc(cur_width * compsize * sizeof(float));
            if(tex->mipmaps[l]->texels == NULL){
                free(tex->mipmaps[l]);
                tex->mipmaps[l] = NULL;
                return ER_OUT_OF_MEMORY;
            }
            tex->mipmaps[l]->width = cur_width;
        }
        float *current = tex->mipmaps[l]->texels;
        float *previous = tex->mipmaps[l-1]->texels;
        int i, ip, c;
        for(i = 0; i < cur_width; i++){
            ip = i << 1;
            for(c = 0; c < compsize; c++){
                current[i*compsize + c] = 0.5f * (previous[ip*compsize + c] + previous[(ip+1)*compsize + c]);
            }
        }
        cur_width = cur_width >> 1;
    }
    return ER_NO_ERROR;
}

/*
 * Generate mipmap stack for a texture 2D.
 * Used box filtering.
 */
static er_StatusEnum generate_mipmaps_texture2D(er_Texture *tex){

    int cur_width, cur_height, prev_width, prev_height;
    int compsize = tex->components;
    cur_width = tex->mipmaps[0]->width;
    cur_height = tex->mipmaps[0]->height;
    prev_width = cur_width;
    if(cur_width > 1){
        cur_width >>= 1;
    }
    prev_height = cur_height;
    if(cur_height > 1){
        cur_height >>= 1;
    }
    int l;
    for(l = 1; l <= tex->lod_max_level; l++){
        if(tex->mipmaps[l] == NULL){
            tex->mipmaps[l] = (Mipmap*)malloc(sizeof(Mipmap));
            if(tex->mipmaps[l] == NULL){
                return ER_OUT_OF_MEMORY;
            }
            tex->mipmaps[l]->texels = (float*)malloc(cur_width * cur_height * compsize * sizeof(float));
            if(tex->mipmaps[l]->texels == NULL){
                free(tex->mipmaps[l]);
                tex->mipmaps[l] = NULL;
                return ER_OUT_OF_MEMORY;
            }
            tex->mipmaps[l]->width = cur_width;
            tex->mipmaps[l]->height = cur_height;
        }
        float *current = tex->mipmaps[l]->texels;
        float *previous = tex->mipmaps[l-1]->texels;
        int i, j, ip, jp, c;
        if(prev_width > 1 && prev_height > 1 ){
            for(i = 0; i < cur_height; i++){
                ip = i << 1;
                for(j = 0; j < cur_width; j++){
                    jp = j << 1;
                    for(c = 0; c < compsize; c++){
                        current[i*cur_width*compsize + j*compsize + c] = 0.25f * ( previous[ip*prev_width*compsize + jp*compsize + c] + previous[ip*prev_width*compsize + (jp+1)*compsize + c] + previous[(ip+1)*prev_width*compsize + jp*compsize + c] +  previous[(ip+1)*prev_width*compsize + (jp+1)*compsize + c] );
                    }
                }
            }
        }else if( prev_width > 1 && prev_height == 1 ){
            for(j = 0; j < cur_width; j++){
                jp = j << 1;
                for(c = 0; c < compsize; c++){
                    current[j*compsize + c] = 0.5f * ( previous[jp*compsize + c] + previous[(jp+1)*compsize + c] );
                }
            }
        }else if( prev_width == 1 && prev_height > 1 ){
            for(i = 0; i < cur_height; i++){
                ip = i << 1;
                for(c = 0; c < compsize; c++){
                    current[i*compsize + c] = 0.5f * ( previous[ip*compsize + c] + previous[(ip+1)*compsize + c] );
                }
            }
        }
        prev_width = cur_width;
        if(cur_width > 1){
            cur_width >>= 1;
        }
        prev_height = cur_height;
        if(cur_height > 1){
            cur_height >>= 1;
        }
    }
    return ER_NO_ERROR;
}

/*
 * Generate mipmap stack for a texture cube map.
 * Used box filtering.
 */
static er_StatusEnum generate_mipmaps_texture_cubemap(er_Texture *tex){

    int cur_dim, prev_dim;
    int compsize = tex->components;
    cur_dim = tex->mipmaps[0]->width;
    prev_dim = cur_dim;
    cur_dim = cur_dim >> 1;
    int l;
    for(l = 1; l <= tex->lod_max_level; l++){
        if(tex->mipmaps[l] == NULL){
            tex->mipmaps[l] = (Mipmap*)malloc(sizeof(Mipmap));
            if(tex->mipmaps[l] == NULL){
                return ER_OUT_OF_MEMORY;
            }
            tex->mipmaps[l]->texels = (float*)malloc( 6 * cur_dim * cur_dim * compsize * sizeof(float));
            if(tex->mipmaps[l]->texels == NULL){
                free(tex->mipmaps[l]);
                tex->mipmaps[l] = NULL;
                return ER_OUT_OF_MEMORY;
            }
            tex->mipmaps[l]->width = cur_dim;
            tex->mipmaps[l]->height = cur_dim;
        }
        int cf, i, j, ip, jp, c;
        for(cf = 0; cf < 6; cf++){
            float *current = (tex->mipmaps[l]->texels + cf * tex->mipmaps[l]->width * tex->mipmaps[l]->width * compsize);
            float *previous = (tex->mipmaps[l-1]->texels + cf * tex->mipmaps[l-1]->width * tex->mipmaps[l-1]->width * compsize);
            for(i = 0; i < cur_dim; i++){
                ip = i << 1;
                for(j = 0; j < cur_dim; j++){
                    jp = j << 1;
                    for(c = 0; c < compsize; c++){
                        current[i*cur_dim*compsize + j*compsize + c] = 0.25f * (previous[ip*prev_dim*compsize + jp*compsize + c] + previous[ip*prev_dim*compsize + (jp+1)*compsize + c] + previous[(ip+1)*prev_dim*compsize + jp*compsize + c] + previous[(ip+1)*prev_dim*compsize + (jp+1)*compsize + c]);
                    }
                }
            }
        }
        prev_dim = cur_dim;
        cur_dim = cur_dim >> 1;
    }
    return ER_NO_ERROR;
}

er_StatusEnum er_generate_mipmaps(er_Texture *tex){

    if(tex == NULL){
        return ER_NULL_POINTER;
    }
    if(tex->texture_target == ER_TEXTURE_1D){
        return generate_mipmaps_texture1D(tex);
    }else if(tex->texture_target == ER_TEXTURE_2D){
        return generate_mipmaps_texture2D(tex);
    }else if(tex->texture_target == ER_TEXTURE_CUBE_MAP){
        return generate_mipmaps_texture_cubemap(tex);
    }
    return ER_NO_ERROR;
}
