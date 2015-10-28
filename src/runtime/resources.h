#pragma once

#include <src/shared/basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FileResource {
  char *data;
  size_t size;
  const char *path;
} FileResource;

#define PRT_READ_BUFFER_SIZE 0x3fff

int load_resource(const char *name, char **out_data, size_t *out_size);
int file_resource_new(const char *path, FileResource **out_fr);
int file_resource_unref(FileResource *fr);

#ifdef __cplusplus
}
#endif
