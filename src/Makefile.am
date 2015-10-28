lib_LTLIBRARIES = libprt.la
libprt_la_SOURCES = runtime/lock.c shared/json.c shared/avl_tree.c shared/basic.c shared/fast_hash.c shared/bit_vector.c shared/sparse_hash.c shared/hashtable.c shared/popcnt.c shared/kd_tree.c runtime/resource_manager.c runtime/resources.c graphics/texture.c graphics/shader.c graphics/renderbuffer.c graphics/framebuffer.c graphics/common.c shared/pool.c engine/render.c graphics/rendering.c shared/array.c engine/mesh.c engine/particles.c
include_HEADERS = shared/basic.h graphics/common.h
libprt_la_CFLAGS = -I../
libprt_la_LDFLAGS = -lassimp -lm -GL -lpthread

AUTOMAKE_OPTIONS = subdir-objects
