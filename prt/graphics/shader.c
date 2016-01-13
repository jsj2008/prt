#include <prt/graphics/shader.h>
#include <prt/shared/json.h>
#include <GLES3/gl31.h>

/* @func `gl_shader_type`
 * @desc Converts `ShaderType` into GL shader type
 *
 * @param(type)  ShaderType type to convert trom
 *
 * @ret `GLuint` of the type
 */
GLuint gl_shader_type(ShaderType type) {
  switch (type) {
    case ST_VERTEX:
      return GL_VERTEX_SHADER;
    case ST_FRAGMENT:
      return GL_FRAGMENT_SHADER;
    case ST_COMPUTE:
      return GL_COMPUTE_SHADER;
    default:
      return GL_INVALID;
  }
}

/* @func `et_from_string`
 * @desc Converts `EffectType` from string representation
 *
 * @param(s)  String value
 *
 * @ret `EffectType` of the string
 */
EffectType et_from_string(const char *s) {
  __free_str char *p = str_to_lower(s);

  if (streq(p, "solid-color"))
    return ET_SOLID_COLOR;
  else if (streq(p, "solid-texture"))
    return ET_SOLID_TEXTURE;
  else if (streq(p, "depth-texture"))
    return ET_DEPTH_TEXTURE;
  else if (streq(p, "gbuffer"))
    return ET_GBUFFER;
  else if (streq(p, "cube-map"))
    return ET_CUBE_MAP;
  else if (streq(p, "point-light"))
    return ET_POINT_LIGHT;
  else if (streq(p, "directional-light"))
    return ET_DIRECTIONAL_LIGHT;

  return ET_NONE;
}

/* @func `svt_from_string`
 * @desc Converts `ShaderVariableType` from string representation
 *
 * @param(s)  String value
 *
 * @ret `ShaderVariableType` of the string
 */
ShaderVariableType svt_from_string(const char *s) {
  __free_str char *p = str_to_lower(s);

  if (streq(p, "float") || streq(p, "float1"))
    return SVT_FLOAT1;

  if (streq(p, "float2") || streq(p, "vec2f"))
    return SVT_FLOAT2;

  if (streq(p, "float3") || streq(p, "vec3f"))
    return SVT_FLOAT3;

  if (streq(p, "float4") || streq(p, "vec4f"))
    return SVT_FLOAT4;

  if (streq(p, "int") || streq(p, "int1") || streq(p, "texture"))
    return SVT_TEXTURE;

  if (streq(p, "int2") || streq(p, "vec2i"))
    return SVT_INT2;

  if (streq(p, "int3") || streq(p, "vec3i"))
    return SVT_INT3;

  if (streq(p, "int4") || streq(p, "vec4i"))
    return SVT_INT4;

  if (streq(p, "float3x3") || streq(p, "mat3f"))
    return SVT_MATRIX_3X3;

  if (streq(p, "float3x4"))
    return SVT_MATRIX_3X4;

  if (streq(p, "float4x4") || streq(p, "mat4f"))
    return SVT_MATRIX_4X4;

  return SVT_NONE;
}

/* @func `_load_effect_binding`
 * @desc Loads uniform/attribute binding from JSON into generic
 *`VariableBinding`
 *
 * @param(bindings)    JSON object describing the bindings
 * @param(out_binding) Created variable bindings
 * @param(out_size)    Size of the bindings
 *
 * @ret 0 on success or error code
 */
int _load_effect_binding(JsonVariant *bindings, VariableBinding **out_binding,
                         size_t *out_size) {
  JsonVariant *e, *v;
  VariableBinding *vb;
  const char *name, *type;
  ShaderVariableType u;
  size_t i;
  int r;
  assert(bindings);

  name = NULL;
  vb = (VariableBinding *)calloc(sizeof(VariableBinding), bindings->size);
  if (!vb)
    return -ENOMEM;

  for (i = 0; i < bindings->size; ++i) {
    v = json_variant_element(bindings, i);
    if (!v)
      goto err;

    e = json_variant_value(v, "name");
    if (!e)
      goto err;

    name = strdup(json_variant_string(e));
    if (!name)
      goto err;

    e = json_variant_value(v, "type");
    if (!e)
      goto err;

    type = json_variant_string(e);
    u = svt_from_string(type);
    if (u == SVT_NONE)
      goto err;

    vb[i].name = name;
    vb[i].type = u;
    name = NULL;
  }

  *out_binding = vb;
  *out_size = bindings->size;

  return 0;

err:
  free((void *)name);
  for (; i; --i)
    free((void *)vb[i].name);
  free((void *)vb);
  return -EINVAL;
}

