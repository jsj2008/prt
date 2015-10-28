/***
  This file is part of systemd.
  Copyright 2014 Lennart Poettering
  Copyright 2015 Pavel Odvody
  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.
  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  JSON_END,
  JSON_COLON,
  JSON_COMMA,
  JSON_OBJECT_OPEN,
  JSON_OBJECT_CLOSE,
  JSON_ARRAY_OPEN,
  JSON_ARRAY_CLOSE,
  JSON_STRING,
  JSON_REAL,
  JSON_INTEGER,
  JSON_BOOLEAN,
  JSON_NULL,
};

typedef enum {
  JSON_VARIANT_CONTROL,
  JSON_VARIANT_STRING,
  JSON_VARIANT_INTEGER,
  JSON_VARIANT_BOOLEAN,
  JSON_VARIANT_REAL,
  JSON_VARIANT_ARRAY,
  JSON_VARIANT_OBJECT,
  JSON_VARIANT_NULL
} JsonVariantType;

union json_value {
  bool boolean;
  double real;
  intmax_t integer;
};

typedef struct _JsonVariant {
  JsonVariantType type;
  size_t size;
  union {
    char *string;
    struct _JsonVariant *objects;
    union json_value value;
  };
} JsonVariant;

int json_variant_new(JsonVariant **ret, JsonVariantType type);
JsonVariant *json_variant_unref(JsonVariant *v);

char *json_variant_string(JsonVariant *v);
bool json_variant_bool(JsonVariant *v);
intmax_t json_variant_integer(JsonVariant *v);
double json_variant_real(JsonVariant *v);

JsonVariant *json_variant_element(JsonVariant *v, unsigned index);
JsonVariant *json_variant_value(JsonVariant *v, const char *key);

#define JSON_VALUE_NULL ((union json_value){})

int json_tokenize(const char **p, char **ret_string,
                  union json_value *ret_value, void **state, unsigned *line);

int json_parse(const char *string, size_t size, JsonVariant **rv);

#ifdef __cplusplus
}
#endif
