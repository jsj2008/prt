#include <src/shared/basic.h>
#include <src/graphics/texture.h>
#include <src/graphics/shader.h>
#include <src/engine/render.h>
#include <src/engine/particles.h>

#define GLM_FORCE_RADIANS
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL2/SDL.h>
#include <sys/time.h>

typedef union vector3d vec3;
typedef union vector4d vec4;

/* C++ example of usage of the PRT framework */

/* cache effect types hashes that we use */
enum { ET_PARTICLES = _ET_MAX + 1, __ET_MAX = ET_PARTICLES + 1 };
static const char *Names[__ET_MAX] = {[ET_SOLID_COLOR] = "solid-color",
                                      [ET_SOLID_TEXTURE] = "solid-texture",
                                      [ET_PARTICLES] = "particles"};
static Id Effects[__ET_MAX] = {[ET_SOLID_COLOR] = HASH(Names[ET_SOLID_COLOR]),
                               [ET_SOLID_TEXTURE] =
                                   HASH(Names[ET_SOLID_TEXTURE]),
                               [ET_PARTICLES] = HASH(Names[ET_PARTICLES])};

double time_diff(struct timeval x, struct timeval y) {
  double x_ms, y_ms, diff;
  x_ms = (double)x.tv_sec * 1000000 + (double)x.tv_usec;
  y_ms = (double)y.tv_sec * 1000000 + (double)y.tv_usec;
  diff = (double)y_ms - (double)x_ms;
  return diff * 0.0000001;
}

/* initialize SDL video with opengl es 3.1 context */
static int sdl_video_init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    return -EINVAL;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  return 0;
}

/* computes model matrix from spatial coordinates of given primitive */
static void compute_model_matrix(struct primitive2d &p) {
  glm::mat4 model;

  /* cache identity matrix and Z axis of rotation */
  static glm::mat4 ident;
  static glm::vec3 axis(0, 0, 1);

  if (p.model_invalid) {
    /*  How the monstrosity below works
        -------------------------------

        (A)                               (B)
                      +                                 +
                      |                                 |
                      |     (1, 1)                      |
                      +----+                            |     (0.5, 0.5)
                      |    |                        +-------+
                      |    |                        |   |   |
        +-------------X----+---------+    +-------------X--------------+
                      | rotation anchor             |   |   |
                      |                             +-------+
                      |                  (-0.5, -0.5)   |
                      |                                 |
                      |                                 |
                      +                                 +

        Matrix transformations are applied in a reverse order as they appaer
        in the source file, hence:

        1) First we translate by half the primitive size to move the rotation
           anchor into the middle of the primitive, as depicted at (A) --> (B)
        2) We proceed by scaling the primitive by specified factor,
           because we aligned the rotation anchor with the origin the primitive
           shrinks or expands uniformly on both sides
        3) Next, rotation is applied along the Z axis
        4) Finally we multiply the scale factor with half the size to move the
           primitive where it should be (A) and finish by adding that to the
           position

     */
    model = glm::translate(
        glm::scale(
            glm::rotate(
                glm::translate(
                    ident,
                    glm::vec3(p.spatial.position.x +
                                  ((1.0 - p.spatial.scale.x) * p.size.x * 0.5f),
                              p.spatial.position.y +
                                  ((1.0 - p.spatial.scale.y) * p.size.y * 0.5f),
                              0)),
                p.spatial.rotation, axis),
            glm::vec3(1.0 - p.spatial.scale.x, 1.0 - p.spatial.scale.y, 1)),
        glm::vec3(-p.size.x * 0.5f, -p.size.y * 0.5f, 0));

    memcpy(&p.model.data[0], glm::value_ptr(model), sizeof(float) * 16);
    p.model_invalid = false;
  }
}

struct Window {
  explicit Window(const glm::ivec2 &size)
      : size_(size), count_(0), window_(NULL), surface_(NULL) {
    assert(array_new(&this->array_) == 0);
    assert(hashtable_new(16, &this->shaders_) == 0);
  }

  int CreateLogo();
  int CreateGrid();
  int CreateParticles();

  int CreateSDLWindow();
  int WindowLoop();
  int Draw();

