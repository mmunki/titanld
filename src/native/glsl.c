/*
 * TitanGL -- desktop GLSL to GLSL ES 3.00 translation.
 *
 * The translator is intentionally a textual, token-aware rewriter rather than
 * a full compiler.  It covers what Minecraft: Java Edition and the launchers
 * in front of it use:
 *
 *   - #version 110/120/130/150/330/400/410/420/430/440/450/460 -> 300 es
 *   - attribute / varying -> in / out
 *   - texture2D / textureCube / shadow2D family -> texture
 *   - gl_FragColor / gl_FragData[n] -> explicit fragment outputs
 *   - layout(location = N) on vertex inputs -> recorded for glBindAttribLocation
 *   - desktop-only #extension lines dropped
 *
 * Everything unsupported is left in place so the driver reports a real
 * compile error instead of silently rendering something wrong.
 */
#include "glsl.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} text_buffer;

static int buffer_reserve(text_buffer *b, size_t extra)
{
    if (b->length + extra + 1 <= b->capacity) {
        return 0;
    }
    size_t want = b->capacity ? b->capacity : 256;
    while (want < b->length + extra + 1) {
        if (want > (SIZE_MAX / 2)) {
            return -1;
        }
        want *= 2;
    }
    char *next = (char *)realloc(b->data, want);
    if (!next) {
        return -1;
    }
    b->data = next;
    b->capacity = want;
    return 0;
}

static int buffer_putn(text_buffer *b, const char *s, size_t n)
{
    if (buffer_reserve(b, n) != 0) {
        return -1;
    }
    memcpy(b->data + b->length, s, n);
    b->length += n;
    b->data[b->length] = '\0';
    return 0;
}

static int buffer_puts(text_buffer *b, const char *s)
{
    return buffer_putn(b, s, strlen(s));
}

static int buffer_putc(text_buffer *b, char c)
{
    return buffer_putn(b, &c, 1);
}

static void buffer_free(text_buffer *b)
{
    free(b->data);
    memset(b, 0, sizeof(*b));
}

static int is_ident_start(unsigned char c)
{
    return isalpha(c) || c == '_';
}

static int is_ident_char(unsigned char c)
{
    return isalnum(c) || c == '_';
}

static void set_error(titangl_shader *out, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(out->error, sizeof(out->error), fmt, ap);
    va_end(ap);
}

/*
 * Return the offset just past a #version line, and its version / es flag.
 * Leading comments and whitespace are tolerated.  If no #version line exists
 * the whole source is the body and a legacy 1.10 version is assumed.
 */
static size_t find_version(const char *s, size_t n, int *version, int *is_es)
{
    size_t i = 0;
    *version = 110;
    *is_es = 0;

    while (i < n) {
        while (i < n && isspace((unsigned char)s[i])) {
            i++;
        }
        if (i + 1 < n && s[i] == '/' && s[i + 1] == '/') {
            i += 2;
            while (i < n && s[i] != '\n') {
                i++;
            }
            continue;
        }
        if (i + 1 < n && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(s[i] == '*' && s[i + 1] == '/')) {
                i++;
            }
            i = (i + 1 < n) ? i + 2 : n;
            continue;
        }
        if (i < n && s[i] == '#') {
            size_t j = i + 1;
            while (j < n && (s[j] == ' ' || s[j] == '\t')) {
                j++;
            }
            if (j + 7 <= n && strncmp(s + j, "version", 7) == 0 &&
                (j + 7 == n || !is_ident_char((unsigned char)s[j + 7]))) {
                j += 7;
                while (j < n && (s[j] == ' ' || s[j] == '\t')) {
                    j++;
                }
                int v = 0;
                while (j < n && isdigit((unsigned char)s[j])) {
                    v = v * 10 + (s[j] - '0');
                    j++;
                }
                size_t line = j;
                int es = 0;
                while (line < n && s[line] != '\n' && s[line] != '\r') {
                    if (s[line] == 'e' && line + 1 < n && s[line + 1] == 's') {
                        es = 1;
                    }
                    line++;
                }
                if (v == 0) {
                    v = 110;
                }
                *version = v;
                *is_es = es;
                while (line < n && s[line] != '\n') {
                    line++;
                }
                if (line < n) {
                    line++;
                }
                return line;
            }
        }
        break;
    }
    return 0;
}

