#include <prt/graphics/rendering.h>

/* 0-based index -> blend mode */
static GLenum _op_lookup[] = {
    GL_ZERO,                     GL_ONE,
    GL_SRC_COLOR,                GL_ONE_MINUS_SRC_COLOR,
    GL_DST_COLOR,                GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA,                GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA,                GL_ONE_MINUS_DST_ALPHA,
    GL_SRC_ALPHA_SATURATE,       GL_CONSTANT_COLOR,
    GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_ALPHA,
    GL_ONE_MINUS_CONSTANT_ALPHA};

/* 0-based index -> blend func */
static GLenum _func_lookup[] = {GL_MIN, GL_MAX, GL_FUNC_ADD, GL_FUNC_SUBTRACT,
                                GL_FUNC_REVERSE_SUBTRACT};

/* @func `_ptr_compare`
 * @desc Unsigned pointer compare
 *
 * @param(a) First pointer
 * @param(b) Second pointer
 *
 * @ret 0 when `a` == `b`, 1 when `a` > `b`, -1 when `b` > `a`
 */
static int _ptr_compare(const void *a, const void *b) {
  if ((uintptr_t)a > (uintptr_t)b)
    return 1;
  else if ((uintptr_t)a < (uintptr_t)b)
    return -1;

  return 0;
}

/* @func `blendmode_to_gl`
 * @desc Converts `BlendMode` in `bm` to OpenGL counterparts
 *
 * @param(bm)   Input `BlendMode`
 * @param(src)  Output `source` blending
 * @param(dst)  Output `destination` blending
 * @param(func) Output blending function
 *
 * @ret always 0
 */
int blendmode_to_gl(BlendMode bm, GLenum *src, GLenum *dst, GLenum *func) {
  *src = _op_lookup[bm.source];
  *dst = _op_lookup[bm.destination];
  *func = _func_lookup[bm.func];
  return 0;
}

/* @func `gl_to_blendmode`
 * @desc Converts OpenGL blending to `BlendMode`
 *
 * @param(src)  Input `source` blending
 * @param(dst)  Input `destination` blending
 * @param(func) Input blending function
 * @param(bm)   Output `BlendMode`
 */
int gl_to_blendmode(GLenum src, GLenum dst, GLenum func, BlendMode *bm) {
  switch (src) {
  case GL_ZERO:
    bm->source = 0;
    break;
  case GL_ONE:
    bm->source = 1;
    break;
  case GL_SRC_COLOR:
    bm->source = 2;
    break;
  case GL_ONE_MINUS_SRC_COLOR:
    bm->source = 3;
    break;
  case GL_DST_COLOR:
    bm->source = 4;
    break;
  case GL_ONE_MINUS_DST_COLOR:
    bm->source = 5;
    break;
  case GL_SRC_ALPHA:
    bm->source = 6;
    break;
  case GL_ONE_MINUS_SRC_ALPHA:
    bm->source = 7;
    break;
  case GL_DST_ALPHA:
    bm->source = 8;
    break;
  case GL_ONE_MINUS_DST_ALPHA:
    bm->source = 9;
    break;
  case GL_SRC_ALPHA_SATURATE:
    bm->source = 10;
    break;
  case GL_CONSTANT_COLOR:
    bm->source = 11;
    break;
  case GL_ONE_MINUS_CONSTANT_COLOR:
    bm->source = 12;
    break;
  case GL_CONSTANT_ALPHA:
    bm->source = 13;
    break;
  case GL_ONE_MINUS_CONSTANT_ALPHA:
    bm->source = 14;
    break;
  default:
    break;
  }

  switch (dst) {
  case GL_ZERO:
    bm->destination = 0;
    break;
  case GL_ONE:
    bm->destination = 1;
    break;
  case GL_SRC_COLOR:
    bm->destination = 2;
    break;
  case GL_ONE_MINUS_SRC_COLOR:
    bm->destination = 3;
    break;
  case GL_DST_COLOR:
    bm->destination = 4;
    break;
  case GL_ONE_MINUS_DST_COLOR:
    bm->destination = 5;
    break;
  case GL_SRC_ALPHA:
    bm->destination = 6;
    break;
  case GL_ONE_MINUS_SRC_ALPHA:
    bm->destination = 7;
    break;
  case GL_DST_ALPHA:
    bm->destination = 8;
    break;
  case GL_ONE_MINUS_DST_ALPHA:
    bm->destination = 9;
    break;
  case GL_SRC_ALPHA_SATURATE:
    bm->destination = 10;
    break;
  case GL_CONSTANT_COLOR:
    bm->destination = 11;
    break;
  case GL_ONE_MINUS_CONSTANT_COLOR:
    bm->destination = 12;
    break;
  case GL_CONSTANT_ALPHA:
    bm->destination = 13;
    break;
  case GL_ONE_MINUS_CONSTANT_ALPHA:
    bm->destination = 14;
    break;
  default:
    break;
  }

  switch (func) {
  case GL_MIN:
    bm->func = 0;
    break;
  case GL_MAX:
    bm->func = 1;
    break;
  case GL_FUNC_ADD:
    bm->func = 2;
    break;
  case GL_FUNC_SUBTRACT:
    bm->func = 3;
    break;
  case GL_FUNC_REVERSE_SUBTRACT:
    bm->func = 4;
    break;
  default:
    break;
  }

  return 0;
}