/* @func `_load_effect_json`
 * @desc Parses JSON and evaluates description of effects
 *
 * @param(json)     JSON string data
 * @param(manager)  Resource manager, can be `NULL`
 * @param(pool)     `StringPool` to use during linking, can be `NULL`
 * @param(out_pass) Output shader pass, ready to be used
 *
 * @ret 0 on success or error code
 */
int _load_effect_json(FileResource *json, ResourceManager *manager,
                      StringPool *pool, Pass **out_pass) {
  JsonVariant *v, *e, *q, *n;
  char *s, *vertex, *fragment, *name;
  ShaderBinding *sb;
  Pass *pass;
  Shader *vtx, *frg;
  FileResource *fvert, *ffrag;
  EffectType type;
  size_t numuniforms, numattrs;
  int r;
  intmax_t version;
  assert(json);

  sb = NEW0(ShaderBinding);
  if (!sb)
    return -ENOMEM;

  r = json_parse(json->data, json->size, &v);
  if (r < 0) {
    _Log(LL_ERROR, "Invalid JSON");
    goto err;
  }

  e = json_variant_value(v, "version");
  if (!e) {
    _Log(LL_ERROR, "Invalid `version` element");
    goto err;
  }

  if (json_variant_integer(e) != 1) {
    _Log(LL_ERROR, "Invalid version number");
    goto err;
  }

  e = json_variant_value(v, "name");
  if (!e) {
    _Log(LL_ERROR, "Invalid `name` element");
    goto err;
  }
  name = json_variant_string(e);

  e = json_variant_value(v, "vertex");
  if (!e) {
    _Log(LL_ERROR, "Invalid `vertex` element");
    goto err;
  }
  vertex = json_variant_string(e);

  e = json_variant_value(v, "fragment");
  if (!e) {
    _Log(LL_ERROR, "Invalid `fragment` element");
    goto err;
  }
  fragment = json_variant_string(e);

  e = json_variant_value(v, "uniforms");
  if (!e) {
    _Log(LL_ERROR, "Invalid `uniforms` element");
    goto err;
  }
  r = _load_effect_binding(e, &sb->uniforms, &sb->num_uniforms);
  if (r < 0) {
    _Log(LL_ERROR, "Error creating uniform bindings");
    goto err;
  }
  e = json_variant_value(v, "attributes");
  if (!e) {
    _Log(LL_ERROR, "Invalid `attributes` element");
    goto err;
  }
  r = _load_effect_binding(e, &sb->attributes, &sb->num_attributes);
  if (r < 0) {
    _Log(LL_ERROR, "Error creating attribute bindings");
    goto err;
  }

  if (manager)
    r = resource_manager_load(manager, vertex, &fvert);
  else
    r = file_resource_new(vertex, &fvert);
  if (r < 0) {
    _Log(LL_ERROR, "Invalid vertex shader resource");
    goto resource_error;
  }

  if (manager)
    r = resource_manager_load(manager, fragment, &ffrag);
  else
    r = file_resource_new(fragment, &ffrag);
  if (r < 0) {
    _Log(LL_ERROR, "Invalid fragment shader resource");
    goto resource_error;
  }

  type = et_from_string(name);
  r = shader_pass_new(type, &pass);
  if (r < 0) {
    _Log(LL_ERROR, "Error creating effect pass");
    goto resource_error;
  }

  r = shader_new(ST_VERTEX, &vtx);
  if (r < 0) {
    _Log(LL_ERROR, "Error creating effect vertex shader");
    goto resource_error;
  }

  r = shader_load_from(vtx, fvert);
  if (r < 0) {
    _Log(LL_ERROR, "Error loading effect vertex shader");
    goto resource_error;
  }

  r = shader_pass_add_shader(pass, vtx);
  if (r < 0) {
    _Log(LL_ERROR, "Error adding vertex shader to pass");
    goto free_vtx;
  }

  r = shader_new(ST_FRAGMENT, &frg);
  if (r < 0) {
    _Log(LL_ERROR, "Error creating effect fragment shader");
    goto free_vtx;
  }

  r = shader_load_from(frg, ffrag);
  if (r < 0) {
    _Log(LL_ERROR, "Error loading effect fragment shader");
    goto free_vtx;
  }

  r = shader_pass_add_shader(pass, frg);
  if (r < 0) {
    _Log(LL_ERROR, "Error adding fragment shader to pass");
    goto free_frg;
  }

  r = shader_pass_link(pass, sb, pool);
  if (r < 0) {
    _Log(LL_ERROR, "Error creating OpenGL program");
    goto free_frg;
  }

  if (!manager) {
    (void)file_resource_unref(fvert);
    (void)file_resource_unref(ffrag);
  }

  free((void *)sb);

  *out_pass = pass;
  Log("Effect '%s' (0x%x) loaded", name, pass->type);
  (void)json_variant_unref(v);

  return 0;

free_frg:
  (void)shader_unref(frg);

free_vtx:
  (void)shader_unref(vtx);

resource_error:
  if (manager) {
    (void)resource_manager_remove(manager, fvert);
    (void)resource_manager_remove(manager, ffrag);
  } else {
    (void)file_resource_unref(fvert);
    (void)file_resource_unref(ffrag);
  }

err:
  free((void *)sb);
  free((void *)pass);
  json_variant_unref(v);

  return r;
}

