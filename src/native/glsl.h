/*
 * TitanGL -- desktop GLSL to GLSL ES 3.00 translation.
 *
 * This file is part of TitanLD's own renderer core.  It is deliberately
 * independent of Android so that the translator can be built and tested on
 * the host.
 */
#ifndef TITANGL_GLSL_H
#define TITANGL_GLSL_H

#include <stddef.h>

#define TITANGL_MAX_ATTRIBUTES 32
#define TITANGL_MAX_ATTRIBUTE_NAME 96
#define TITANGL_MAX_FRAG_OUTPUTS 8

typedef struct {
    int location;
    char name[TITANGL_MAX_ATTRIBUTE_NAME];
} titangl_attribute;

typedef struct {
    char *text;
    size_t length;
    int source_version;
    int source_is_es;
    int target_version;
    int used_frag_color;
    int used_frag_data;
    int frag_output_indices[TITANGL_MAX_FRAG_OUTPUTS];
    int frag_output_count;
    titangl_attribute attributes[TITANGL_MAX_ATTRIBUTES];
    int attribute_count;
    char error[256];
} titangl_shader;

/*
 * Translate a desktop GLSL (or legacy GLSL ES 1.00) source to GLSL ES 3.00.
 *
 * is_vertex selects vertex/fragment specific keyword mapping.  On success 0
 * is returned and out->text belongs to the caller (free with
 * titangl_shader_free).  On failure -1 is returned and out->error explains
 * why.  A source that is already GLSL ES 3.00 or newer is returned almost
 * unchanged, but version/es detection still runs.
 */
int titangl_translate_shader(const char *source, size_t length, int is_vertex,
                             titangl_shader *out);

void titangl_shader_free(titangl_shader *shader);

#endif /* TITANGL_GLSL_H */
