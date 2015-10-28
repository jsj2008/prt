#include <prt/engine/render.h>
#include <prt/engine/particles.h>

#define CAM_TO_MATRIX(info)                                                    \
  (union matrix4 *)&info->camera->view_projection.data[0]

int render_particles(RenderInfo *info, float dt) {
  union matrix4 mvp;
  Particles *particles;
  assert(info);

  particles = (Particles *)info->context;

  matrix4_mult(CAM_TO_MATRIX(info), &particles->primitive.model, &mvp);
  shader_pass_set_uniform(info->shader_pass, HASH("mvp"), &mvp.data[0]);

  /* update the particle system */
  (void)particles_tick(particles, dt);
  /* update final particle count */
  info->primitive->num_primitives = particles->particle_count;

  /* bind position & color vbo's */
  /*GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, particles->vbo_positions));
  GL_CHECK(glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, 0, 0, (void *)0));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, particles->vbo_colors));
  GL_CHECK(glVertexAttribPointer(ATTR_COLOR, 4, GL_FLOAT, 0, 0, (void *)0));*/

  GL_CHECK(glBindVertexArray(particles->vao));

  return 0;
}

int render_textured(RenderInfo *info, float dt) {
  union matrix4 mvp;
  TexturedQuad *quad;
  assert(info);

  quad = (TexturedQuad *)info->context;

  matrix4_mult(CAM_TO_MATRIX(info), &quad->primitive.model, &mvp);
  shader_pass_set_uniform(info->shader_pass, HASH("mvp"), &mvp.data[0]);
  GL_CHECK(glBindVertexArray(quad->primitive.vao));

  return 0;
}

int render_solid_color(RenderInfo *info, float dt) {
  union matrix4 mvp;
  SolidColor *solid;
  assert(info);

  solid = (SolidColor *)info->context;

  matrix4_mult(CAM_TO_MATRIX(info), &solid->primitive.model, &mvp);
  shader_pass_set_uniform(info->shader_pass, HASH("mvp"), &mvp.data[0]);
  shader_pass_set_uniform(info->shader_pass, HASH("color"),
                          &solid->color.data[0]);

  GL_CHECK(glBindVertexArray(solid->primitive.vao));

  return 0;
}

