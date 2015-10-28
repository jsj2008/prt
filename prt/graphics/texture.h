#pragma once

#include <prt/shared/basic.h>
#include <prt/graphics/stb_image.h>
#include <prt/runtime/resource_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GPUTexture {
  const char *name;
  uint32_t id;
  float size[2];
} GPUTexture;

int texture_new(GPUTexture **out_texture);
int texture_load(GPUTexture *texture, FileResource *fr);
int texture_load_from(GPUTexture *texture, ResourceManager *manager,
                      const char *path);
int texture_unref(GPUTexture *texture);

#ifdef __cplusplus
}
#endif