/* @func `renderinfo_new`
 * @desc Creates a new `RenderInfo` object
 *
 * @param(out_ri) Receives the create object
 *
 * @ret 0 on success or error code
 */
int renderinfo_new(RenderInfo **out_ri) {
  RenderInfo *ri;

  ri = NEW0(RenderInfo);
  if (!ri)
    return -ENOMEM;

  ri->primitive = NEW0(RenderPrimitive);
  if (!ri->primitive) {
    free((void *)ri);
    return -ENOMEM;
  }
  *out_ri = ri;

  return 0;
}

/* @func `renderinfo_set_primitives`
 * @desc Sets primitive's OpenGL type and basic rendering information
 *
 * @param(info)  Render info object
 * @param(type)  Type of the primitive `(GL_LINES, GL_TRIANGLES ...)`
 * @param(start) Start index
 * @param(num)   Number of primitives
 *
 * @ret always 0
 */
int renderinfo_set_primitives(RenderInfo *info, GLenum type, size_t start,
                              size_t num) {
  assert(info);
  info->primitive->primitive = type;
  info->primitive->start_primitive = start;
  info->primitive->num_primitives = num;
  return 0;
}

/* @func `renderinfo_set_vertex_attribs`
 * @desc Sets which vertex attribute to activate
 *
 * @param(info)   Render info object
 * @param(attrib) Attribute to activate
 *
 * @ret always 0
 */
int renderinfo_set_vertex_attribs(RenderInfo *info, VertexAttrib attrib) {
  assert(info);
  info->attribs |= (1 << attrib);
  return 0;
}

/* @func `renderinfo_set_blending`
 * @desc Sets blending information
 *
 * @param(info) Render info object
 * @param(src)  Source blending
 * @param(dst)  Destination blending
 * @param(func) Blending function
 *
 * @ret 0 on success or error code
 */
int renderinfo_set_blending(RenderInfo *info, GLenum src, GLenum dst,
                            GLenum func) {
  assert(info);
  return gl_to_blendmode(src, dst, func, &info->blending);
}

/* @func `renderinfo_set_pass`
 * @desc Sets shader effect pass for rendering
 *
 * @param(info) Render info object
 * @param(pass) Shader effect pass
 *
 * @ret always 0
 */
int renderinfo_set_pass(RenderInfo *info, Pass *pass) {
  assert(info);
  info->shader_pass = pass;
  return 0;
}

/* @func `renderinfor_set_textures`
 * @desc Add reference to the given `textures`
 *
 * @param(info)     Render info object
 * @param(textures) Array of texture objects
 * @param(num)      Number of texture objects in the array
 *
 * @ret 0 on success or error code
 */
int renderinfo_set_textures(RenderInfo *info, GPUTexture **textures,
                            size_t num) {
  size_t i;
  int r;
  assert(info);

  for (i = 0; i < num; ++i) {
    r = array_add(&info->textures, (const char *)(textures + i),
                  sizeof(GPUTexture *));
    if (r < 0)
      return r;
  }

  return array_sort(&info->textures, _ptr_compare, sizeof(GPUTexture *));
}

/* @func `renderinfo_set_context`
 * @desc Sets render info's context
 *
 * @param(info)    Render info object
 * @param(context) Context
 *
 * @ret always 0
 */
int renderinfo_set_context(RenderInfo *info, void *context) {
  assert(info);
  info->context = context;
  return 0;
}

/* @func `renderinfo_unref`
 * @desc Deletes render info object
 *
 * @param(info) Render info object
 *
 * @ret always 0
 */