/* @func `create_effect`
 * @desc Convenience function to load effect description from JSON object that
 *       can use @param(manager) as resource cache.
 *
 * @param(frompath) Relative path to the resource
 * @param(manager)  Resource manager, can be `NULL`
 * @param(pool)     String pool to use during linking, can be `NULL`
 * @param(out_pass) New *linked* effect from specification
 *
 * @ret 0 on success or error code
 */
int create_effect(const char *frompath, ResourceManager *manager,
                  StringPool *pool, Pass **out_pass) {
  FileResource *fr;
  int r;
  assert(frompath);

  if (manager)
    r = resource_manager_load(manager, frompath, &fr);
  else
    r = file_resource_new(frompath, &fr);

  if (r < 0)
    return r;

  r = _load_effect_json(fr, manager, pool, out_pass);
  if (r < 0) {
    if (manager)
      (void)resource_manager_remove(manager, fr);
    else
      (void)file_resource_unref(fr);

    return r;
  }

  if (!manager)
    (void)file_resource_unref(fr);

  return 0;
}

/* @func `set_uniform`
 * @desc `ShaderVariableType` based generic uniform setter
 *
 * @param(type)     Type of the variable
 * @param(location) Bound location of the uniform
 * @param(value)    Value
 *
 * @ret 0 on success or error code
 */
int set_uniform(ShaderVariableType type, int32_t location, void *value) {
  switch (type) {
  case SVT_TEXTURE:
    glUniform1i(location, PTR_TO_INT(value));
    break;
  case SVT_INT2:
    glUniform2iv(location, 1, (GLint *)value);
    break;
  case SVT_INT3:
    glUniform3iv(location, 1, (GLint *)value);
    break;
  case SVT_INT4:
    glUniform4iv(location, 1, (GLint *)value);
    break;
  case SVT_FLOAT1:
    glUniform1fv(location, 1, (GLfloat *)value);
    break;
  case SVT_FLOAT2:
    glUniform2fv(location, 1, (GLfloat *)value);
    break;
  case SVT_FLOAT3:
    glUniform3fv(location, 1, (GLfloat *)value);
    break;
  case SVT_FLOAT4:
    glUniform4fv(location, 1, (GLfloat *)value);
    break;
  case SVT_MATRIX_3X3:
    glUniformMatrix3fv(location, 1, GL_FALSE, (GLfloat *)value);
    break;
  case SVT_MATRIX_3X4:
    glUniformMatrix4x3fv(location, 1, GL_FALSE, (GLfloat *)value);
    break;
  case SVT_MATRIX_4X4:
    glUniformMatrix4fv(location, 1, GL_FALSE, (GLfloat *)value);
    break;
  default:
    return -EINVAL;
  }
  return 0;
}

