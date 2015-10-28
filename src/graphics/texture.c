#include <src/graphics/texture.h>
#include <src/graphics/common.h>
#define STB_IMAGE_IMPLEMENTATION
#include <src/graphics/stb_image.h>

/* @func `_load_texture`
 * @desc Loads the texture from `fr` and uploads it to the GPU
 *
 * @param(texture) Texture object
 * @param(fr)      File resource from which to load
 *
 * @ret 0 on success or error code
 */
int _load_texture(GPUTexture *texture, FileResource *fr) {
  GLuint *textureids;
  uint8_t *image;
  size_t i, t;
  int r, w, h, comp;
  GLenum bc;

  r = 0;
  image = NULL;

  textureids = (GLuint *)calloc(1, sizeof(GLuint));
  if (!textureids) {
    r = -ENOMEM;
    goto cleanup;
  }

  texture->name = strdup(fr->path);
  if (!texture->name) {
    r = -ENOMEM;
    goto cleanup;
  }

  image = stbi_load_from_memory((uint8_t *)fr->data, fr->size, &w, &h, &comp,
                                STBI_default);
  if (!image) {
    printf("Image Loading Failure: %s\n", stbi_failure_reason());
    free((void *)texture->name);
    r = -EINVAL;
    goto cleanup;
  }

  bc = comp == 3 ? GL_RGB : GL_RGBA;
  GL_CHECK(glGenTextures(1, textureids));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureids[0]));

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
  GL_CHECK(
      glTexImage2D(GL_TEXTURE_2D, 0, bc, w, h, 0, bc, GL_UNSIGNED_BYTE, image));
  GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
  GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
  GL_CHECK(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
  GL_CHECK(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));

  texture->size[0] = w;
  texture->size[1] = h;
  texture->id = textureids[0];

  Log("Texture '%s' loaded! [%u]", fr->path, texture->id);

cleanup:
  if (image)
    free((void *)image);
  free(textureids);
  return r;
}

/* @func `texture_new`
 * @desc Creates a new texture object
 *
 * @param(out_texture) Receives the created object
 *
 * @ret 0 on success or error code
 */
int texture_new(GPUTexture **out_texture) {
  GPUTexture *t;

  t = NEW0(GPUTexture);
  if (!t)
    return -ENOMEM;

  *out_texture = t;

  return 0;
}

/* @func `texture_load`
 * @desc Loads a GPU texture from given file resource
 *
 * @param(texture) Texture object
 * @param(fr)      File resource from which to load
 *
 * @ret 0 on success or error code
 */
int texture_load(GPUTexture *texture, FileResource *fr) {
  return _load_texture(texture, fr);
}

/* @func `texture_load_from`
 * @desc Loads the texture from filesystem path and optionally caches
 * the file resource in `manager`
 *
 * @param(texture) Texture object
 * @param(manager) Optional `ResourceManager` instance
 * @param(path)    Path from where to load
 *
 * @ret 0 on success or error code
 */
int texture_load_from(GPUTexture *texture, ResourceManager *manager,
                      const char *path) {
  FileResource *fr = NULL;
  int r;
  assert(texture);
  assert(manager);
  assert(path);

  r = resource_manager_load(manager, path, &fr);
  if (r < 0)
    return r;

  return _load_texture(texture, fr);
}

/* @func `texture_unref`
 * @desc Deletes a texture object
 *
 * @param(texture) Texture object
 *
 * @ret 0 on success or error code
 */
int texture_unref(GPUTexture *texture) {
  assert(texture);
  glDeleteTextures(1, &texture->id);
  free((void *)texture->name);
  free((void *)texture);
  return 0;
}
