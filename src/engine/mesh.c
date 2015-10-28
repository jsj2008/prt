#include <src/engine/mesh.h>
#include <src/graphics/common.h>

int _get_bounding_box(float *vertices, size_t num_vertices, BoundingBox *box) {
  size_t i;

  for (i = 0; i < num_vertices; i += 3) {
    box->min.x = MIN(box->min.x, vertices[i * 3 + 0]);
    box->min.y = MIN(box->min.y, vertices[i * 3 + 1]);
    box->min.z = MIN(box->min.z, vertices[i * 3 + 2]);
    box->max.x = MAX(box->max.x, vertices[i * 3 + 0]);
    box->max.y = MAX(box->max.y, vertices[i * 3 + 1]);
    box->max.z = MAX(box->max.z, vertices[i * 3 + 2]);
  }

  box->center.x = (box->max.x + box->min.x) / 2.0;
  box->center.y = (box->max.y + box->min.y) / 2.0;
  box->center.z = (box->max.z + box->min.z) / 2.0;

  return 0;
}

int _merge_scene_meshes(const struct aiScene *scene, Mesh *m,
                        MeBuffersCallback cb) {
  struct raw_buffers buffers = {0};
  size_t face_count = 0, vertex_count = 0, i = 0, f = 0;
  size_t face_index = 0, vertex_index = 0, uv_index = 0;
  GLuint mesh_vao = 0, buffer_vbo = 0;
  const struct aiMesh *mesh = NULL;
  const struct aiFace *face = NULL;
  int r;

  for (; i < scene->mNumMeshes; ++i) {
    mesh = scene->mMeshes[i];

    face_count += mesh->mNumFaces;
    vertex_count += mesh->mNumVertices;
  }

  buffers.indices = (uint16_t *)calloc(face_count * 3, sizeof(uint16_t));
  buffers.positions = (float *)calloc(vertex_count * 3, sizeof(float));
  buffers.normals = (float *)calloc(vertex_count * 3, sizeof(float));
  buffers.texcoords = (float *)calloc(vertex_count * 2, sizeof(float));

  if (!buffers.indices || !buffers.normals || !buffers.positions ||
      !buffers.texcoords) {
    r = -ENOMEM;
    goto cleanup;
  }

  for (i = 0; i < scene->mNumMeshes; ++i) {
    mesh = scene->mMeshes[i];

    for (f = 0; f < mesh->mNumFaces; ++f) {
      face = &mesh->mFaces[f];

      buffers.indices[face_index++] = face->mIndices[0];
      buffers.indices[face_index++] = face->mIndices[1];
      buffers.indices[face_index++] = face->mIndices[2];
    }

    memcpy(buffers.positions + vertex_index, &mesh->mVertices[0].x,
           mesh->mNumVertices * 3 * sizeof(float));
    memcpy(buffers.normals + vertex_index, &mesh->mNormals[0].x,
           mesh->mNumVertices * 3 * sizeof(float));
    memcpy(buffers.texcoords + uv_index, &mesh->mTextureCoords[0][0].x,
           mesh->mNumVertices * 2 * sizeof(float));

    buffers.num += mesh->mNumVertices;
    vertex_index += mesh->mNumVertices * 3;
    uv_index += mesh->mNumVertices * 2;
  }

  /* create VAO */
  GL_CHECK(glGenVertexArrays(1, &mesh_vao));
  GL_CHECK(glBindVertexArray(mesh_vao));
  m->vao_buffer = mesh_vao;

  /* index buffer */
  GL_CHECK(glGenBuffers(1, &buffer_vbo));
  GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_vbo));
  GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, face_index, buffers.indices,
                        GL_STATIC_DRAW));
  m->buffers.vbo_index = buffer_vbo;

  /* vertex buffer */
  GL_CHECK(glGenBuffers(1, &buffer_vbo));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, buffer_vbo));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, vertex_index, buffers.positions,
                        GL_STATIC_DRAW));
  GL_CHECK(glEnableVertexAttribArray(ATTR_VERTEX));
  GL_CHECK(glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, 0, 0, (void *)0));
  m->buffers.vbo_positions = buffer_vbo;

  /* normal buffer  */
  GL_CHECK(glGenBuffers(1, &buffer_vbo));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, buffer_vbo));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, vertex_index, buffers.normals,
                        GL_STATIC_DRAW));
  GL_CHECK(glEnableVertexAttribArray(ATTR_NORMALS));
  GL_CHECK(glVertexAttribPointer(ATTR_NORMALS, 3, GL_FLOAT, 0, 0, (void *)0));
  m->buffers.vbo_normals = buffer_vbo;

  /* uv buffer */
  GL_CHECK(glGenBuffers(1, &buffer_vbo));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, buffer_vbo));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, uv_index, buffers.texcoords,
                        GL_STATIC_DRAW));
  GL_CHECK(glEnableVertexAttribArray(ATTR_UV));
  GL_CHECK(glVertexAttribPointer(ATTR_UV, 2, GL_FLOAT, 0, 0, (void *)0));
  m->buffers.vbo_texcoords = buffer_vbo;

  /* unbind buffers */
  GL_CHECK(glBindVertexArray(0));
  GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

  if (cb)
    cb(&buffers);

  (void)_get_bounding_box(buffers.positions, vertex_index, m->box);