/* @func `shader_new`
 * @desc Allocates a new shader with given type
 *
 * @param(type)       Type of the shader
 * @param(out_shader) Receives the created shader
 *
 * @ret 0 on success or error code
 */
int shader_new(ShaderType type, Shader **out_shader) {
  Shader *sh;

  sh = NEW0(Shader);
  if (!sh)
    return -ENOMEM;
  sh->type = type;
  *out_shader = sh;

  return 0;
}

/* @func `shader_load_from`
 * @desc Loads shader source from given file resource `fr`
 *
 * @param(shader) Shader object
 * @param(fr)     File resource to load from
 *
 * @ret 0 on success or error code
 */
int shader_load_from(Shader *shader, FileResource *fr) {
  GLuint sh = 0;
  GLint compiled = 0;
  GLint size[] = {fr->size - 1};
  GLenum type;

  type = gl_shader_type(shader->type);
  sh = glCreateShader(type);

  if (sh == 0)
    return -EIO;

  glShaderSource(sh, 1, (const char *const *)&fr->data, size);
  glCompileShader(sh);
  glGetShaderiv(sh, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infolen = 0;

    glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &infolen);
    if (infolen > 1) {
      char *infolog = (char *)malloc(infolen);
      if (!infolog) {
        glDeleteShader(sh);
        return -ENOMEM;
      }

      glGetShaderInfoLog(sh, infolen, NULL, infolog);
      printf("Error compiling shader:\n%s\n", infolog);
      free((void *)infolog);
    }

    glDeleteShader(sh);
    return -EINVAL;
  }

  shader->program = sh;

  return 0;
}

/* @func `shader_unref`
 * @desc Deletes a shader
 *
 * @param(shader) Shader to delete
 *
 * @ret 0 on success or error code
 */
int shader_unref(Shader *shader) {
  if (shader->program)
    glDeleteShader(shader->program);
  free((void *)shader);
  return 0;
}

/* @func `shader_pass_new`
 * @desc Create a new shader effect pass
 *
 * @param(type)     Type of the pass
 * @param(out_pass) Receives the created pass
 *
 * @ret 0 on success or error code
 */
int shader_pass_new(EffectType type, Pass **out_pass) {
  Pass *p;

  p = NEW0(Pass);
  if (!p)
    return -ENOMEM;

  p->type = type;

  *out_pass = p;

  return 0;
}

/* @func `shader_pass_add_shader`
 * @desc Adds instantiated shader to the effect pass
 *
 * @param(pass)   Shader effect pass
 * @param(shader) Shader to add to the pass
 *
 * @ret 0 on success or error code
 */
int shader_pass_add_shader(Pass *pass, Shader *shader) {
  assert(pass);

  if (shader->type == ST_VERTEX && pass->vertex.program != 0)
    return -EINVAL;

  if (shader->type == ST_FRAGMENT && pass->fragment.program != 0)
    return -EINVAL;

  if (shader->type == ST_VERTEX) {
    pass->vertex.program = shader->program;
    pass->vertex.type = ST_VERTEX;
  } else if (shader->type == ST_FRAGMENT) {
    pass->fragment.program = shader->program;
    pass->fragment.type = ST_FRAGMENT;
  }

  free((void *)shader);

  return 0;
}

/* @func `shader_pass_link`
 * @desc Links the shader effect pass shaders
 *
 * @param(pass)    Effect pass to link
 * @param(binding) Attribute and uniform binding
 * @param(pool)    Optional `StringPool` where to store uniform names
 *
 * @ret 0 on success or error code
 */