/* (AB)ij = SUM(k=1...4) Aik * Bkj */
union matrix4 *matrix4_mult(union matrix4 *a, union matrix4 *b,
                            union matrix4 *m) {

  /* Python generator of the unrolled version:
   *
   * https://gist.github.com/shaded-enmity/2153a5200321bd36f0f5#file-matgen-py
   *
   */

  m->data[0 + (0 * 4)] = a->data[0 + (0 * 4)] * b->data[0 + (0 * 4)] +
                         a->data[0 + (1 * 4)] * b->data[1 + (0 * 4)] +
                         a->data[0 + (2 * 4)] * b->data[2 + (0 * 4)] +
                         a->data[0 + (3 * 4)] * b->data[3 + (0 * 4)];
  m->data[0 + (1 * 4)] = a->data[0 + (0 * 4)] * b->data[0 + (1 * 4)] +
                         a->data[0 + (1 * 4)] * b->data[1 + (1 * 4)] +
                         a->data[0 + (2 * 4)] * b->data[2 + (1 * 4)] +
                         a->data[0 + (3 * 4)] * b->data[3 + (1 * 4)];
  m->data[0 + (2 * 4)] = a->data[0 + (0 * 4)] * b->data[0 + (2 * 4)] +
                         a->data[0 + (1 * 4)] * b->data[1 + (2 * 4)] +
                         a->data[0 + (2 * 4)] * b->data[2 + (2 * 4)] +
                         a->data[0 + (3 * 4)] * b->data[3 + (2 * 4)];
  m->data[0 + (3 * 4)] = a->data[0 + (0 * 4)] * b->data[0 + (3 * 4)] +
                         a->data[0 + (1 * 4)] * b->data[1 + (3 * 4)] +
                         a->data[0 + (2 * 4)] * b->data[2 + (3 * 4)] +
                         a->data[0 + (3 * 4)] * b->data[3 + (3 * 4)];
  m->data[1 + (0 * 4)] = a->data[1 + (0 * 4)] * b->data[0 + (0 * 4)] +
                         a->data[1 + (1 * 4)] * b->data[1 + (0 * 4)] +
                         a->data[1 + (2 * 4)] * b->data[2 + (0 * 4)] +
                         a->data[1 + (3 * 4)] * b->data[3 + (0 * 4)];
  m->data[1 + (1 * 4)] = a->data[1 + (0 * 4)] * b->data[0 + (1 * 4)] +
                         a->data[1 + (1 * 4)] * b->data[1 + (1 * 4)] +
                         a->data[1 + (2 * 4)] * b->data[2 + (1 * 4)] +
                         a->data[1 + (3 * 4)] * b->data[3 + (1 * 4)];
  m->data[1 + (2 * 4)] = a->data[1 + (0 * 4)] * b->data[0 + (2 * 4)] +
                         a->data[1 + (1 * 4)] * b->data[1 + (2 * 4)] +
                         a->data[1 + (2 * 4)] * b->data[2 + (2 * 4)] +
                         a->data[1 + (3 * 4)] * b->data[3 + (2 * 4)];
  m->data[1 + (3 * 4)] = a->data[1 + (0 * 4)] * b->data[0 + (3 * 4)] +
                         a->data[1 + (1 * 4)] * b->data[1 + (3 * 4)] +
                         a->data[1 + (2 * 4)] * b->data[2 + (3 * 4)] +
                         a->data[1 + (3 * 4)] * b->data[3 + (3 * 4)];
  m->data[2 + (0 * 4)] = a->data[2 + (0 * 4)] * b->data[0 + (0 * 4)] +
                         a->data[2 + (1 * 4)] * b->data[1 + (0 * 4)] +
                         a->data[2 + (2 * 4)] * b->data[2 + (0 * 4)] +
                         a->data[2 + (3 * 4)] * b->data[3 + (0 * 4)];
  m->data[2 + (1 * 4)] = a->data[2 + (0 * 4)] * b->data[0 + (1 * 4)] +
                         a->data[2 + (1 * 4)] * b->data[1 + (1 * 4)] +
                         a->data[2 + (2 * 4)] * b->data[2 + (1 * 4)] +
                         a->data[2 + (3 * 4)] * b->data[3 + (1 * 4)];
  m->data[2 + (2 * 4)] = a->data[2 + (0 * 4)] * b->data[0 + (2 * 4)] +
                         a->data[2 + (1 * 4)] * b->data[1 + (2 * 4)] +
                         a->data[2 + (2 * 4)] * b->data[2 + (2 * 4)] +
                         a->data[2 + (3 * 4)] * b->data[3 + (2 * 4)];
  m->data[2 + (3 * 4)] = a->data[2 + (0 * 4)] * b->data[0 + (3 * 4)] +
                         a->data[2 + (1 * 4)] * b->data[1 + (3 * 4)] +
                         a->data[2 + (2 * 4)] * b->data[2 + (3 * 4)] +
                         a->data[2 + (3 * 4)] * b->data[3 + (3 * 4)];
  m->data[3 + (0 * 4)] = a->data[3 + (0 * 4)] * b->data[0 + (0 * 4)] +
                         a->data[3 + (1 * 4)] * b->data[1 + (0 * 4)] +
                         a->data[3 + (2 * 4)] * b->data[2 + (0 * 4)] +
                         a->data[3 + (3 * 4)] * b->data[3 + (0 * 4)];
  m->data[3 + (1 * 4)] = a->data[3 + (0 * 4)] * b->data[0 + (1 * 4)] +
                         a->data[3 + (1 * 4)] * b->data[1 + (1 * 4)] +
                         a->data[3 + (2 * 4)] * b->data[2 + (1 * 4)] +
                         a->data[3 + (3 * 4)] * b->data[3 + (1 * 4)];
  m->data[3 + (2 * 4)] = a->data[3 + (0 * 4)] * b->data[0 + (2 * 4)] +
                         a->data[3 + (1 * 4)] * b->data[1 + (2 * 4)] +
                         a->data[3 + (2 * 4)] * b->data[2 + (2 * 4)] +
                         a->data[3 + (3 * 4)] * b->data[3 + (2 * 4)];
  m->data[3 + (3 * 4)] = a->data[3 + (0 * 4)] * b->data[0 + (3 * 4)] +
                         a->data[3 + (1 * 4)] * b->data[1 + (3 * 4)] +
                         a->data[3 + (2 * 4)] * b->data[2 + (3 * 4)] +
                         a->data[3 + (3 * 4)] * b->data[3 + (3 * 4)];

  return m;
}

