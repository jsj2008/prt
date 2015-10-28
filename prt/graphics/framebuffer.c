#include <prt/graphics/framebuffer.h>

int framebuffer_new(Framebuffer **out_fb) {
  Framebuffer *fb;
  fb = NEW0(Framebuffer);
  if (!fb)
    return -ENOMEM;

  GL_CHECK(glGenFramebuffers(1, &fb->buffer_id));
  GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fb->buffer_id));

  return 0;
}

int framebuffer_finalize(Framebuffer *fb, GLenum *targets, size_t num_targets) {
  GLenum status;
  assert(fb);
  assert(targets);

  GL_CHECK(glDrawBuffers(num_targets, targets));

  status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    printf("FBO Initialization failed!\n");
    return 1;
  }

  GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

  return 0;
}

int framebuffer_unref(Framebuffer *fb) {
  assert(fb);

  glDeleteFramebuffers(1, &fb->buffer_id);
  renderbuffer_unref(fb->diffuse);
  renderbuffer_unref(fb->normal);
  renderbuffer_unref(fb->position);
  renderbuffer_unref(fb->depth);
  free((void *)fb);
  return 0;
}
