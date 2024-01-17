#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/stat.h>

static void error_callback(int _, const char* desc)
{
    fprintf(stderr, "ERROR: %s\n", desc);
}

static void joystick_callback(int jid, int event)
{
    if (event == GLFW_CONNECTED) {
        printf("INFO: Joystick connected: %s\n", glfwGetJoystickName(jid));
    } else if (event == GLFW_DISCONNECTED) {
        printf("INFO: Joystick disconnected: %s\n", glfwGetJoystickName(jid));
    }
}

int main(int argc, char *argv[])
{
    const int RES_X = 16*40, RES_Y = 9*40;
    GLFWwindow *window1;

    glfwInit();
    glfwSetErrorCallback(error_callback);
    glfwSetJoystickCallback(joystick_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
//    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

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

    GLuint bufferA;
    {
        GLuint tex1, tex2;
        glGenTextures(1, &tex1);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, RES_X, RES_Y);
        glGenTextures(1, &tex2);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, RES_X, RES_Y);

        glGenFramebuffers(1, &bufferA);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex1, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex2, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex2);

        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        float data[16] = { RES_X,RES_Y,0,0, 1,1,1,1, };
        GLuint ubo;
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof data, data, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    }

    typedef int (plugFunction1)(float);
    typedef int (plugFunction2)(void);
    plugFunction1 *mainAnimation = NULL;
    plugFunction2 *mainGeometry, *mainGUI;

    int loadShader1(GLuint, const char*);
    int loadShader2(GLuint, const char*);
    int loadShaderA(long*, GLuint, const char*);
    int loadShaderB(long*, GLuint, const char*);
    int loadLibrary(long*, void *, const char*);

    const GLuint prog1 = glCreateProgram();
    const GLuint prog2 = glCreateProgram();
    const GLuint prog3 = glCreateProgram();
    assert(0 == loadShader2(prog3, "../line.glsl"));

    GLint enableWireFrame;
    long lastModTime1, lastModTime2, lastModTime3;
    float fps, lastFrameTime = 0;
    uint32_t iFrame = 0;

    while (!glfwWindowShouldClose(window1))
    {
        ++iFrame;
        float iTime = glfwGetTime();
        float dt = iTime - lastFrameTime; lastFrameTime = iTime;
        if ((iFrame & 0xf) == 0) fps = 1./dt;
        char title[32];
        sprintf(title, "%4.2f\t\t%.1f fps\t\t%d x %d", iTime, fps, RES_X, RES_Y);
        glfwSetWindowTitle(window1, title);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(float)*2, sizeof(float), &iTime);

        int dirty1 = loadShaderA(&lastModTime1, prog1, "../base.frag");
        int dirty2 = loadShaderB(&lastModTime2, prog2, "../base.glsl");
        if (dirty1)
        {
            GLint iChannel1 = glGetUniformLocation(prog1, "iChannel1");
            glProgramUniform1i(prog1, iChannel1, 1);
        }
        if (dirty2)
        {
            enableWireFrame = glGetUniformLocation(prog2, "enableWireFrame");
        }

        static int drawCount = 0;
        {
//        loadLibrary(&lastModTime3, handle, libraryFilename);
            static void *libraryHandle = NULL;
            static long lastModTime;
            static const char *libraryFilename="libModule.so";

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
                    mainGeometry = (plugFunction2*)dlsym(libraryHandle, "mainGeometry");
                    if (mainGeometry) drawCount = mainGeometry();
                }
                else
                {
                    mainAnimation = NULL;
                }
            }

            if (mainAnimation) mainAnimation(iTime);
        }

        glClearColor(0,0,0,1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LESS);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
        glViewport(0,0, RES_X, RES_Y);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        if (0)
        {
            glUseProgram(prog2);
            glProgramUniform1f(prog2, enableWireFrame, 0);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, 0, 0);
        }

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0,0, RES_X, RES_Y);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (drawCount > 0)
        {
            glUseProgram(prog3);
            glDrawElements(GL_LINES, drawCount & 0xff, GL_UNSIGNED_INT, NULL);
            glDrawArrays(GL_POINTS, 0, drawCount >> 8);
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

int loadShader1(GLuint, const char*);
int loadShader2(GLuint, const char*);

int loadShaderX(long *lastModTime, typeof loadShader1 f, GLuint prog, const char *filename)
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

int loadShaderA(long *lastModTime, GLuint prog, const char *filename)
{
    return loadShaderX(lastModTime, loadShader1, prog, filename);
}

int loadShaderB(long *lastModTime, GLuint prog, const char *filename)
{
    return loadShaderX(lastModTime, loadShader2, prog, filename);
}

int loadLibrary(long *lastModTime, void *handle, const char *filename)
{
    struct stat libStat;
    int err = stat(filename, &libStat);
    if (err == 0 && *lastModTime != libStat.st_mtime)
    {
        if (handle)
        {
            assert(dlclose(handle) == 0);
        }
        handle = dlopen(filename, RTLD_NOW);
        if (handle)
        {
            *lastModTime = libStat.st_mtime;
            printf("INFO: reloading file %s\n", filename);
            return 1;
        }
        else
        {
            return 2;
        }
    }
    return 0;
}
