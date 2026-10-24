/*
 * Host-side tests for the TitanGL shader translator.  No Android, no GL
 * driver: the translator is deliberately a pure C module.
 */
#include "../src/native/glsl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

static void check(int condition, const char *what)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", what);
        failures++;
    }
}

static void check_contains(const titangl_shader *s, const char *needle, const char *what)
{
    if (!s->text || !strstr(s->text, needle)) {
        fprintf(stderr, "FAIL: %s (missing %s)\n", what, needle);
        if (s->text) {
            fprintf(stderr, "---- translated ----\n%s\n--------------------\n", s->text);
        }
        failures++;
    }
}

static void check_not_contains(const titangl_shader *s, const char *needle, const char *what)
{
    if (s->text && strstr(s->text, needle)) {
        fprintf(stderr, "FAIL: %s (unexpected %s)\n", what, needle);
        failures++;
    }
}

static void test_legacy_vertex(void)
{
    const char *src =
        "#version 120\n"
        "attribute vec4 Position;\n"
        "attribute vec2 TexCoord;\n"
        "varying vec2 vTexCoord;\n"
        "uniform mat4 ModelViewProjection;\n"
        "void main() {\n"
        "    vTexCoord = TexCoord;\n"
        "    gl_Position = ModelViewProjection * Position;\n"
        "}\n";
    titangl_shader s;
    check(titangl_translate_shader(src, strlen(src), 1, &s) == 0, "legacy vertex translates");
    check_contains(&s, "#version 300 es", "legacy vertex version");
    check_contains(&s, "in vec4 Position", "attribute becomes in");
    check_contains(&s, "out vec2 vTexCoord", "varying becomes out in vertex");
    check_not_contains(&s, "attribute", "no attribute keyword");
    check_not_contains(&s, "varying", "no varying keyword");
    titangl_shader_free(&s);
}

static void test_modern_fragment(void)
{
    const char *src =
        "#version 330 core\n"
        "layout(location = 0) out vec4 fragColor;\n"
        "uniform sampler2D tex;\n"
        "in vec2 uv;\n"
        "void main() { fragColor = texture2D(tex, uv); }\n";
    titangl_shader s;
    check(titangl_translate_shader(src, strlen(src), 0, &s) == 0, "modern fragment translates");
    check_contains(&s, "#version 300 es", "modern fragment version");
    check_contains(&s, "texture(tex, uv)", "texture2D becomes texture");
    check_contains(&s, "layout(location = 0) out vec4 fragColor", "explicit output kept");
    check_not_contains(&s, "texture2D", "no texture2D");
    titangl_shader_free(&s);
}

static void test_attribute_location(void)
{
    const char *src =
        "#version 330 core\n"
        "layout(location = 3) in vec3 aNormal;\n"
        "layout(location = 0) in vec4 aPos;\n"
        "out vec3 vNormal;\n"
        "void main() { vNormal = aNormal; gl_Position = aPos; }\n";
    titangl_shader s;
    check(titangl_translate_shader(src, strlen(src), 1, &s) == 0, "location vertex translates");
    check_not_contains(&s, "layout", "vertex input location stripped");
    check(s.attribute_count == 2, "two attributes recorded");
    if (s.attribute_count == 2) {
        int found_normal = 0, found_pos = 0;
        for (int i = 0; i < s.attribute_count; i++) {
            if (s.attributes[i].location == 3 &&
                strcmp(s.attributes[i].name, "aNormal") == 0) {
                found_normal = 1;
            }
            if (s.attributes[i].location == 0 &&
                strcmp(s.attributes[i].name, "aPos") == 0) {
                found_pos = 1;
            }
        }
        check(found_normal && found_pos, "attribute names and locations recorded");
    }
    titangl_shader_free(&s);
}

