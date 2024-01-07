#include <stdio.h>
#include <assert.h>
#include <glad/glad.h>

int loadShader1(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "ERROR: file %s not found.", filename);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char version[32];
    fgets(version, sizeof(version), f);
    length -= ftell(f);
    char source[length+1]; source[length] = 0; // set null terminator
    fread(source, length, 1, f);
    fclose(f);

#define COMMON_SHADER_FILE_PATH "../common.glsl"
#ifdef COMMON_SHADER_FILE_PATH
    f = fopen(COMMON_SHADER_FILE_PATH, "r");
    if (!f) {
        fprintf(stderr, "ERROR: file %s not found.", filename);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long commonLength = ftell(f);
    rewind(f);
    char commonSource[commonLength+1]; commonSource[commonLength] = 0; // set null terminator
    fread(commonSource, commonLength, 1, f);
    fclose(f);
#endif

    { // detach shaders
        GLsizei numShaders;
        GLuint shaders[5];
        glGetAttachedShaders(prog, 5, &numShaders, shaders);
        for (int i=0; i<numShaders; i++)
        {
            glDetachShader(prog, shaders[i]);
        }
    }

    {
        const char *string[] = { version, commonSource, source };
        const GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, sizeof string/sizeof *string, string, NULL);
        glCompileShader(fs);
        int success;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            int length;
            glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &length);
            char message[length];
            glGetShaderInfoLog(fs, length, &length, message);
            fprintf(stderr, "ERROR: fail to compile fragment shader. file %s\n%s\n", filename, message);
            return 2;
        }
        glAttachShader(prog, fs);
        glDeleteShader(fs);
    }
    {
        const char vsSource[] = R"(
        precision mediump float;
        void main() {
            vec2 UV = vec2(gl_VertexID%2, gl_VertexID/2)*2.-1.;
            gl_Position = vec4(UV, 0, 1);
        }
        )";

        const char *string[] = { version, vsSource };
        const GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 2, string, NULL);
        glCompileShader(vs);
        int success;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        assert(success);
        glAttachShader(prog, vs);
        glDeleteShader(vs);
    }

    glLinkProgram(prog);
    glValidateProgram(prog);
    return 0;
}

int loadShader2(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        fprintf(stderr, "ERROR: file %s not found.", filename);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char version[32];
    fgets(version, sizeof(version), f);
    length -= ftell(f);
    char source[length+1]; source[length] = 0; // set null terminator
    fread(source, length, 1, f);
    fclose(f);

#ifdef COMMON_SHADER_FILE_PATH
    f = fopen(COMMON_SHADER_FILE_PATH, "r");
    if (!f) {
        fprintf(stderr, "ERROR: file %s not found.", filename);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long commonLength = ftell(f);
    rewind(f);
    char commonSource[commonLength+1]; commonSource[commonLength] = 0; // set null terminator
    fread(commonSource, commonLength, 1, f);
    fclose(f);
#endif

    { // detach shaders
        GLsizei numShaders;
        GLuint shaders[5];
        glGetAttachedShaders(prog, 5, &numShaders, shaders);
        for (int i=0; i<numShaders; i++)
        {
            glDetachShader(prog, shaders[i]);
        }
    }

    for (int i=0; i<2; i++)
    {
        const char *string[] = { version, i==0?"#define _VS\n":"#define _FS\n", commonSource, source };
        const GLuint sha = glCreateShader(i==0?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);
        glShaderSource(sha, sizeof string/sizeof *string, string, NULL);
        glCompileShader(sha);
        int success;
        glGetShaderiv(sha, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            int length;
            glGetShaderiv(sha, GL_INFO_LOG_LENGTH, &length);
            char message[length];
            glGetShaderInfoLog(sha, length, &length, message);
            fprintf(stderr, "ERROR: fail to compile %s shader. file %s\n%s\n",
                i==0?"vertex":"fragment", filename, message);
            return 2;
        }
        glAttachShader(prog, sha);
        glDeleteShader(sha);
    }
    glLinkProgram(prog);
    glValidateProgram(prog);
    return 0;
}

int loadShader6(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        fprintf(stderr, "ERROR: file %s not found.", filename);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char source[length+1]; source[length] = 0; // set null terminator
    fread(source, length, 1, f);
    fclose(f);

    { // detach shaders
        GLsizei numShaders;
        GLuint shaders[5];
        glGetAttachedShaders(prog, 5, &numShaders, shaders);
        for (int i=0; i<numShaders; i++)
        {
            glDetachShader(prog, shaders[i]);
        }
    }

    const char *string[] = { source };
    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, sizeof string/sizeof *string, string, NULL);
    glCompileShader(cs);
    int success;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        int length;
        glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &length);
        char message[length];
        glGetShaderInfoLog(cs, length, &length, message);
        fprintf(stderr, "ERROR: fail to compile compute shader. file \n%s\n", message);
        return 2;
    }
    glAttachShader(prog, cs);
    glDeleteShader(cs);
    glLinkProgram(prog);
    glValidateProgram(prog);
    return 0;
}
