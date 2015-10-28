#include <stdint.h>

typedef struct _Vec2i {
  int x;
  int y;
} Vec2i;

typedef struct _Vec2f {
  float x;
  float y;
} Vec2f;

typedef struct _Vec3f {
  float x;
  float y;
  float z;
} Vec3f;

typedef struct _Vec4f {
  float x;
  float y;
  float z;
  float w;
} Vec4f;

typedef struct _Mat3f {
  Vec3f a;
  Vec3f b;
  Vec3f c;
} Mat3f;

typedef struct _Mat4f {
  Vec4f a;
  Vec4f b;
  Vec4f c;
  Vec4f d;
} Mat4f;

Vec2i vec2i_add(Vec2i a, Vec2i b);
Vec2i vec2i_sub(Vec2i a, Vec2i b);

Vec2i vec2i_add(Vec2i a, Vec2i b) {
  Vec2i t;
  t.x = a.x + b.x;
  t.y = a.y + b.y;
  return t;
}

Vec2i vec2i_sub(Vec2i a, Vec2i b) {
  Vec2i t;
  t.x = a.x - b.x;
  t.y = a.y - b.y;
  return t;
}
