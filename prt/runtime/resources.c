#include <prt/runtime/resources.h>

int load_resource(const char *name, char **out_data, size_t *out_size) {
  char buf[PRT_READ_BUFFER_SIZE] = {0};
  char *fp = NULL;
  FILE *fd = NULL;
  char *d = NULL, *dup = NULL;
  size_t read = 0, total = 0;

  fd = fopen(name, "rb");
  if (!fd)
    return -EIO;

  read = fread(buf, 1, sizeof(buf), fd);
  do {
    total += read;
    d = (char *)realloc(d, total);
    if (!d)
      goto nomem;
    memcpy(d + (total - read), buf, read);
  } while ((read = fread(buf, 1, sizeof(buf), fd)) > 0);

  fclose(fd);

  *out_data = d;
  *out_size = total;

  return 0;
nomem:
  free((void *)dup);
  free((void *)d);
  fclose(fd);
  return -ENOMEM;
}

int file_resource_new(const char *path, FileResource **out_fre) {
  FileResource *fr;
  int r;
  assert(path);

  fr = NEW0(FileResource);
  if (!fr)
    return -ENOMEM;

  fr->path = strdup(path);
  if (!fr->path) {
    free((void *)fr);
    return -ENOMEM;
  }

  r = load_resource(path, &fr->data, &fr->size);
  if (r < 0) {
    free((void *)fr->path);
    free((void *)fr);
    return r;
  }

  *out_fre = fr;

  return 0;
}

int file_resource_unref(FileResource *fr) {
  free((void *)fr->data);
  free((void *)fr->path);
  free((void *)fr);
  return 0;
}
