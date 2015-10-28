#pragma once

#include <prt/graphics/rendering.h>
//#include <prt/engine/particles.h>

#ifdef __cplusplus
extern "C" {
#endif

/* These vector/matrix/etc. things here are merely
 * convenient data acccessors without any additional
 * functions for working with them. See C++ example,
 * where these are handled with GLM and data passed
 * around with the `value_ptr` interface.
 */
union vector2d {
  float data[2];
  struct {
    float x;
    float y;
  };
};

union vector3d {
  float data[3];
  struct {
    float x;
    float y;
    float z;
  };
};

union vector4d {
  float data[4];
  struct {
    float x;
    float y;
    float z;
    float w;
  };
};

union matrix3 {
  float data[3 * 3];
  struct {
    union vector3d m13;
    union vector3d m23;
    union vector3d m33;
  };
};

union matrix4 {
  float data[4 * 4];
  struct {
    union vector4d m14;
    union vector4d m24;
    union vector4d m34;
    union vector4d m44;
  };
};

union quaternion {
  float data[4];
  union vector4d a;
};

struct spatial2d {
  union vector2d position;
  union vector2d scale;
  float rotation;
};

struct spatial3d {
  union vector3d position;
  union vector3d scale;
  /* Euler angles rotation */
  float rx, ry, rz;
};

typedef struct _Camera {
  union matrix4 view_projection;
  /* TODO: Handle these externally
   * so that I don't have to reimplement half of
   * GLM in C. On the other hand this is really minimal
   * yet sufficient concept, with delegated "beefy" camera
   * interface to the caller.

  union vector3d eye_point;
  union vector3d world_up;
  union vector3d target;
  union vector3d view_direction;
  union quaternion orientation;
  float field_of_view;
  float aspect_ratio;
  float near;
  float far;
  bool invalid : 1;
  bool orthographic : 1;*/
} Camera;

/* mvp = model * view * projection
 * model      - maps from local space to world space
 * view       - maps from world space to camera space
 * projection - maps from camera space to screen space (3d -> 2d)
 */

/* primitives do not have index buffer */
struct primitive2d {
  uint32_t vao;
  uint32_t vbo_positions;
  struct spatial2d spatial;
  union vector2d size;
  union matrix4 model;
  /* spatial has changed -> recompute model matrix */
  bool model_invalid;
};

typedef enum _RenderType {
  RT_NONE = 0,

  RT_SOLID_COLOR = 1,
  RT_TEXTURED_QUAD = 2,
  RT_PARTICLES = 3,

  RT_MAX = RT_PARTICLES
} RenderType;

typedef struct _SolidColor {
  struct primitive2d primitive;
  union vector4d color;
} SolidColor;

typedef struct _TexturedQuad {
  struct primitive2d primitive;
  uint32_t vbo_texcoords;
} TexturedQuad;

union matrix4 *matrix4_mult(union matrix4 *a, union matrix4 *b,
                            union matrix4 *m);
union matrix3 *matrix3_mult(union matrix3 *a, union matrix3 *b);

int render_solid_color(RenderInfo *info, float dt);
int render_textured(RenderInfo *info, float dt);
int render_particles(RenderInfo *info, float dt);

int add_render_type(Array *ris, RenderInfo *info, void *context,
                    RenderType type);

int coloredgrid_new(union vector2d *lower_left, union vector2d *upper_right,
                    size_t *subdivisions, union vector4d *color,
                    SolidColor **out_color);
int texturedquad_new(union vector2d *lower_left, union vector2d *upper_right,
                     TexturedQuad **out_quad);

int renderinfo_coloredgrid(RenderInfo *info, size_t subdivisions);
int renderinfo_texturedquad(RenderInfo *info);
int renderinfo_particles(RenderInfo *info);

#ifdef __cplusplus
}
#endif