  glm::mat4 camera_;         /* camera matrix */
  Array *array_;             /* render info array */
  Particles *sys_;
  GPUTexture *texture_;      /* logo texture */
  GPUTexture *particle_;
  struct primitive2d *logo_; /* logo transform */
  size_t count_;             /* used for logo animation */
  SDL_Window *window_;
  SDL_Surface *surface_;
  glm::ivec2 size_;          /* windows size  */
  Hashtable *shaders_;       /* shader hashtable */
  struct timeval last_frame; /* last frame render time */
  float dt_last;             /* last time delta */
  Pass *pp_;
};

/* draw all buffered items */
int Window::Draw() {
  size_t i, num_items;
  float dt;
  struct timeval now;

  (void)gettimeofday(&now, NULL);

  dt = time_diff(this->last_frame, now);

  GL_CHECK(glClearColor(0.3f, 0.3f, 0.7f, 1.0f));
  GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                   GL_STENCIL_BUFFER_BIT));

  (void)array_num_items(this->array_, sizeof(RenderInfo), &num_items);
  for (i = 0; i < num_items; ++i)
    renderinfo_render(ARRAY_GET(this->array_, i, RenderInfo), dt);

  (void)gettimeofday(&this->last_frame, NULL);
  this->dt_last = dt;

  return 0;
}

/* poll sdl event queue, draw frame, animate logo, and swap frames */
int Window::WindowLoop() {
  int r;
  SDL_Event event;

  /* process queued SDL events */
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT)
      return 1;
  }

  //(void)this->Draw();
  glDisable(GL_CULL_FACE);
  GL_CHECK(glClearColor(0.3f, 0.3f, 0.3f, 1.0f));
  GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                   GL_STENCIL_BUFFER_BIT));
  GL_CHECK(glUseProgram(this->pp_->shader_program));
  GL_CHECK(glEnable(GL_TEXTURE_2D));
  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, this->particle_->id));
  GLfloat points[] = { 0.5f, 0.5f, 1.0f, 150.0f, 175.0f, 1.0f };
  GLfloat colors[] = { 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f };
  shader_pass_set_uniform(this->pp_, HASH("mvp"), glm::value_ptr(this->camera_));
  GL_CHECK(glEnableVertexAttribArray(0));
  GL_CHECK(glEnableVertexAttribArray(3));
  GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, &points[0]));
  GL_CHECK(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, &colors[0]));
  GL_CHECK(glDrawArrays(GL_POINTS, 0, 2));

  /* update the transform each 500th frame and reset the counter */
  /*if (this->count_ == 500) {
    this->logo_->spatial.position.x += 0.5;
    this->logo_->spatial.scale.y -= 0.001f;
    this->logo_->spatial.scale.x -= 0.001f;
    this->logo_->spatial.rotation += 0.01f;
    this->count_ = 0;
    this->logo_->model_invalid = true;
    compute_model_matrix(*this->logo_);
  }*/
  this->count_++;

  /* update main buffer  */
  SDL_GL_SwapWindow(this->window_);

  return 0;
}

/* create SDL GL context and backing 2D surface */
int Window::CreateSDLWindow() {
  int r;
  this->window_ = SDL_CreateWindow("PRT-Example-CPP", SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, this->size_.x,
                                   this->size_.y, SDL_WINDOW_SHOWN);
  if (!this->window_)
    return -EINVAL;

  this->surface_ = SDL_GetWindowSurface(this->window_);
  if (!this->surface_) {
    printf("SDL Error: %s", SDL_GetError());
    return -EINVAL;
  }

  r = SDL_GL_MakeCurrent(this->window_, SDL_GL_CreateContext(this->window_));
  if (r < 0) {
    printf("SDL Error: %s", SDL_GetError());
    return -EINVAL;
  }

  return 0;
}

