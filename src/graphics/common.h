#pragma once

#include <src/shared/basic.h>
#include <GLES3/gl31.h>
#include <GLES3/gl3platform.h>
#include <GLES2/gl2ext.h>

#ifdef __cplusplus
extern "C" {
#endif

void CheckOpenGLError(const char *stmt, const char *fname, int line);

#define _DEBUG

#ifdef _DEBUG
#define GL_CHECK(stmt)                                                         \
  do {                                                                         \
    stmt;                                                                      \
    CheckOpenGLError(#stmt, __FILE__, __LINE__);                               \
  } while (0)
#else
#define GL_CHECK(stmt) stmt
#endif

typedef enum _VertexAttrib {
  ATTR_NONE = -1,

  ATTR_VERTEX = 0,
  ATTR_UV = 1 << 0,
  ATTR_NORMALS = 1 << 1,
  ATTR_TANGENT = 1 << 2,
  ATTR_COLOR = 1 << 3,

  _ATTR_MAX = ATTR_COLOR
} VertexAttrib;

typedef struct _BlendMode {
  union {
    uint16_t value;
    struct {
      uint32_t source : 5;
      uint32_t destination : 5;
      uint32_t func : 3;
      uint32_t padXXX : 3;
    };
  };
} BlendMode;

#ifdef __cplusplus
}
#endif
