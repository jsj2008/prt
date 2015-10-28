#include <prt/engine/particles.h>

#define RAND() ((random() / (float)0x3fffffff) - 1.0f)
#define DEG_TO_RAD(__ANGLE__) ((__ANGLE__) / 180.0f * (float)M_PI)

static int _swap_particle(Particles *p, size_t a, size_t b) {
  union vector4d tcolor, tdelta_color;
  union vector3d tposition, tdirection;
  float tradial, ttangential, tsize, tlife, tlinear;

  /* save a in tmp */
  tcolor = p->data.color[a];
  tdelta_color = p->data.delta_color[a];
  tposition = p->data.position[a];
  tdirection = p->data.direction[a];
  tradial = p->data.radial_acceleration[a];
  ttangential = p->data.tangential_acceleration[a];
  tsize = p->data.size[a];
  tlife = p->data.life[a];
  tlinear = p->data.linear_attenuation[a];

  /* a <- b */
  p->data.color[a] = p->data.color[b];
  p->data.delta_color[a] = p->data.delta_color[b];
  p->data.position[a] = p->data.position[b];
  p->data.direction[a] = p->data.direction[b];
  p->data.radial_acceleration[a] = p->data.radial_acceleration[b];
  p->data.tangential_acceleration[a] = p->data.tangential_acceleration[b];
  p->data.size[a] = p->data.size[b];
  p->data.life[a] = p->data.life[b];
  p->data.linear_attenuation[a] = p->data.linear_attenuation[b];

  /* tmp -> b */
  p->data.color[b] = tcolor;
  p->data.delta_color[b] = tdelta_color;
  p->data.position[b] = tposition;
  p->data.direction[b] = tdirection;
  p->data.radial_acceleration[b] = tradial;
  p->data.tangential_acceleration[b] = ttangential;
  p->data.size[b] = tsize;
  p->data.life[b] = tlife;
  p->data.linear_attenuation[b] = tlinear;

  return 0;
}

int particles_new(size_t count, Particles **out_particles) {
  Particles *p;

  p = NEW0(Particles);
  if (!p)
    return -ENOMEM;

  p->primitive.model_invalid = true;

  /* prepare gpu buffer names */
  GL_CHECK(glGenVertexArrays(1, &p->vao));
  GL_CHECK(glBindVertexArray(p->vao));
  GL_CHECK(glGenBuffers(1, &p->vbo_positions));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, p->vbo_positions));
  GL_CHECK(glEnableVertexAttribArray(ATTR_VERTEX));
  GL_CHECK(glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, 0, 0, (void *)0));
  GL_CHECK(glGenBuffers(1, &p->vbo_colors));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, p->vbo_colors));
  GL_CHECK(glEnableVertexAttribArray(3));
  GL_CHECK(glVertexAttribPointer(3, 4, GL_FLOAT, 0, 0, (void *)0));

  /* unbind buffers */
  GL_CHECK(glBindVertexArray(0));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
  GL_CHECK(glDisableVertexAttribArray(3));
  GL_CHECK(glDisableVertexAttribArray(ATTR_VERTEX));

  /* allocate single chunk for all particles */
  p->data_buffer = calloc(PS_PARTICLE_SIZE, count);
  if (!p->data_buffer) {
    free((void *)p);
    return -ENOMEM;
  }

  /* store offset information for all members */
  p->data.position =
      (union vector3d *)((char *)p->data_buffer + (PS_POSITION_STRIDE * count));
  p->data.color =
      (union vector4d *)((char *)p->data_buffer + (PS_COLOR_STRIDE * count));
  p->data.direction = (union vector3d *)((char *)p->data_buffer +
                                         (PS_DIRECTION_STRIDE * count));
  p->data.delta_color = (union vector4d *)((char *)p->data_buffer +
                                           (PS_DELTA_COLOR_STRIDE * count));
  p->data.radial_acceleration =
      (float *)((char *)p->data_buffer +
                (PS_RADIAL_ACCELERATION_STRIDE * count));
  p->data.tangential_acceleration =
      (float *)((char *)p->data_buffer +
                (PS_TANGENTIAL_ACCELERATION_STRIDE * count));
  p->data.size = (float *)((char *)p->data_buffer + (PS_SIZE_STRIDE * count));
  p->data.life = (float *)((char *)p->data_buffer + (PS_LIFE_STRIDE * count));
  p->data.linear_attenuation =
      (float *)((char *)p->data_buffer +
                (PS_LINEAR_ATTENUATION_STRIDE * count));

  p->count_limit = count;

  *out_particles = p;

  return 0;
}