int renderinfo_unref(RenderInfo *info) {
  assert(info);
  free((void *)info->textures.items);
  free((void *)info);
  return 0;
}

/* the sort order determines how much OpenGL state we need to change/keep
 * between successive render calls */
int renderinfo_compare(const RenderInfo *a, const RenderInfo *b) {
  size_t num_textures = 0, i;
  assert(a);
  assert(b);

  if (a->order > b->order)
    return 1;
  else if (a->order < b->order)
    return -1;

  if (a->shader_pass > b->shader_pass)
    return 1;
  else if (a->shader_pass < b->shader_pass)
    return -1;

  num_textures =
      MIN(a->textures.occupied, b->textures.occupied) / sizeof(GPUTexture *);

  /* textures should be sorted by this point already */
  for (i = 0; i < num_textures; ++i) {
    if (ARRAY_GET(&a->textures, i, GPUTexture) >
        ARRAY_GET(&b->textures, i, GPUTexture))
      return 1;
    else if (ARRAY_GET(&a->textures, i, GPUTexture) <
             ARRAY_GET(&b->textures, i, GPUTexture))
      return -1;
  }

  if (a->blending.value > b->blending.value)
    return 1;
  else if (a->blending.value < b->blending.value)
    return -1;

  return 0;
}

int renderinfo_sort(RenderInfo *infos, size_t num) {
  assert(infos);
  qsort(infos, num, sizeof(RenderInfo),
        (int (*)(const void *, const void *))renderinfo_compare);
  return 0;
}

int renderinfo_set_camera(RenderInfo *info, struct _Camera *camera) {
  assert(info);
  info->camera = camera;
  return 0;
}

int renderinfo_erase(Array *array, const RenderInfo *info) {
  size_t num_items = array->occupied / sizeof(*info), i;
  typeof(info) ri;
  assert(array);
  assert(info);

  for (i = 0; i < num_items; ++i) {
    ri = ARRAY_GET(array, i, RenderInfo);

    if (ri->order != info->order)
      continue;

    if (ri->blending.value != info->blending.value)
      continue;

    if (ri->shader_pass != info->shader_pass)
      continue;

    if (ri->primitive != info->primitive)
      continue;

    if (ri->primitive != info->primitive)
      continue;

    if (ri->attribs != info->attribs)
      continue;

    if (ri->textures.items != info->textures.items)
      continue;

    if (ri->textures.capacity != info->textures.capacity)
      continue;

    if (ri->textures.occupied != info->textures.occupied)
      continue;

    if (ri->camera != info->camera)
      continue;

    return array_remove(array, i * sizeof(info), sizeof(info));
  }

  return -ENOENT;
}

int renderinfo_set_order(RenderInfo *info, uint32_t order) {
  info->order = order;
  return 0;
}

/* @func `renderinfo_render`
 * @desc Renders a single render info object
 *
 * @param(info) Render info object
 * @param(dt)   Time delta since last draw
 *
 * @ret 0 on success or error code
 */
int renderinfo_render(RenderInfo *info, float dt) {
  size_t i, n;
  GLenum src, dst, func;
  assert(info);

  GL_CHECK(glUseProgram(info->shader_pass->shader_program));

  if (info->blending.value != 0) {
    GL_CHECK(glEnable(GL_BLEND));
    blendmode_to_gl(info->blending, &src, &dst, &func);
    GL_CHECK(glBlendFunc(src, dst));
  } else
    GL_CHECK(glDisable(GL_BLEND));

  (void)array_num_items(&info->textures, sizeof(GPUTexture *), &n);
  if (!n)
    GL_CHECK(glDisable(GL_TEXTURE_2D));
  else
    GL_CHECK(glEnable(GL_TEXTURE_2D));

  for (i = 0; i < n; ++i) {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + i));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D,
                           (*ARRAY_GET(&info->textures, i, GPUTexture *))->id));
  }

  /* callback should set appropriate uniforms so that
   * we can render and have clients control over the
   * uniform buffers */
  info->callback(info, dt);

  for (i = info->attribs, n = 0; i > 0; i >>= 1, n++)
    if (i & 1)
      GL_CHECK(glEnableVertexAttribArray(n));
  
  GL_CHECK(glDrawArrays(info->primitive->primitive,
                        info->primitive->start_primitive,
                        info->primitive->num_primitives));

  for (i = info->attribs, n = 0; i > 0; i >>= 1, n++)
    if (i & 1)
      GL_CHECK(glDisableVertexAttribArray(n));

  return 0;
}