/* create fire particles */
int Window::CreateParticles() { 
  Particles *p = NULL;
  RenderInfo *info = NULL;
  Pass *pass = NULL;
  glm::mat4 ident;
  int r;

  /* find effect */
  r = hashtable_find(this->shaders_, Effects[ET_PARTICLES],
                     (void *)Names[ET_PARTICLES], (void **)&pass);
  if (r < 0)
    return r;

  r = renderinfo_new(&info);
  if (r < 0)
    return r;

  r = particles_new(450, &p);
  if (r < 0) {
    renderinfo_unref(info);
    return r;
  }

  p->duration = 1.0f;
  p->angle_variance = 45.0f; /* or radians? hgmm */
  p->start_color = (vec4){ .x = 1.0f, .y = 0.1f, .z = 0.14f, .w = 1.0f };
  p->start_color_variance = (vec4){ .x = 0.0f, .y = 0.025f, .z = 0.04f, .w = 0.0f };
  p->linear_attenuation = true;
  p->gravity = (vec3){ .x = -0.15f, .y = -0.15f, .z = 0.0f };
  p->tangential_acceleration = 5.0f;
  p->radial_acceleration = 20.0f;
  p->radial_acceleration_variance = 10.0f;
  p->texture = this->particle_;
  p->active = true;
  p->speed = 20.0f;
  p->emission_rate = 500.0f;
  p->life = 0.7f;

  p->primitive.spatial.position = (union vector2d) { .x = 200.0f, .y = 200.0f };
  compute_model_matrix(p->primitive);

  this->sys_ = p;

  r = renderinfo_set_textures(info, &this->particle_, 1);
  if (r < 0)
    goto out;

  (void)renderinfo_set_camera(info,
                              (struct _Camera *)glm::value_ptr(this->camera_));
  (void)renderinfo_set_pass(info, (Pass *)pass);
  (void)renderinfo_set_blending(info, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                GL_FUNC_ADD);
  (void)renderinfo_particles(info);
  /* append created RenderInfo to the array */
  r = add_render_type(this->array_, info, p, RT_PARTICLES);
  (void)gettimeofday(&this->last_frame, NULL);
out:
  if (r < 0) {
    renderinfo_unref(info);
    particles_unref(p);
    return r;
  }

  return 0; 
}

/* create grid with single color */
int Window::CreateGrid() {
  SolidColor *grid = NULL;
  RenderInfo *info = NULL;
  Pass *pass = NULL;
  glm::vec2 min(0, 0), max(1024, 768);
  glm::vec4 color(0.3f, 0.2f, 0.4f, 0.6f);
  size_t sub = 25;
  int r;

  /* find effect */
  r = hashtable_find(this->shaders_, Effects[ET_SOLID_COLOR],
                     (void *)Names[ET_SOLID_COLOR], (void **)&pass);
  if (r < 0)
    return r;

  r = renderinfo_new(&info);
  if (r < 0)
    return r;

  r = coloredgrid_new((union vector2d *)glm::value_ptr(min),
                      (union vector2d *)glm::value_ptr(max), &sub,
                      (union vector4d *)glm::value_ptr(color), &grid);

  if (r < 0) {
    free((void *)info);
    return r;
  }

  /* compute model matrix and set render info */
  compute_model_matrix(grid->primitive);
  (void)renderinfo_set_camera(info,
                              (struct _Camera *)glm::value_ptr(this->camera_));
  (void)renderinfo_set_pass(info, (Pass *)pass);
  (void)renderinfo_set_blending(info, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                GL_FUNC_ADD);
  (void)renderinfo_coloredgrid(info, sub);

  /* append created RenderInfo to the array */
  r = add_render_type(this->array_, info, grid, RT_SOLID_COLOR);
out:
  if (r < 0) {
    renderinfo_unref(info);
    free((void *)grid);
    /* TODO: cleanup VBO/VAO */
    return r;
  }

  return 0;
}

