#include <src/graphics/renderbuffer.h>

typedef struct _DeferredFramebuffer {
  Renderbuffer *diffuse;
  Renderbuffer *normal;
  Renderbuffer *position;
  Renderbuffer *depth;
  uint32_t buffer_id;
} DeferredFramebuffer;

typedef DeferredFramebuffer Framebuffer;

int framebuffer_new(Framebuffer **out_fb);
int framebuffer_finalize(Framebuffer *fb, GLenum *targets, size_t num_targets);
int framebuffer_unref(Framebuffer *fb);