int particles_init_particle(Particles *particles, size_t index) {
  ParticleData *pd;
  float speed;
  float angle;
  assert(particles);
  assert(particles->count_limit > index);

  pd = &particles->data;

  /* set initial position of the particle */
  pd->position[index].x =
      particles->position.x + particles->position_variance.x * RAND();
  pd->position[index].y =
      particles->position.y + particles->position_variance.y * RAND();
  pd->position[index].z =
      particles->position.z + particles->position_variance.z * RAND();

  printf("particles: x: %f y: %f %p\n", particles->position.x, particles->position.y, pd->position);
  //printf("x: %f y: %f\n", pd->position[index].x, pd->position[index].y);

  /* compute velocity/direction vector */
  speed = particles->speed + particles->speed_variance * RAND();
  angle =
      (float)DEG_TO_RAD(particles->angle + particles->angle_variance * RAND());

  /* move particles only in 2D for the time being */
  pd->direction[index].x = sinf(angle) * speed;
  pd->direction[index].y = cosf(angle) * speed;
  /* pd->direction[index].z = 0.f; */

  /* tangential/radial accelerations */
  pd->radial_acceleration[index] =
      particles->radial_acceleration +
      particles->radial_acceleration_variance * RAND();
  pd->tangential_acceleration[index] =
      particles->tangential_acceleration +
      particles->tangential_acceleration_variance * RAND();

  /* lifetime */
  pd->life[index] = particles->life + particles->life_variance * RAND();

  /* attenuate */
  if (particles->linear_attenuation)
    pd->linear_attenuation[index] = pd->life[index];

  /* start color */
  pd->color[index].x =
      particles->start_color.x + particles->start_color_variance.x * RAND();
  pd->color[index].y =
      particles->start_color.y + particles->start_color_variance.y * RAND();
  pd->color[index].z =
      particles->start_color.z + particles->start_color_variance.z * RAND();
  pd->color[index].w =
      particles->start_color.w + particles->start_color_variance.w * RAND();

  /* color delta = (color_end - color_start) / life */
  pd->delta_color[index].x =
      ((particles->end_color.x + particles->end_color_variance.x * RAND()) -
       pd->color[index].x) /
      pd->life[index];
  pd->delta_color[index].y =
      ((particles->end_color.y + particles->end_color_variance.y * RAND()) -
       pd->color[index].y) /
      pd->life[index];
  pd->delta_color[index].z =
      ((particles->end_color.z + particles->end_color_variance.z * RAND()) -
       pd->color[index].z) /
      pd->life[index];
  pd->delta_color[index].w =
      ((particles->end_color.w + particles->end_color_variance.w * RAND()) -
       pd->color[index].w) /
      pd->life[index];

  //printf("x: %f y: %f\n", pd->position[index].x, pd->position[index].y);
  return 0;
}

int particles_tick(Particles *particles, float dt) {
  ParticleData *pd;
  union vector3d radial;
  union vector3d tangential;
  union vector3d normalized;
  float rate, factor;
  size_t index;
  assert(particles);

  pd = &particles->data;
  puts("======================================== TICK =================================================");

  /* emit particles if active and emission rate above zero */
  if (particles->active && particles->emission_rate > 0.f) {
    rate = 1.f / particles->emission_rate;
    particles->emit_counter += dt;

    while (((particles->particle_count + 1) < particles->count_limit) &&
           (particles->emit_counter > rate)) {
      /* add particle */
      particles_init_particle(particles, particles->particle_count++);
      particles->emit_counter -= rate;
    }

    particles->elapsed += dt;
    /* turn off if our time elapsed */
    if (particles->duration != 1 && particles->duration < particles->elapsed) {
      particles->active = false;
      particles->emit_counter = 0.f;
      particles->elapsed = particles->duration;
    }
  }

  index = 0;
  while (index < particles->particle_count) {
    if (pd->life[index] > 0.0f) {
      printf("-- x: %f y: %f - %zu %zu %p %f\n", pd->position[index].x, pd->position[index].y, index, particles->particle_count, pd->position, dt);
      /* compute normalization factor */
      factor = (pd->position[index].x + pd->position[index].y);
      if (factor > 0.0f)
        factor = 1.0 / factor;
      else 
        factor = 1.0f;

//      printf("factor: %f\n", factor);

      /* swap axes in position */
      normalized.x = pd->position[index].y * factor;
      normalized.y = pd->position[index].x * factor;
      normalized.z = pd->position[index].z * factor;

      /* this works only for 2d */
      radial.x = normalized.x * pd->radial_acceleration[index];
      radial.y = normalized.y * pd->radial_acceleration[index];
      radial.z = normalized.z * pd->radial_acceleration[index];

      /* this works only for 2d */
      tangential.x = -normalized.y * pd->tangential_acceleration[index];
      tangential.y = normalized.x * pd->tangential_acceleration[index];
      tangential.z = normalized.z * pd->tangential_acceleration[index];

      /* direction influenced by gravity, radial & tangential accelerations */
      pd->direction[index].x =
          pd->direction[index].x +
          ((particles->gravity.x + radial.x + tangential.x) * dt);
      pd->direction[index].y =
          pd->direction[index].y +
          ((particles->gravity.y + radial.y + tangential.y) * dt);
      pd->direction[index].z =
          pd->direction[index].z +
          ((particles->gravity.z + radial.z + tangential.z) * dt);

      /* new position */
      pd->position[index].x += (pd->direction[index].x * particles->speed * dt);
      pd->position[index].y += (pd->direction[index].y * particles->speed * dt);

      /* compute new color */
      pd->color[index].x += (pd->delta_color[index].x * dt);
      pd->color[index].y += (pd->delta_color[index].y * dt);
      pd->color[index].z += (pd->delta_color[index].z * dt);
      pd->color[index].w += (pd->delta_color[index].w * dt);

      pd->life[index] -= dt;

      /* attenuate */
      if (particles->linear_attenuation)
        pd->size[index] =
            pd->size[index] * (pd->life[index] / pd->linear_attenuation[index]);

      /* the shader uses the z coordinate as point size */
      pd->position[index].z = pd->size[index]; // pd->position[index].z +
                                               // (pd->direction[index].z * dt);
      //printf("x: %f y: %f : %zu\n", pd->position[index].x, pd->position[index].y, index);

    } else {
      particles->particle_count--;

      /* swap out with the last particle */
      /* TODO: check if last particle is alive */
      if (index != particles->particle_count)
        _swap_particle(particles, index, particles->particle_count - 1);
    }

    index++;
  }

  /* update GPU buffers with the updated state */
  glBindBuffer(GL_ARRAY_BUFFER, particles->vbo_positions);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(union vector3d) * particles->particle_count,
               particles->data.position, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, particles->vbo_colors);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(union vector4d) * particles->particle_count,
               particles->data.color, GL_DYNAMIC_DRAW);

  return 0;
}

int particles_unref(Particles *particles) {
  return 0;
}
