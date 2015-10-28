#pragma once

#include <src/engine/render.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ParticleStats {
  PS_PARTICLE_SIZE = (sizeof(union vector3d) * 2) +
                     (sizeof(union vector4d) * 2) + (sizeof(float) * 5),
  PS_POSITION_STRIDE = 0,
  PS_COLOR_STRIDE = sizeof(union vector3d),
  PS_DIRECTION_STRIDE = sizeof(union vector4d) + PS_COLOR_STRIDE,
  PS_DELTA_COLOR_STRIDE = sizeof(union vector3d) + PS_DIRECTION_STRIDE,
  PS_RADIAL_ACCELERATION_STRIDE =
      sizeof(union vector4d) + PS_DELTA_COLOR_STRIDE,
  PS_TANGENTIAL_ACCELERATION_STRIDE =
      sizeof(float) + PS_RADIAL_ACCELERATION_STRIDE,
  PS_SIZE_STRIDE = sizeof(float) + PS_TANGENTIAL_ACCELERATION_STRIDE,
  PS_LIFE_STRIDE = sizeof(float) + PS_SIZE_STRIDE,
  PS_LINEAR_ATTENUATION_STRIDE = sizeof(float) + PS_LIFE_STRIDE
} ParticleStats;

typedef struct _ParticleData {
  union vector3d *position;
  union vector4d *color;
  // --
  union vector3d *direction;
  union vector4d *delta_color;
  float *radial_acceleration;
  float *tangential_acceleration;
  float *size;
  float *life;
  float *linear_attenuation;
} ParticleData;

typedef struct _Particles {
  ParticleData data;
  void *data_buffer;
  GPUTexture *texture;
  Pass *shader;
  struct primitive2d primitive;

  float duration;
  float elapsed;
  float angle;
  float angle_variance;
  float speed;
  float speed_variance;
  float tangential_acceleration;
  float tangential_acceleration_variance;
  float radial_acceleration;
  float radial_acceleration_variance;
  float size;
  float size_variance;
  float life;
  float life_variance;
  float emission_rate;
  float emit_counter;

  union vector4d start_color;
  union vector4d start_color_variance;
  union vector4d end_color;
  union vector4d end_color_variance;

  union vector3d position;
  union vector3d position_variance;
  union vector3d gravity;

  size_t count_limit;
  size_t particle_count;
  size_t particle_index;

  uint32_t vbo_positions;
  uint32_t vbo_colors;
  uint32_t vao;

  bool active : 1;
  bool blend_additive : 1;
  bool color_modulate : 1;
  bool linear_attenuation : 1;
} Particles;

int particles_new(size_t count, Particles **out_particles);
int particles_init_particle(Particles *particles, size_t index);
int particles_tick(Particles *particles, float dt);
int particles_unref(Particles *particles);

#ifdef __cplusplus
}
#endif
