#pragma once

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <prt/runtime/resource_manager.h>
#include <prt/graphics/texture.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vertex {
  union {
    float buf[3];
    struct {
      float x;
      float y;
      float z;
    };
  };
};

struct buffers {
  uint32_t vbo_positions;
  uint32_t vbo_normals;
  uint32_t vbo_texcoords;
  uint32_t vbo_index;
};

struct raw_buffers {
  size_t num;
  float *positions;
  float *normals;
  float *texcoords;
  uint16_t *indices;
};

typedef void (*MeBuffersCallback)(struct raw_buffers *buffers);

typedef struct _BoundingBox {
  struct vertex min;
  struct vertex max;
  struct vertex center;
} BoundingBox;

typedef struct _Mesh {
  uint32_t vao_buffer;
  struct buffers buffers;
  BoundingBox *box;
  GPUTexture *texture;
} Mesh;

int mesh_new(Mesh **out_mesh);
int mesh_load_from(Mesh *mesh, FileResource *fr, ResourceManager *manager);
int mesh_load_async(Mesh *mesh, const char *p, ResourceManager *rm,
                    MeBuffersCallback buffers_callback, WaitHandle *wh);
int mesh_unref(Mesh *mesh);

#ifdef __cplusplus
}
#endif
