#include <prt/graphics/renderbuffer.h>

int renderbuffer_new(uint32_t width, uint32_t height, GLenum target,
                     GLenum spec, Renderbuffer **out_rb) {
  Renderbuffer *rb;

  rb = NEW0(Renderbuffer);
  if (!rb)
    return -ENOMEM;

  GL_CHECK(glGenRenderbuffers(1, &rb->buf_id));
  GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, rb->buf_id));
  GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, spec, width, height));
  GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, target, GL_RENDERBUFFER,
                                     rb->buf_id));

  rb->size[0] = width;
  rb->size[1] = height;
  rb->target = target;
  rb->color_spec = spec;

  *out_rb = rb;

  return 0;
}

/* value_type could quite possibly be derived from rb->color_spec */
int renderbuffer_texture_create(Renderbuffer *rb, GLenum value_type) {
  assert(rb);
  GL_CHECK(glGenTextures(1, &rb->texture_id));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, rb->texture_id));
  GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, rb->color_spec, rb->size[0],
                        rb->size[1], 0, GL_RGBA, value_type, NULL));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, rb->target, GL_TEXTURE_2D,
                                  rb->texture_id, 0));
  return 0;
}

int renderbuffer_unref(Renderbuffer *rb) {
  glDeleteRenderbuffers(1, &rb->buf_id);
  glDeleteTextures(1, &rb->texture_id);
  free((void *)rb);
  return 0;
}