static void test_frag_color(void)
{
    const char *src =
        "#version 120\n"
        "varying vec2 uv;\n"
        "uniform sampler2D tex;\n"
        "void main() { gl_FragColor = texture2D(tex, uv); }\n";
    titangl_shader s;
    check(titangl_translate_shader(src, strlen(src), 0, &s) == 0, "frag color translates");
    check_contains(&s, "out highp vec4 titan_FragColor", "frag color declaration");
    check_contains(&s, "titan_FragColor = texture(tex, uv)", "frag color body");
    check_not_contains(&s, "gl_FragColor", "no gl_FragColor");
    titangl_shader_free(&s);
}

static void test_mrt(void)
{
    const char *src =
        "#version 330\n"
        "void main() {\n"
        "    gl_FragData[1] = vec4(1.0);\n"
        "    gl_FragData[0] = vec4(0.5);\n"
        "}\n";
    titangl_shader s;
    check(titangl_translate_shader(src, strlen(src), 0, &s) == 0, "MRT translates");
    check(s.frag_output_count == 2, "two MRT outputs recorded");
    check_contains(&s, "layout(location = 1) out highp vec4 titan_FragData1;", "MRT 1 declaration");
    check_contains(&s, "layout(location = 0) out highp vec4 titan_FragData0;", "MRT 0 declaration");
    titangl_shader_free(&s);
}

static void test_comments_are_not_rewritten(void)
{
    const char *src =
        "#version 150\n"
        "// texture2D gl_FragColor attribute varying\n"
        "/* texture2D attribute */\n"
        "in vec2 uv;\n"
        "uniform sampler2D tex;\n"
        "out vec4 color;\n"
        "void main() { color = texture2D(tex, uv); }\n";
    titangl_shader s;
    check(titangl_translate_shader(src, strlen(src), 0, &s) == 0, "comments translate");
    check_contains(&s, "// texture2D gl_FragColor attribute varying", "line comment preserved");
    check_contains(&s, "/* texture2D attribute */", "block comment preserved");
    check_contains(&s, "color = texture(tex, uv)", "code outside comments translated");
    titangl_shader_free(&s);
}

static void test_desktop_extensions_dropped(void)
{
    const char *src =
        "#version 400\n"
        "#extension GL_ARB_gpu_shader5 : enable\n"
        "#extension GL_NV_gpu_program5 : enable\n"
        "in vec2 uv;\n"
        "void main() {}\n";
    titangl_shader s;
    check(titangl_translate_shader(src, strlen(src), 1, &s) == 0, "extensions translate");
    check_not_contains(&s, "GL_ARB_gpu_shader5", "ARB extension dropped");
    check_not_contains(&s, "GL_NV_gpu_program5", "NV extension dropped");
    titangl_shader_free(&s);
}

static void test_uniform_block_binding(void)
{
    const char *src =
        "#version 330\n"
        "layout(std140, binding = 2) uniform Globals { mat4 mvp; };\n"
        "layout(location = 0) in vec4 pos;\n"
        "void main() { gl_Position = mvp * pos; }\n";
    titangl_shader s;
    check(titangl_translate_shader(src, strlen(src), 1, &s) == 0, "uniform block translates");
    check_contains(&s, "layout(std140) uniform Globals", "std140 kept, binding stripped");
    check_not_contains(&s, "binding", "binding removed");
    titangl_shader_free(&s);
}

static void test_es_source_passthrough(void)
{
    const char *src =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 uv;\n"
        "out vec4 color;\n"
        "void main() { color = vec4(uv, 0.0, 1.0); }\n";
    titangl_shader s;
    check(titangl_translate_shader(src, strlen(src), 0, &s) == 0, "ES source translates");
    check_contains(&s, "#version 300 es", "ES version kept");
    check_contains(&s, "color = vec4(uv, 0.0, 1.0)", "ES body kept");
    titangl_shader_free(&s);
}

int main(void)
{
    test_legacy_vertex();
    test_modern_fragment();
    test_attribute_location();
    test_frag_color();
    test_mrt();
    test_comments_are_not_rewritten();
    test_desktop_extensions_dropped();
    test_uniform_block_binding();
    test_es_source_passthrough();

    if (failures) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }
    printf("glsl translator: all tests passed\n");
    return 0;
}