static void skip_space_and_comments(const char *s, size_t n, size_t *pos)
{
    size_t i = *pos;
    for (;;) {
        while (i < n && isspace((unsigned char)s[i])) {
            i++;
        }
        if (i + 1 < n && s[i] == '/' && s[i + 1] == '/') {
            i += 2;
            while (i < n && s[i] != '\n') {
                i++;
            }
            continue;
        }
        if (i + 1 < n && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(s[i] == '*' && s[i + 1] == '/')) {
                i++;
            }
            i = (i + 1 < n) ? i + 2 : n;
            continue;
        }
        break;
    }
    *pos = i;
}

static int read_word(const char *s, size_t n, size_t *pos, char *word, size_t wordsz)
{
    size_t i = *pos;
    skip_space_and_comments(s, n, &i);
    if (i >= n || !is_ident_start((unsigned char)s[i])) {
        *pos = i;
        return 0;
    }
    size_t start = i;
    while (i < n && is_ident_char((unsigned char)s[i])) {
        i++;
    }
    size_t len = i - start;
    if (len >= wordsz) {
        len = wordsz - 1;
    }
    memcpy(word, s + start, len);
    word[len] = '\0';
    *pos = i;
    return 1;
}

static int word_is(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

/*
 * The declaration name of an "in"/"out" statement starts at pos (just after
 * the keyword).  Return the first declarator name; handles basic qualifiers,
 * types and arrays.  Returns 0 when no name could be identified.
 */
static size_t parse_declarator_name(const char *s, size_t n, size_t pos, char *name,
                                    size_t namesz)
{
    size_t i = pos;
    char word[96];
    int have_name = 0;
    char last_name[96] = {0};

    while (i < n) {
        skip_space_and_comments(s, n, &i);
        if (i >= n) {
            break;
        }
        unsigned char c = (unsigned char)s[i];
        if (c == ';' || c == ',' || c == '{' || c == '}') {
            break;
        }
        if (c == '[') {
            int depth = 1;
            i++;
            while (i < n && depth > 0) {
                if (s[i] == '[') {
                    depth++;
                } else if (s[i] == ']') {
                    depth--;
                }
                i++;
            }
            continue;
        }
        if (is_ident_start(c)) {
            size_t before = i;
            if (!read_word(s, n, &i, word, sizeof(word))) {
                i = before + 1;
                continue;
            }
            if (word_is(word, "flat") || word_is(word, "smooth") ||
                word_is(word, "noperspective") || word_is(word, "centroid") ||
                word_is(word, "sample") || word_is(word, "invariant") ||
                word_is(word, "precise") || word_is(word, "highp") ||
                word_is(word, "mediump") || word_is(word, "lowp") ||
                word_is(word, "float") || word_is(word, "int") ||
                word_is(word, "uint") || word_is(word, "bool") ||
                word_is(word, "vec2") || word_is(word, "vec3") ||
                word_is(word, "vec4") || word_is(word, "ivec2") ||
                word_is(word, "ivec3") || word_is(word, "ivec4") ||
                word_is(word, "uvec2") || word_is(word, "uvec3") ||
                word_is(word, "uvec4") || word_is(word, "bvec2") ||
                word_is(word, "bvec3") || word_is(word, "bvec4") ||
                word_is(word, "mat2") || word_is(word, "mat3") ||
                word_is(word, "mat4") || word_is(word, "mat2x2") ||
                word_is(word, "mat2x3") || word_is(word, "mat2x4") ||
                word_is(word, "mat3x2") || word_is(word, "mat3x3") ||
                word_is(word, "mat3x4") || word_is(word, "mat4x2") ||
                word_is(word, "mat4x3") || word_is(word, "mat4x4") ||
                strncmp(word, "sampler", 7) == 0 || word_is(word, "struct")) {
                continue;
            }
            snprintf(last_name, sizeof(last_name), "%s", word);
            have_name = 1;
            continue;
        }
        i++;
    }

    if (!have_name) {
        return 0;
    }
    snprintf(name, namesz, "%s", last_name);
    return strlen(name);
}

static void add_attribute(titangl_shader *out, int location, const char *name)
{
    if (location < 0 || out->attribute_count >= TITANGL_MAX_ATTRIBUTES) {
        return;
    }
    titangl_attribute *a = &out->attributes[out->attribute_count];
    a->location = location;
    snprintf(a->name, sizeof(a->name), "%s", name);
    out->attribute_count++;
}

static void add_frag_output(titangl_shader *out, int index)
{
    int i;
    if (index < 0 || index >= TITANGL_MAX_FRAG_OUTPUTS) {
        return;
    }
    for (i = 0; i < out->frag_output_count; i++) {
        if (out->frag_output_indices[i] == index) {
            return;
        }
    }
    out->frag_output_indices[out->frag_output_count++] = index;
}

/* Parse the contents of a layout(...) qualifier. */
static void parse_layout(const char *s, size_t n, size_t start, size_t end,
                         int *has_location, int *location, int *has_binding,
                         int *has_std140, int *has_shared, int *has_packed)
{
    size_t i = start;
    *has_location = 0;
    *location = 0;
    *has_binding = 0;
    *has_std140 = 0;
    *has_shared = 0;
    *has_packed = 0;

    while (i < end) {
        char word[64];
        size_t before = i;
        if (!read_word(s, n, &i, word, sizeof(word))) {
            i = before + 1;
            if (i > end) {
                break;
            }
            continue;
        }
        if (i > end) {
            break;
        }
        if (word_is(word, "location")) {
            size_t j = i;
            while (j < end && isspace((unsigned char)s[j])) {
                j++;
            }
            if (j < end && s[j] == '=') {
                j++;
                while (j < end && isspace((unsigned char)s[j])) {
                    j++;
                }
                int value = 0;
                while (j < end && isdigit((unsigned char)s[j])) {
                    value = value * 10 + (s[j] - '0');
                    j++;
                }
                *has_location = 1;
                *location = value;
                i = j;
            }
        } else if (word_is(word, "binding")) {
            *has_binding = 1;
        } else if (word_is(word, "std140")) {
            *has_std140 = 1;
        } else if (word_is(word, "shared")) {
            *has_shared = 1;
        } else if (word_is(word, "packed")) {
            *has_packed = 1;
        }
    }
}

static int put_token(text_buffer *b, const char *word)
{
    if (word_is(word, "attribute")) {
        return buffer_puts(b, "in");
    }
    if (word_is(word, "varying")) {
        return 1; /* caller knows vertex/fragment context */
    }
    if (word_is(word, "texture2D") || word_is(word, "textureCube") ||
        word_is(word, "texture3D") || word_is(word, "texture1D") ||
        word_is(word, "shadow1D") || word_is(word, "shadow2D")) {
        return buffer_puts(b, "texture");
    }
    if (word_is(word, "texture2DProj") || word_is(word, "textureCubeProj") ||
        word_is(word, "texture3DProj") || word_is(word, "texture1DProj") ||
        word_is(word, "shadow1DProj") || word_is(word, "shadow2DProj")) {
        return buffer_puts(b, "textureProj");
    }
    if (word_is(word, "texture2DLod") || word_is(word, "textureCubeLod") ||
        word_is(word, "texture3DLod") || word_is(word, "texture1DLod") ||
        word_is(word, "shadow1DLod") || word_is(word, "shadow2DLod")) {
        return buffer_puts(b, "textureLod");
    }
    if (word_is(word, "texture2DProjLod") || word_is(word, "texture3DProjLod") ||
        word_is(word, "texture1DProjLod") || word_is(word, "shadow1DProjLod") ||
        word_is(word, "shadow2DProjLod")) {
        return buffer_puts(b, "textureProjLod");
    }
    return buffer_puts(b, word);
}

static const char k_precision_block[] =
    "precision highp float;\n"
    "precision highp int;\n"
    "precision highp sampler2D;\n"
    "precision highp sampler3D;\n"
    "precision highp samplerCube;\n"
    "precision highp sampler2DShadow;\n"
    "precision highp samplerCubeShadow;\n"
    "precision highp sampler2DArray;\n"
    "precision highp sampler2DArrayShadow;\n"
    "precision highp isampler2D;\n"
    "precision highp isampler3D;\n"
    "precision highp isamplerCube;\n"
    "precision highp isampler2DArray;\n"
    "precision highp usampler2D;\n"
    "precision highp usampler3D;\n"
    "precision highp usamplerCube;\n"
    "precision highp usampler2DArray;\n";

int titangl_translate_shader(const char *source, size_t length, int is_vertex,
                             titangl_shader *out)
{
    int version = 110;
    int source_es = 0;
    size_t body;
    text_buffer b;
    text_buffer extensions;
    size_t header_length;
    size_t i;
    int line_start = 1;
    int pending_location = -1;
    int saw_explicit_frag_out = 0;

    if (!source || !out) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    memset(&b, 0, sizeof(b));
    memset(&extensions, 0, sizeof(extensions));

    body = find_version(source, length, &version, &source_es);
    out->source_version = version;
    out->source_is_es = source_es;
    out->target_version = (source_es && version >= 300) ? version : 300;

    char header[128];
    if (source_es && version >= 300) {
        snprintf(header, sizeof(header), "#version %d es\n", version);
    } else {
        snprintf(header, sizeof(header), "#version 300 es\n");
    }
    if (buffer_puts(&b, header) != 0) {
        goto oom;
    }
    header_length = b.length;

    i = body;
    while (i < length) {
        unsigned char c = (unsigned char)source[i];

        if (line_start) {
            size_t j = i;
            while (j < length && (source[j] == ' ' || source[j] == '\t')) {
                j++;
            }
            if (j < length && source[j] == '#') {
                size_t k = j + 1;
                char directive[64];
                size_t dw = 0;
                while (k < length && (source[k] == ' ' || source[k] == '\t')) {
                    k++;
                }
                while (k < length && dw + 1 < sizeof(directive) &&
                       is_ident_char((unsigned char)source[k])) {
                    directive[dw++] = source[k++];
                }
                directive[dw] = '\0';
                size_t line_end = k;
                while (line_end < length && source[line_end] != '\n') {
                    line_end++;
                }
                if (word_is(directive, "version")) {
                    i = line_end < length ? line_end + 1 : length;
                    line_start = 1;
                    continue;
                }
                if (word_is(directive, "extension")) {
                    int drop = 0;
                    size_t m = k;
                    while (m < length && source[m] != '\n') {
                        if (strncmp(source + m, "GL_ARB_", 7) == 0 ||
                            strncmp(source + m, "GL_NV_", 6) == 0 ||
                            strncmp(source + m, "GL_AMD_", 7) == 0 ||
                            strncmp(source + m, "GL_INTEL_", 9) == 0 ||
                            strncmp(source + m, "GL_ATI_", 7) == 0 ||
                            strncmp(source + m, "GL_KHR_", 7) == 0 ||
                            strncmp(source + m, "GL_EXT_gpu_shader4", 18) == 0) {
                            drop = 1;
                            break;
                        }
                        m++;
                    }
                    if (drop) {
                        i = line_end < length ? line_end + 1 : length;
                        line_start = 1;
                        continue;
                    }
                    /*
                     * GLSL requires #extension directives before any other
                     * declaration.  Keep the line, but move it to the header
                     * between #version and the precision block.
                     */
                    size_t ext_end = (line_end < length) ? line_end + 1 : line_end;
                    if (buffer_putn(&extensions, source + j, ext_end - j) != 0) {
                        goto oom;
                    }
                    i = ext_end;
                    line_start = 1;
                    continue;
                }
            }
        }

        if (i + 1 < length && source[i] == '/' && source[i + 1] == '/') {
            size_t start = i;
            i += 2;
            while (i < length && source[i] != '\n') {
                i++;
            }
            if (buffer_putn(&b, source + start, i - start) != 0) {
                goto oom;
            }
            continue;
        }
        if (i + 1 < length && source[i] == '/' && source[i + 1] == '*') {
            size_t start = i;
            i += 2;
            while (i + 1 < length && !(source[i] == '*' && source[i + 1] == '/')) {
                i++;
            }
            i = (i + 1 < length) ? i + 2 : length;
            if (buffer_putn(&b, source + start, i - start) != 0) {
                goto oom;
            }
            continue;
        }
        if (source[i] == '"' || source[i] == '\'') {
            char quote = source[i];
            size_t start = i++;
            while (i < length) {
                if (source[i] == '\\' && i + 1 < length) {
                    i += 2;
                    continue;
                }
                if (source[i] == quote) {
                    i++;
                    break;
                }
                i++;
            }
            if (buffer_putn(&b, source + start, i - start) != 0) {
                goto oom;
            }
            line_start = 0;
            continue;
        }

        if (is_ident_start(c)) {
            size_t start = i;
            char word[96];
            while (i < length && is_ident_char((unsigned char)source[i])) {
                i++;
            }
            size_t wl = i - start;
            if (wl >= sizeof(word)) {
                wl = sizeof(word) - 1;
            }
            memcpy(word, source + start, wl);
            word[wl] = '\0';

            if (word_is(word, "layout")) {
                size_t open = i;
                while (open < length && isspace((unsigned char)source[open])) {
                    open++;
                }
                if (open < length && source[open] == '(') {
                    int depth = 1;
                    size_t close = open + 1;
                    while (close < length && depth > 0) {
                        if (source[close] == '(') {
                            depth++;
                        } else if (source[close] == ')') {
                            depth--;
                        }
                        close++;
                    }
                    int has_location = 0, location = 0, has_binding = 0;
                    int has_std140 = 0, has_shared = 0, has_packed = 0;
                    parse_layout(source, length, open + 1,
                                 close > open + 1 ? close - 1 : open + 1,
                                 &has_location, &location, &has_binding,
                                 &has_std140, &has_shared, &has_packed);
                    size_t next = close;
                    char keyword[64] = {0};
                    read_word(source, length, &next, keyword, sizeof(keyword));
                    int need_space = (close < length && !isspace((unsigned char)source[close]));
                    if (word_is(keyword, "uniform") || word_is(keyword, "buffer")) {
                        if (has_std140) {
                            if (buffer_puts(&b, "layout(std140)") != 0) {
                                goto oom;
                            }
                        } else if (has_shared) {
                            if (buffer_puts(&b, "layout(shared)") != 0) {
                                goto oom;
                            }
                        } else if (has_packed) {
                            if (buffer_puts(&b, "layout(packed)") != 0) {
                                goto oom;
                            }
                        }
                        if (need_space && buffer_putc(&b, ' ') != 0) {
                            goto oom;
                        }
                    } else if (word_is(keyword, "in")) {
                        if (is_vertex && has_location) {
                            pending_location = location;
                        }
                    } else if (word_is(keyword, "out")) {
                        if (!is_vertex && has_location) {
                            char loc[48];
                            snprintf(loc, sizeof(loc), "layout(location = %d)", location);
                            if (buffer_puts(&b, loc) != 0) {
                                goto oom;
                            }
                            if (need_space && buffer_putc(&b, ' ') != 0) {
                                goto oom;
                            }
                            saw_explicit_frag_out = 1;
                        }
                    }
                    (void)has_binding;
                    i = close;
                    line_start = 0;
                    continue;
                }
            }

            if (word_is(word, "varying")) {
                if (buffer_puts(&b, is_vertex ? "out" : "in") != 0) {
                    goto oom;
                }
                line_start = 0;
                continue;
            }
            if (word_is(word, "attribute")) {
                if (buffer_puts(&b, "in") != 0) {
                    goto oom;
                }
                line_start = 0;
                continue;
            }
            if (word_is(word, "in") || word_is(word, "out")) {
                if (buffer_puts(&b, word) != 0) {
                    goto oom;
                }
                if (pending_location >= 0 && word_is(word, "in") && is_vertex) {
                    char name[TITANGL_MAX_ATTRIBUTE_NAME];
                    if (parse_declarator_name(source, length, i, name, sizeof(name)) > 0) {
                        add_attribute(out, pending_location, name);
                    }
                    pending_location = -1;
                }
                if (pending_location >= 0) {
                    pending_location = -1;
                }
                line_start = 0;
                continue;
            }
            if (word_is(word, "gl_FragColor")) {
                out->used_frag_color = 1;
                if (buffer_puts(&b, "titan_FragColor") != 0) {
                    goto oom;
                }
                line_start = 0;
                continue;
            }
            if (word_is(word, "gl_FragData")) {
                size_t j = i;
                while (j < length && isspace((unsigned char)source[j])) {
                    j++;
                }
                if (j < length && source[j] == '[') {
                    j++;
                    while (j < length && isspace((unsigned char)source[j])) {
                        j++;
                    }
                    int index = 0;
                    while (j < length && isdigit((unsigned char)source[j])) {
                        index = index * 10 + (source[j] - '0');
                        j++;
                    }
                    while (j < length && isspace((unsigned char)source[j])) {
                        j++;
                    }
                    if (j < length && source[j] == ']') {
                        j++;
                        out->used_frag_data = 1;
                        add_frag_output(out, index);
                        char name[48];
                        snprintf(name, sizeof(name), "titan_FragData%d", index);
                        if (buffer_puts(&b, name) != 0) {
                            goto oom;
                        }
                        i = j;
                        line_start = 0;
                        continue;
                    }
                }
                if (buffer_puts(&b, "titan_FragData") != 0) {
                    goto oom;
                }
                line_start = 0;
                continue;
            }

            if (put_token(&b, word) != 0) {
                goto oom;
            }
            line_start = 0;
            continue;
        }

        if (buffer_putc(&b, source[i]) != 0) {
            goto oom;
        }
        line_start = (source[i] == '\n');
        i++;
    }

    /*
     * Declare the fragment outputs after the precision block but before the
     * translated body.  Build a second buffer so the declarations sit between
     * the preamble and the body.
     */
    {
        text_buffer final;
        memset(&final, 0, sizeof(final));
        if (buffer_putn(&final, b.data, header_length) != 0) {
            buffer_free(&final);
            goto oom;
        }
        if (extensions.length &&
            buffer_putn(&final, extensions.data, extensions.length) != 0) {
            buffer_free(&final);
            goto oom;
        }
        if (buffer_puts(&final, k_precision_block) != 0) {
            buffer_free(&final);
            goto oom;
        }
        if (out->used_frag_color) {
            if (buffer_puts(&final,
                            "layout(location = 0) out highp vec4 titan_FragColor;\n") != 0) {
                buffer_free(&final);
                goto oom;
            }
        }
        for (int oi = 0; oi < out->frag_output_count; oi++) {
            char line[80];
            snprintf(line, sizeof(line),
                     "layout(location = %d) out highp vec4 titan_FragData%d;\n",
                     out->frag_output_indices[oi], out->frag_output_indices[oi]);
            if (buffer_puts(&final, line) != 0) {
                buffer_free(&final);
                goto oom;
            }
        }
        if (buffer_putn(&final, b.data + header_length, b.length - header_length) != 0) {
            buffer_free(&final);
            goto oom;
        }
        buffer_free(&b);
        buffer_free(&extensions);
        b = final;
    }

    (void)saw_explicit_frag_out;
    out->text = b.data;
    out->length = b.length;
    return 0;

oom:
    buffer_free(&b);
    buffer_free(&extensions);
    set_error(out, "out of memory while translating the shader");
    return -1;
}

void titangl_shader_free(titangl_shader *shader)
{
    if (!shader) {
        return;
    }
    free(shader->text);
    shader->text = NULL;
    shader->length = 0;
}
