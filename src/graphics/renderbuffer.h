#include <src/graphics/common.h>

typedef struct _Renderbuffer {
  uint32_t buf_id;
  uint32_t texture_id;
  uint32_t size[2];
  GLenum target;
  GLenum color_spec;
} Renderbuffer;

int renderbuffer_new(uint32_t width, uint32_t height, GLenum target,
                     GLenum spec, Renderbuffer **out_rb);
int renderbuffer_texture_create(Renderbuffer *rb, GLenum value_type);
int renderbuffer_unref(Renderbuffer *rb);