/* create logo from the bound texture and render as solid texture */
int Window::CreateLogo() {
  TexturedQuad *quad = NULL;
  RenderInfo *info = NULL;
  Pass *pass = NULL;
  glm::vec2 min(0, 0), max(83, 43);
  int r;

  /* find effect */
  r = hashtable_find(this->shaders_, Effects[ET_SOLID_TEXTURE],
                     (void *)Names[ET_SOLID_TEXTURE], (void **)&pass);
  if (r < 0)
    return r;

  r = renderinfo_new(&info);
  if (r < 0)
    return r;

  r = texturedquad_new((union vector2d *)glm::value_ptr(min),
                       (union vector2d *)glm::value_ptr(max), &quad);
  if (r < 0) {
    free((void *)info);
    return r;
  }

  /* store primitive and compute model matrix  */
  this->logo_ = &quad->primitive;
  compute_model_matrix(quad->primitive);

  r = renderinfo_set_textures(info, &this->texture_, 1);
  if (r < 0)
    goto out;

  /* setters don't really return anything besides 0 at this point */
  (void)renderinfo_set_camera(info,
                              (struct _Camera *)glm::value_ptr(this->camera_));
  (void)renderinfo_set_pass(info, (Pass *)pass);
  (void)renderinfo_set_blending(info, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                GL_FUNC_ADD);
  (void)renderinfo_texturedquad(info);

  /* render on top */
  (void)renderinfo_set_order(info, 1);

  /* append created RenderInfo to the array */
  r = add_render_type(this->array_, info, quad, RT_TEXTURED_QUAD);

out:
  if (r < 0) {
    renderinfo_unref(info);
    free((void *)quad);
    /* TODO: cleanup VBO/VAO */
    return r;
  }

  return 0;
}

int main(int argc, const char *argv[]) {
  ResourceManager *rm = NULL;
  Pass *p = NULL;
  Window *win;
  glm::ivec2 dim(1024, 768);
  int r;

  char *cwd = (char *)malloc(PATH_MAX);
  getcwd(cwd, PATH_MAX);

  /* this probably throws on failure, hmm */
  win = new Window(dim);
  if (!win)
    return -ENOMEM;

  r = resource_manager_new(cwd, &rm);
  if (r < 0)
    goto free;

  r = sdl_video_init();
  if (r < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    goto free;
  }

  /* orthographic camera */
  win->camera_ = glm::ortho(0.f, (float)dim.x, 0.f, (float)dim.y, -1.f, 1.f);
  win->CreateSDLWindow();

  /* load logo texture */
  (void)texture_new(&win->texture_);
  r = texture_load_from(win->texture_, rm, "res/textures/7.png");
  if (r < 0) {
    printf("Error creating texture: %i\n", r);
    r = -EINVAL;
    goto free;
  }

  /* load logo texture */
  (void)texture_new(&win->particle_);
  r = texture_load_from(win->particle_, rm, "res/textures/texture.png");
  if (r < 0) {
    printf("Error creating texture: %i\n", r);
    r = -EINVAL;
    goto free;
  }

  /* load texturing effect */
  r = create_effect("res/effects/solid-texture.json", rm, NULL, &p);
  if (r < 0) {
    printf("Error creating effect: %i\n", r);
    r = -EINVAL;
    goto free;
  }
  (void)hashtable_add(win->shaders_, Effects[ET_SOLID_TEXTURE],
                      (void *)Names[ET_SOLID_TEXTURE], p);

  /* load solid color */
  r = create_effect("res/effects/solid-color.json", rm, NULL, &p);
  if (r < 0) {
    printf("Error creating effect: %i\n", r);
    r = -EINVAL;
    goto free;
  }
  (void)hashtable_add(win->shaders_, Effects[ET_SOLID_COLOR],
                      (void *)Names[ET_SOLID_COLOR], p);

  /* load particles */
  r = create_effect("res/effects/particle.json", rm, NULL, &p);
  if (r < 0) {
    printf("Error creating effect: %i\n", r);
    r = -EINVAL;
    goto free;
  }
  (void)hashtable_add(win->shaders_, Effects[ET_PARTICLES],
                      (void *)Names[ET_PARTICLES], p);
  win->pp_ = p;


  /* create rendering primitives for the texture */
  //win->CreateLogo();
  /* create backing grid */
  //win->CreateGrid();
  /* create cpu-particles system */
  //win->CreateParticles();
  //
  //GLint range[2];
  //glGetIntegerv(GL_ALIASED_POINT_SIZE_RANGE, range);
  //printf("%i %i\n", range[0], range[1]);

  while (win->WindowLoop() == 0)
    usleep(3);

  SDL_DestroyWindow(win->window_);

free:
  SDL_Quit();
  free((void *)cwd);
  resource_manager_unref(rm);
  delete win;

  return r;
}
