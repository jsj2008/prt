lib_LTLIBRARIES = libprt.la
libprt_la_SOURCES = runtime/lock.c shared/json.c shared/avl_tree.c shared/basic.c shared/fast_hash.c shared/bit_vector.c shared/sparse_hash.c shared/hashtable.c shared/popcnt.c shared/kd_tree.c runtime/resource_manager.c runtime/resources.c graphics/texture.c graphics/shader.c graphics/renderbuffer.c graphics/framebuffer.c graphics/common.c shared/pool.c engine/render.c graphics/rendering.c shared/array.c engine/mesh.c engine/particles.c
nobase_pkginclude_HEADERS = graphics/texture.h graphics/renderbuffer.h graphics/common.h graphics/rendering.h graphics/framebuffer.h graphics/shader.h engine/particles.h engine/mesh.h engine/render.h runtime/resource_manager.h runtime/resources.h runtime/lock.h shared/refcounted.h shared/hashtable.h shared/avl_tree.h shared/bit_vector.h shared/json.h shared/fast_hash.h shared/sparse_hash.h shared/pool.h shared/popcnt.h shared/array.h shared/basic.h shared/kd_tree.h shared/list.h shared/config.h
libprt_la_CFLAGS = -I../
libprt_la_LDFLAGS = -lassimp -lm -lGL -lpthread

AUTOMAKE_OPTIONS = subdir-objects
