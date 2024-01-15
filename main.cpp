#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/stat.h>

static int _mainAnimation(float) { return 0; }

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

#define ENABLE_FFMPEG 0
#if ENABLE_FFMPEG
    const char cmd[] = "LD_LIBRARY_PATH=/usr/bin ffmpeg -r 60 -f rawvideo -pix_fmt rgb24 -s 640x360"
                       " -i pipe: -c:v libx264 -c:a aac"
                       " -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip 1.mp4";
    FILE *pipe = popen(cmd, "w");
#endif

    glfwInit();
    glfwSetErrorCallback(error_callback);
    glfwSetJoystickCallback(joystick_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    { // window1
        int screenWidth, screenHeight;
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        glfwGetMonitorWorkarea(primaryMonitor, NULL, NULL, &screenWidth, &screenHeight);

        window1 = glfwCreateWindow(RES_X, RES_Y, "#1", NULL, NULL);
        glfwMakeContextCurrent(window1);
        glfwSetWindowPos(window1, screenWidth-RES_X, 0);
        glfwSetWindowAttrib(window1, GLFW_FLOATING, GLFW_TRUE);
        glfwSwapInterval(1);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    }

    GLuint fbo;
    {
        GLuint tex1, tex2;
        glGenTextures(1, &tex1);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, RES_X, RES_Y);
        glGenTextures(1, &tex2);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, RES_X, RES_Y);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
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

    int loadShader1(GLuint, const char*);
    int loadShader2(GLuint, const char*);

    const GLuint prog1 = glCreateProgram();
    const GLuint prog2 = glCreateProgram();
    const GLuint prog3 = glCreateProgram();
    assert(0 == loadShader2(prog3, "../line.glsl"));
    GLint enableWireFrame;

    typedef int (plugFunction1)(float);
    typedef int (plugFunction2)(void);
    typedef int (plugFunction3)(void);
    plugFunction1 *mainAnimation = _mainAnimation;
    plugFunction2 *mainGeometry;
    plugFunction3 *mainGUI;
    int result1 = 0, result2 = 0, result3 = 0;

    void *libraryHandle = NULL;
    const char *libraryFilename="libModule.so";

    while (!glfwWindowShouldClose(window1))
    {
        {
            static long lastModTime;
            const char *shaderFilename="../base.glsl";
            struct stat libStat;
            int err = stat(shaderFilename, &libStat);
            if (err == 0 && lastModTime != libStat.st_mtime)
            {
                err = loadShader2(prog2, shaderFilename);
                if (err != 1)
                {
                    printf("INFO: reloading file %s\n", shaderFilename);
                    lastModTime = libStat.st_mtime;
                    enableWireFrame = glGetUniformLocation(prog2, "enableWireFrame");
                }
            }
        }
        {
            static long lastModTime;
            const char *shaderFilename="../base.frag";
            struct stat libStat;
            int err = stat(shaderFilename, &libStat);
            if (err == 0 && lastModTime != libStat.st_mtime)
            {
                err = loadShader1(prog1, shaderFilename);
                if (err != 1)
                {
                    printf("INFO: reloading file %s\n", shaderFilename);
                    lastModTime = libStat.st_mtime;
                    GLint iChannel1 = glGetUniformLocation(prog1, "iChannel1");
                    glProgramUniform1i(prog1, iChannel1, 1);
                    GLint iChannel2 = glGetUniformLocation(prog1, "iChannel2");
                    glProgramUniform1i(prog1, iChannel2, 2);
                    GLint iChannel3 = glGetUniformLocation(prog1, "iChannel3");
                    glProgramUniform1i(prog1, iChannel3, 3);
                }
            }
        }
        {
            static long lastModTime;
            struct stat libStat;
            int err = stat(libraryFilename, &libStat);
            if (err == 0 && lastModTime != libStat.st_mtime)
            {
                if (libraryHandle)
                {
                    assert(dlclose(libraryHandle) == 0);
                }
                libraryHandle = dlopen(libraryFilename, RTLD_NOW);
                if (!libraryHandle) continue;
                printf("INFO: reloading file %s\n", libraryFilename);
                lastModTime = libStat.st_mtime;
                mainAnimation = (plugFunction1*)dlsym(libraryHandle, "mainAnimation");
                assert(mainAnimation);

                mainGeometry = (plugFunction2*)dlsym(libraryHandle, "mainGeometry");
                if (mainGeometry) result2 = mainGeometry();
                mainGUI = (plugFunction3*)dlsym(libraryHandle, "mainGUI");
                if (mainGUI) result3 = mainGUI();
            }
            float t = glfwGetTime();
            glBufferSubData(GL_UNIFORM_BUFFER, sizeof(float)*2, sizeof(float), &t);
            result1 = mainAnimation(t);
        }

        { // joysticks
            int countAxes;
            const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &countAxes);
            if (countAxes >= 2)
            {
                // printf("INFO: Left Stick X: %f | Left Stick Y: %f\n", axes[0], axes[1]);
            }
        }

        {
            static GLuint prog3, tex3, frame=0;
            if (!frame++)
            {
                int loadShader3(GLuint, const char*);
                prog3 = glCreateProgram();
                int err = loadShader3(prog3, "../renderVoxels.glsl");
                GLint iChannel3 = glGetUniformLocation(prog3, "iChannel3");
                glProgramUniform1i(prog3, iChannel3, 3);

                glGenTextures(1, &tex3);
                glBindTexture(GL_TEXTURE_2D, tex3);
                glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, RES_X, RES_Y);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            glBindImageTexture(2, tex3, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
            glUseProgram(prog3);
            glDispatchCompute(20,20,1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, tex3);
        }

        glClearColor(0,0,0,1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LESS);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0,0, RES_X, RES_Y);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        if (result2 > 0)
        {
            glUseProgram(prog2);
            glProgramUniform1f(prog2, enableWireFrame, 0);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, result2, 0);
        }

        if (result2 > 0 && result1)
        {
            glPointSize(2.0);
            glLineWidth(1.0);
            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glProgramUniform1f(prog2, enableWireFrame, 1);
            glMultiDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_SHORT, NULL, result2, 0);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, result2, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0,0, RES_X, RES_Y);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (result3 > 0)
        {
            glUseProgram(prog3);
            glDrawElements(GL_LINES, result3 & 0xff, GL_UNSIGNED_INT, NULL);
            glDrawArrays(GL_POINTS, 0, result3 >> 8);
        }

        glfwSwapBuffers(window1);
        glfwPollEvents();

#if ENABLE_FFMPEG
        static char buffer[RES_X*RES_Y*3];
        glReadPixels(0,0, RES_X, RES_Y, GL_RGB, GL_UNSIGNED_BYTE, buffer);
        fwrite(buffer, sizeof(buffer), 1, pipe);
        static int frame = 0;
        if (frame++ > 60*10) break;
#endif
    }

#if ENABLE_FFMPEG
    pclose(pipe);
#endif

    if (libraryHandle)
    {
        assert(dlclose(libraryHandle) == 0);
    }
    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
    glfwDestroyWindow(window1);
    glfwTerminate();
    return 0;
}
