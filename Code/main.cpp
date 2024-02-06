#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include <glm/glm.hpp>
using glm::vec3;
#include <boost/container/vector.hpp>
using boost::container::vector;

static void error_callback(int _, const char* desc)
{
    fprintf(stderr, "ERROR: %s\n", desc);
}

int main(int argc, char *argv[])
{
    glfwInit();
    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 4);
//    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    const int RES_X = 16*40, RES_Y = 9*40;
    GLFWwindow *window1;
    { // window1
        int screenWidth, screenHeight;
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        glfwGetMonitorWorkarea(primaryMonitor, NULL, NULL, &screenWidth, &screenHeight);

        window1 = glfwCreateWindow(RES_X, RES_Y, "#1", NULL, NULL);
        glfwMakeContextCurrent(window1);
        glfwSetWindowPos(window1, screenWidth-RES_X, 0);
        glfwSetWindowAttrib(window1, GLFW_FLOATING, GLFW_TRUE);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
//        glfwSwapInterval(0);
    }

    GLuint bufferA, tex1, tex2;
    {
        glGenTextures(1, &tex1);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, RES_X, RES_Y);
        glGenTextures(1, &tex2);
        glBindTexture(GL_TEXTURE_2D_ARRAY, tex2);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGB8, RES_X, RES_Y, 2);

        GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glGenFramebuffers(1, &bufferA);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex1, 0);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex2, 0, 0);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tex2, 0, 1);
        glDrawBuffers(2, drawBuffers);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    const int RES_W = 1024;
    GLuint bufferB, tex3;
    {
        glGenTextures(1, &tex3);
        glBindTexture(GL_TEXTURE_2D, tex3);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, RES_W, RES_W);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &bufferB);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferB);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex3, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        const float data[16] = { RES_X,RES_Y,0,0, 1,1,1,1, };
        GLuint ubo;
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof data, data, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    }

    while (!glfwWindowShouldClose(window1))
    {
        static uint32_t iFrame = 0;
        float iTime = glfwGetTime();
        {
            static float fps, lastFrameTime = 0;

            float dt = iTime - lastFrameTime; lastFrameTime = iTime;
            if ((iFrame++ & 0xf) == 0) fps = 1./dt;
            char title[32];
            sprintf(title, "%.2f\t\t%.1f fps\t\t%d x %d", iTime, fps, RES_X, RES_Y);
            glfwSetWindowTitle(window1, title);
            glBufferSubData(GL_UNIFORM_BUFFER, sizeof(float)*2, sizeof(float), &iTime);
        }

        typedef struct{ int x,y,z,w; }ivec4;
        typedef void (plugFunction1)(vector<vec3>&, float);
        typedef ivec4 (plugFunction2)();
        static ivec4 ret = {};
        static vector<vec3> appendBuffer;
        {
            static void *libraryHandle = NULL;
            static long lastModTime;
            static const char *libraryFilename="libModule.so";
            static plugFunction1 *mainAnimation = NULL;

            struct stat libStat;
            int err = stat(libraryFilename, &libStat);
            if (err == 0 && lastModTime != libStat.st_mtime)
            {
                if (libraryHandle)
                {
                    assert(dlclose(libraryHandle) == 0);
                }
                libraryHandle = dlopen(libraryFilename, RTLD_NOW);
                if (libraryHandle)
                {
                    lastModTime = libStat.st_mtime;
                    printf("INFO: reloading file %s\n", libraryFilename);

                    mainAnimation = (plugFunction1*)dlsym(libraryHandle, "mainAnimation");
                    assert(mainAnimation);
                    plugFunction2 *mainGeometry = (plugFunction2*)dlsym(libraryHandle, "mainGeometry");
                    ret = mainGeometry ? mainGeometry() : ivec4{};
                }
                else
                {
                    mainAnimation = NULL;
                }
            }

            if (mainAnimation) mainAnimation(appendBuffer, iTime);
        }

        {
            static GLuint vbo, frame;
            if (!frame++)
            {
                glGenBuffers(1, &vbo);
            }
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glEnableVertexAttribArray(8);
            glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, 12, 0);
            int oldSize;
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &oldSize);
            int newSize = appendBuffer.size() * sizeof appendBuffer[0];
            if (oldSize < newSize)
            {
                glBufferData(GL_ARRAY_BUFFER, newSize, appendBuffer.data(), GL_DYNAMIC_DRAW);
            }
            else
            {
                glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, appendBuffer.data());
            }
        }