/* Naive approach to matrix multiplication using loops
 * (AB)ij = SUM(k=1...3) Aik * Bkj */
/* TODO: Unroll & let caller allocate space for out matrix */
union matrix3 *matrix3_mult(union matrix3 *a, union matrix3 *b) {
  size_t i, j, k;
  union matrix3 *m = NEW0(union matrix3);
  if (!m)
    return NULL;

  for (i = 0; i < 3; ++i)
    for (j = 0; j < 3; ++j)
      for (k = 0; k < 3; ++k)
        m->data[i + (j * 3)] += a->data[i + (k * 3)] * b->data[k + (j * 3)];

  return m;
}

/* `info` gets update with `info->callback` and `info->context` */
int add_render_type(Array *ris, RenderInfo *info, void *context,
                    RenderType type) {
  RiContextCb func;
  int r;
  assert(ris);
  assert(info);
  assert(context);

  switch (type) {
  case RT_SOLID_COLOR:
    func = render_solid_color;
    break;
  case RT_TEXTURED_QUAD:
    func = render_textured;
    break;
  case RT_PARTICLES:
    func = render_particles;
    break;
  default:
    return -EINVAL;
  }

  info->callback = func;
  info->context = context;

  r = array_add(ris, (const char *)info, sizeof(*info));
  if (r < 0)
    return 0;

  renderinfo_sort((RenderInfo *)ris->items, ris->occupied / sizeof(*info));
  return 0;
}

int coloredgrid_new(union vector2d *lower_left, union vector2d *upper_right,
                    size_t *subdivisions, union vector4d *color,
                    SolidColor **out_color) {
  float *linebuf;
  SolidColor *sc;
  float dx, dy;
  float cy, cx, cor;
  size_t k, numfloats, dcy, dcx, j;
  GLuint vbo;

  dx = (upper_right->x - lower_left->x) / (float)*subdivisions;
  dy = (upper_right->y - lower_left->y) / (float)*subdivisions;

  /* cy/cx are corrections to fit the points onto a rectangle */
  cy = dy / dx;
  cx = dx / dy;

  if (cy > cx)
    cor = *subdivisions * (1.0f - cx); /* vertical boost */
  else
    cor = *subdivisions * (1.0f - cy); /* horizontal boost */

  numfloats = (*subdivisions * 12 * 2) + (cor * 6);

  sc = NEW0(SolidColor);
  if (!sc)
    return -ENOMEM;

  linebuf = (float *)calloc(numfloats, sizeof(float));
  if (!linebuf) {
    free((void *)sc);
    return -ENOMEM;
  }

  for (k = 0; k < (*subdivisions * 2); ++k) {
    linebuf[k * 12 + 0] = lower_left->x;
    linebuf[k * 12 + 1] = lower_left->y + (k * dy);
    linebuf[k * 12 + 2] = 0.0f;

    linebuf[k * 12 + 3] = upper_right->x;
    linebuf[k * 12 + 4] = lower_left->y + (k * dy);
    linebuf[k * 12 + 5] = 0.0f;

    linebuf[k * 12 + 6] = lower_left->x + (k * dx * cy);
    linebuf[k * 12 + 7] = lower_left->y;
    linebuf[k * 12 + 8] = 0.0f;

    linebuf[k * 12 + 9] = lower_left->x + (k * dx * cy);
    linebuf[k * 12 + 10] = upper_right->y;
    linebuf[k * 12 + 11] = 0.0f;
  }
  /*
    for (j = (k * 12); k < ((*subdivisions * 2) + dcy); k += 6) {
      linebuf[j + k + 0] = lower_left->x + (k * dx * cy);
      linebuf[j + k + 1] = lower_left->y;
      linebuf[j + k + 2] = 0.0f;

      linebuf[j + k + 3] = lower_left->x + (k * dx * cy);
      linebuf[j + k + 4] = upper_right->y;
      linebuf[j + k + 5] = 0.0f;
    }
  */
  *subdivisions = (*subdivisions * 2) + (cor);

  GL_CHECK(glGenVertexArrays(1, &sc->primitive.vao));
  GL_CHECK(glBindVertexArray(sc->primitive.vao));

  GL_CHECK(glEnableVertexAttribArray(ATTR_VERTEX));
  GL_CHECK(glGenBuffers(1, &sc->primitive.vbo_positions));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, sc->primitive.vbo_positions));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, numfloats * sizeof(float), linebuf,
                        GL_STATIC_DRAW));
  GL_CHECK(glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, 0, 0, NULL));
  GL_CHECK(glBindVertexArray(0));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

  free((void *)linebuf);

  if (color)
    memcpy(&sc->color.data[0], &color->data[0], sizeof(*color));

  sc->primitive.model_invalid = true;
  *out_color = sc;

  return 0;
}