int shader_pass_link(Pass *pass, ShaderBinding *binding, StringPool *pool) {
  FastHashBuilder *builder;
  FastHash *fh;
  UniformBinding ub;
  size_t *indices;
  uint32_t program;
  int32_t linked, location;
  int r;
  size_t i, index;
  assert(pass);
  assert(binding);

  if (!pass->fragment.program || !pass->vertex.program)
    return -EINVAL;

  program = glCreateProgram();
  if (program == 0)
    return -EIO;

  glAttachShader(program, pass->vertex.program);
  glAttachShader(program, pass->fragment.program);

  for (i = 0; i < binding->num_attributes; ++i)
    glBindAttribLocation(program, i, binding->attributes[i].name);

  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint infolen = 0;

    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infolen);
    if (infolen > 1) {
      char *infolog = (char *)malloc(infolen);

      if (!infolog) {
        glDeleteProgram(program);
        return -ENOMEM;
      }

      glGetProgramInfoLog(program, infolen, NULL, infolog);
      printf("Error linking program:\n%s\n", infolog);
      free((void *)infolog);
    }

    glDeleteProgram(program);
    return -EINVAL;
  }

  i = 0;
  r = fasthash_builder_new(&builder);
  if (r < 0)
    goto err;

  pass->shader_program = program;
  if (pool)
    indices = alloca(sizeof(*indices) * binding->num_uniforms);

  for (i = 0; i < binding->num_uniforms; ++i) {
    location = glGetUniformLocation(program, binding->uniforms[i].name);

    if (pool) {
      r = string_pool_add(pool, strdup(binding->uniforms[i].name), &index);
      if (r < 0)
        goto err;
      indices[i] = index;
    } else
      index = 0;

    ub.literal = index;
    ub.location = location;
    ub.type = binding->uniforms[i].type;

    r = fasthash_builder_add(
        builder, ULONG_TO_PTR(HASH(binding->uniforms[i].name)), ub.pointer);
    if (r < 0) {
      i += 1;
      goto err;
    }
  }

  r = fasthash_build(builder, &fh);
  if (r < 0) {
    i += 1;
    goto err;
  }

  pass->uniforms = fh;
  fasthash_builder_unref(builder);

  return 0;
err:
  if (pool)
    for (--i; i != (size_t)-1; --i)
      (void)string_pool_remove(pool, indices[i]);
  fasthash_builder_unref(builder);
  glDeleteProgram(program);
  return r;
}

/* @func `shader_pass_ready`
 * @desc Determines whether the pass can be used for rendering
 *
 * @param(pass) Shader effect pass
 *
 * @ret 0 when ready or error code
 */
int shader_pass_ready(Pass *pass) {
  if (pass->fragment.program == 0 || pass->vertex.program == 0)
    return -EINVAL;

  if (pass->shader_program == 0)
    return -ENOENT;

  if (!pass->uniforms)
    return -EFAULT;

  return 0;
}

/* @func `shader_pass_set_uniform`
 * @desc Sets uniform with `name` to `value` for given `pass`
 *
 * @param(pass)  Shader effect pass
 * @param(name)  Name of the uniform
 * @param(value) Value of the uniform
 *
 * @ret 0 on success or error code
 */
int shader_pass_set_uniform(Pass *pass, Id name, void *value) {
  UniformBinding ub;
  int r;
  assert(pass);

  r = fasthash_find(pass->uniforms, ULONG_TO_PTR(name), &ub.pointer);
  if (r == 0)
    return set_uniform(ub.type, ub.location, value);

  return r;
}

/* @func `shader_pass_unref`
 * @desc Deletes the shader effect pass
 *
 * @param(pass) Shader effect pass
 * @param(pool) Optional `StringPool` instance that was used
 * during `pass` linking
 *
 * @ret
 */
int shader_pass_unref(Pass *pass, StringPool *pool) {
  size_t i;
  UniformBinding ub;
  assert(pass);

  if (pool) {
    for (i = 1; i < pass->uniforms->num_items; i += 2) {
      ub.pointer = *((void **)pass->uniforms->items + i);
      (void)string_pool_remove(pool, ub.literal);
    }
  }

  fasthash_unref(pass->uniforms);
  free((void *)pass);
  return 0;
}
