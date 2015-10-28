#pragma once

#include <prt/graphics/common.h>
#include <prt/graphics/texture.h>
#include <prt/graphics/shader.h>
#include <prt/shared/array.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _RenderInfo;
typedef uint8_t VertexAttribFlags;
typedef int (*RiContextCb)(struct _RenderInfo *, float dt);
typedef void (*RiMatrixMult)(const float *a, const float *b, float *c);

typedef struct _RenderIndirect {
  uint32_t num_primitives;
  uint32_t instances;
  uint32_t start_primitive;
  uint32_t reserved;
} RenderIndirect;

typedef struct _RenderPrimitive {
  GLenum primitive;
  uint32_t num_primitives;
  uint32_t start_primitive;
  uint32_t instances;
  // RenderIndirect indirect;
} RenderPrimitive;

typedef struct _RenderInfo {
  unsigned order;

  RenderPrimitive *primitive;

  VertexAttribFlags attribs;
  BlendMode blending;

  Pass *shader_pass;
  struct _Camera *camera;

  Array textures;

  RiMatrixMult matrixmult;
  RiContextCb callback;
  void *context;
} RenderInfo;

int blendmode_to_gl(BlendMode bm, GLenum *src, GLenum *dst, GLenum *func);
int gl_to_blendmode(GLenum src, GLenum dst, GLenum func, BlendMode *bm);

int renderinfo_new(RenderInfo **out_ri);
int renderinfo_set_primitives(RenderInfo *info, GLenum type, size_t start,
                              size_t num);
int renderinfo_set_vertex_attribs(RenderInfo *info, VertexAttrib attrib);
int renderinfo_set_blending(RenderInfo *info, GLenum src, GLenum dst,
                            GLenum func);
int renderinfo_set_pass(RenderInfo *info, Pass *pass);
int renderinfo_set_textures(RenderInfo *info, GPUTexture **textures,
                            size_t num);
int renderinfo_set_context(RenderInfo *info, void *context);
int renderinfo_set_camera(RenderInfo *info, struct _Camera *camera);
int renderinfo_set_order(RenderInfo *info, uint32_t order);

int renderinfo_unref(RenderInfo *info);

int renderinfo_compare(const RenderInfo *a, const RenderInfo *b);
int renderinfo_sort(RenderInfo *infos, size_t num);
int renderinfo_erase(Array *array, const RenderInfo *info);
int renderinfo_render(RenderInfo *info, float dt);

#ifdef __cplusplus
}
#endif