int texturedquad_new(union vector2d *lower_left, union vector2d *upper_right,
                     TexturedQuad **out_quad) {
  TexturedQuad *quad;
  int r;
  assert(lower_left);
  assert(upper_right);

  GLfloat rect[] = {lower_left->x,  lower_left->y,  0.0f,
                    upper_right->x, lower_left->y,  0.0f,
                    lower_left->x,  upper_right->y, 0.0f,
                    upper_right->x, upper_right->y, 0.0f};
  GLfloat uvs[] = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f};

  quad = NEW0(TexturedQuad);
  if (!quad)
    return -ENOMEM;

  GL_CHECK(glGenVertexArrays(1, &quad->primitive.vao));
  GL_CHECK(glBindVertexArray(quad->primitive.vao));

  GL_CHECK(glEnableVertexAttribArray(ATTR_VERTEX));
  GL_CHECK(glGenBuffers(1, &quad->primitive.vbo_positions));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, quad->primitive.vbo_positions));
  GL_CHECK(
      glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), rect, GL_STATIC_DRAW));
  GL_CHECK(glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL));

  GL_CHECK(glEnableVertexAttribArray(ATTR_UV));
  GL_CHECK(glGenBuffers(1, &quad->vbo_texcoords));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, quad->vbo_texcoords));
  GL_CHECK(
      glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), uvs, GL_STATIC_DRAW));
  GL_CHECK(glVertexAttribPointer(ATTR_UV, 2, GL_FLOAT, GL_FALSE, 0, NULL));
  GL_CHECK(glBindVertexArray(0));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

  quad->primitive.size.x = upper_right->x - lower_left->x;
  quad->primitive.size.y = upper_right->y - lower_left->y;

  quad->primitive.model_invalid = true;
  *out_quad = quad;

  return 0;
}

int renderinfo_coloredgrid(RenderInfo *info, size_t subdivisions) {
  assert(info);
  info->primitive->primitive = GL_LINES;
  info->primitive->num_primitives = subdivisions * 4;
  info->attribs = ATTR_VERTEX + 1;
  return 0;
}

int renderinfo_texturedquad(RenderInfo *info) {
  assert(info);
  info->primitive->primitive = GL_TRIANGLE_STRIP;
  info->primitive->num_primitives = 4;
  info->attribs = (ATTR_VERTEX + 1) | (ATTR_UV + 1);
  return 0;
}

int renderinfo_particles(RenderInfo *info) {
  assert(info);
  info->primitive->primitive = GL_POINTS;
  info->attribs = (ATTR_VERTEX + 1) | (ATTR_COLOR + 1);
  return 0;
}
