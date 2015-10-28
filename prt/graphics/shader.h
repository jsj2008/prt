#pragma once

#include <prt/shared/basic.h>
#include <prt/shared/fast_hash.h>
#include <prt/shared/pool.h>
#include <prt/runtime/resource_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ShaderVariableType {
  SVT_NONE = 0,

  SVT_FLOAT1 = 1,
  SVT_FLOAT2 = 2,
  SVT_FLOAT3 = 3,
  SVT_FLOAT4 = 4,

  SVT_MATRIX_3X3 = 5,
  SVT_MATRIX_3X4 = 6,
  SVT_MATRIX_4X4 = 7,

  SVT_INT1 = 8,
  SVT_INT2 = 9,
  SVT_INT3 = 10,
  SVT_INT4 = 11,

  SVT_TEXTURE = SVT_INT1,

  SVT_MAX = SVT_INT4
} ShaderVariableType;

typedef enum _EffectType {
  ET_NONE = 0,
  ET_SOLID_TEXTURE = 1,
  ET_GBUFFER = 2,
  ET_DEPTH_TEXTURE = 3,
  ET_SOLID_COLOR = 4,
  ET_CUBE_MAP = 5,
  ET_POINT_LIGHT = 6,
  ET_DIRECTIONAL_LIGHT = 7,

  _ET_MAX = ET_DIRECTIONAL_LIGHT
} EffectType;

typedef enum _ShaderType {
  ST_NONE = 0,

  ST_VERTEX = 1,
  ST_FRAGMENT = 2,
  ST_GEOMETRY = 4,
  ST_TESSELATION = 8,
  ST_DOMAIN = 16,
  ST_COMPUTE = 32,

  _ST_MAX = ST_COMPUTE
} ShaderType;

typedef struct _Shader {
  uint32_t program;
  ShaderType type;
} Shader;

typedef struct _Pass {
  Shader vertex;
  Shader fragment;
  uint32_t shader_program;
  FastHash *uniforms;
  EffectType type;
} Pass;

typedef struct _UniformBinding {
  union {
    void *pointer;
    struct {
      int32_t location;
      uint16_t type;
      uint16_t literal;
    };
  };
} UniformBinding;

typedef struct _VariableBinding {
  const char *name;
  ShaderVariableType type;
} VariableBinding;

typedef struct _ShaderBinding {
  VariableBinding *attributes;
  size_t num_attributes;
  VariableBinding *uniforms;
  size_t num_uniforms;
} ShaderBinding;

EffectType et_from_string(const char *s);
ShaderVariableType svt_from_string(const char *s);

int set_uniform(ShaderVariableType type, int32_t location, void *value);
int create_effect(const char *frompath, ResourceManager *manager,
                  StringPool *pool, Pass **out_pass);

int shader_new(ShaderType type, Shader **out_shader);
int shader_load_from(Shader *shader, FileResource *fr);
int shader_unref(Shader *shader);

int shader_pass_new(EffectType type, Pass **out_pass);

/* deletes the incoming `shader` argument */
int shader_pass_add_shader(Pass *pass, Shader *shader);
int shader_pass_link(Pass *pass, ShaderBinding *binding, StringPool *pool);
int shader_pass_ready(Pass *pass);
int shader_pass_set_uniform(Pass *pass, Id name, void *value);
int shader_pass_unref(Pass *pass, StringPool *pool);

#ifdef __cplusplus
}
#endif