cleanup:
  free(buffers.indices);
  free(buffers.positions);
  free(buffers.normals);
  free(buffers.texcoords);

  return 0;
}

static int _get_material_texture(const struct aiMaterial *material,
                                 enum aiTextureType type,
                                 const char **out_path) {
  __free_str char *tmp;
  const char *fullpath;
  enum aiReturn rt;
  struct aiString path;

  tmp = NULL;
  rt = aiGetMaterialTexture(material, type, 0, &path, NULL, NULL, NULL, NULL,
                            NULL, NULL);

  if (rt != AI_SUCCESS)
    return -EINVAL;

  tmp = str_strip_left(path.data);
  fullpath = strdup(strjoina("textures/", tmp));
  if (!fullpath)
    return -ENOMEM;

  *out_path = fullpath;

  return 0;
}

static int _load_single_mesh(Mesh *mesh, const struct aiScene *scene,
                             ResourceManager *manager, MeBuffersCallback cb) {
  const struct aiMesh *m;
  const struct aiMaterial *material;
  FileResource *fr;
  const char *texture_name;
  size_t i;
  int r;

  if (scene->mNumMaterials != 1) {
    Log("Invalid number of materials: %zu\n", scene->mNumMaterials);
    return -EINVAL;
  }

  material = scene->mMaterials[0];

  r = _get_material_texture(material, aiTextureType_DIFFUSE, &texture_name);
  if (r < 0)
    return r;

  r = texture_new(&mesh->texture);
  if (r < 0)
    goto out;

  r = texture_load_from(mesh->texture, manager, texture_name);
  if (r < 0)
    goto free_texture;

  r = _merge_scene_meshes(scene, mesh, cb);
  if (r < 0)
    goto free_texture;

out:
  free((void *)texture_name);
  return r;
free_texture:
  free((void *)texture_name);
  if (mesh->texture)
    texture_unref(mesh->texture);
  return r;
}

static int _load_assimp_scene(Mesh *m, FileResource *fr,
                              ResourceManager *manager, MeBuffersCallback cb) {
  const struct aiScene *scene;
  int r;

  scene = aiImportFileFromMemory(fr->data, fr->size,
                                 aiProcessPreset_TargetRealtime_Quality, NULL);
  if (!scene) {
    Log("%s", aiGetErrorString());
    return -EINVAL;
  }

  r = _load_single_mesh(m, scene, manager, cb);

  aiReleaseImport(scene);

  return r;
}

struct CbContext {
  Mesh *mesh;
  ResourceManager *manager;
  MeBuffersCallback buffers_callback;
};

/* executes on the worker thread */
static void _rm_callback(FileResource *fr, void *context) {
  struct CbContext *cb = (struct CbContext *)context;
  int r;

  if (!fr)
    return;

  r = _load_assimp_scene(cb->mesh, fr, cb->manager, cb->buffers_callback);
  if (r < 0)
    Log("_rm_callback: Load failed: %i\n", r);
}

int mesh_new(Mesh **out_mesh) {
  Mesh *m;

  m = NEW0(Mesh);
  if (!m)
    return -ENOMEM;

  m->box = NEW0(BoundingBox);
  if (!m->box) {
    free((void *)m);
    return -ENOMEM;
  }

  return 0;
}

int mesh_load_from(Mesh *mesh, FileResource *fr, ResourceManager *manager) {
  assert(mesh);
  assert(fr);
  return _load_assimp_scene(mesh, fr, manager, NULL);
}

int mesh_load_async(Mesh *mesh, const char *path, ResourceManager *manager,
                    MeBuffersCallback buffers_callback, WaitHandle *wh) {
  struct CbContext *cb;
  int r;
  assert(mesh);
  assert(path);
  assert(manager);

  cb = NEW0(struct CbContext);
  if (!cb)
    return -ENOMEM;

  cb->mesh = mesh;
  cb->manager = manager;
  cb->buffers_callback = buffers_callback;

  r = resource_manager_load_async(manager, path, _rm_callback, cb, wh);
  if (r < 0) {
    free((void *)cb);
    return r;
  }

  return 0;
}

int mesh_unref(Mesh *mesh) {
  glDeleteVertexArrays(1, &mesh->vao_buffer);
  glDeleteBuffers(4, &mesh->buffers.vbo_positions);
  texture_unref(mesh->texture);
  free((void *)mesh->box);
  free((void *)mesh);
  return 0;
}