//>>>>>>>>>>>>>>>>>>>>>>>>>RENDER<<<<<<<<<<<<<<<<<<<<<<
#define SHADER_DIR "../Code/"
        int reloadShader1(long*, GLuint, const char*);
        int reloadShader2(long*, GLuint, const char*);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ret.y);

        glDepthMask(1);
        glFrontFace(GL_CW);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferB);
        glViewport(0,0, RES_W, RES_W);
        glClear(GL_DEPTH_BUFFER_BIT);
        { // shadow
            static long lastModTime4;
            static const GLuint prog4 = glCreateProgram();
            reloadShader2(&lastModTime4, prog4, SHADER_DIR"shadowmap.glsl");
            glUseProgram(prog4);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, ret.x, 0);
        }
        glDepthFunc(GL_LESS);
        glFrontFace(GL_CCW);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
        glViewport(0,0, RES_X, RES_Y);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        { // geometry
            static long lastModTime2;
            static const GLuint prog2 = glCreateProgram();
            reloadShader2(&lastModTime2, prog2, SHADER_DIR"base.glsl");
            glUseProgram(prog2);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, ret.x, 0);
        }
        glDepthMask(0);
        { // gizmo
            static long lastModTime;
            static GLuint prog = glCreateProgram();
            reloadShader2(&lastModTime, prog, SHADER_DIR"line.glsl");
            glUseProgram(prog);
            glDrawArrays(GL_LINES, 0, appendBuffer.size());
            appendBuffer.clear();
        }
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0,0, RES_X, RES_Y);
        glClear(GL_COLOR_BUFFER_BIT);
        { // lighting
            static long lastModTime1;
            static const GLuint prog1 = glCreateProgram();
            int dirty = reloadShader1(&lastModTime1, prog1, SHADER_DIR"base.frag");
            if (dirty)
            {
                GLint iChannel1 = glGetUniformLocation(prog1, "iChannel1");
                glProgramUniform1i(prog1, iChannel1, 1);
                GLint iChannel2 = glGetUniformLocation(prog1, "iChannel2");
                glProgramUniform1i(prog1, iChannel2, 2);
                GLint iChannel3 = glGetUniformLocation(prog1, "iChannel3");
                glProgramUniform1i(prog1, iChannel3, 3);
            }
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D_ARRAY, tex2);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, tex3);
            glUseProgram(prog1);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glfwSwapBuffers(window1);
        glfwPollEvents();

#if 0
        static const char cmd[] = "LD_LIBRARY_PATH=/usr/bin ffmpeg"
                " -r 60 -f rawvideo -pix_fmt rgb24 -s 640x360"
                " -i pipe: -c:v libx264 -c:a aac"
                " -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip 1.mp4";

        static FILE *pipe = popen(cmd, "w");
        static char buffer[RES_X*RES_Y*3];
        static int frame = 0;

        glReadPixels(0,0, RES_X, RES_Y, GL_RGB, GL_UNSIGNED_BYTE, buffer);
        fwrite(buffer, sizeof(buffer), 1, pipe);
        if (iFrame > 10*60)
        {
            pclose(pipe);
            break;
        }
#endif
    }

    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
    glfwDestroyWindow(window1);
    glfwTerminate();
}

static void detachShaders(GLuint prog)
{
    GLsizei numShaders;
    GLuint shaders[5];
    glGetAttachedShaders(prog, 5, &numShaders, shaders);
    for (int i=0; i<numShaders; i++)
    {
        glDetachShader(prog, shaders[i]);
    }
}

int loadShader1(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char version[32];
    fgets(version, sizeof(version), f);
    length -= ftell(f);
    char source1[length+1]; source1[length] = 0; // set null terminator
    fread(source1, length, 1, f);
    fclose(f);

    detachShaders(prog);
    {
        const char *string[] = { version, source1 };
        const GLuint sha = glCreateShader(GL_FRAGMENT_SHADER);
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
            fprintf(stderr, "ERROR: fail to compile fragment shader. file %s\n%s\n", filename, message);
            return 2;
        }
        glAttachShader(prog, sha);
        glDeleteShader(sha);
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
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char version[32];
    fgets(version, sizeof(version), f);
    length -= ftell(f);
    char source1[length+1]; source1[length] = 0; // set null terminator
    fread(source1, length, 1, f);
    fclose(f);

    detachShaders(prog);
    for (int i=0; i<2; i++)
    {
        const char *string[] = { version, i==0?"#define _VS\n":"#define _FS\n", source1 };
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
int reloadShaderX(typeof loadShader1 f, long *lastModTime, GLuint prog, const char *filename)
{
    struct stat libStat;
    int err = stat(filename, &libStat);
    if (err == 0 && *lastModTime != libStat.st_mtime)
    {
        err = f(prog, filename);
        if (err != 1)
        {
            printf("INFO: reloading file %s\n", filename);
            *lastModTime = libStat.st_mtime;
            return 1;
        }
    }
    return 0;
}

int reloadShader1(long *lastModTime, GLuint prog, const char *filename)
{
    return reloadShaderX(loadShader1, lastModTime, prog, filename);
}

int reloadShader2(long *lastModTime, GLuint prog, const char *filename)
{
    return reloadShaderX(loadShader2, lastModTime, prog, filename);
}
