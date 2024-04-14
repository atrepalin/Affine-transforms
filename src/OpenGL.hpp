#include <GL/glew.h>
#include <GLFW/glfw3.h>

void PrintShaderInfoLog(GLint const Shader)
{
    int InfoLogLength = 0;
    int CharsWritten = 0;

    glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &InfoLogLength);

    if (InfoLogLength > 0)
    {
        GLchar *InfoLog = new GLchar[InfoLogLength];
        glGetShaderInfoLog(Shader, InfoLogLength, &CharsWritten, InfoLog);
        std::cout << "Shader Info Log:" << std::endl
                  << InfoLog << std::endl;
        delete[] InfoLog;
    }
}

void init(int width, int height)
{
    glfwInit();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(width, height, "", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();

    GLuint fb;

    GLuint rb;

    GLuint tex;

    glGenFramebuffers(1, &fb);

    glGenRenderbuffers(1, &rb);

    glGenTextures(1, &tex);

    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    glBindRenderbuffer(GL_RENDERBUFFER, rb);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, width, height);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);

    glBindTexture(GL_TEXTURE_2D, tex);

    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glViewport(0, 0, width, height);
}

void createBuffers(GLuint *VAO, GLuint *VBO, GLuint *EBO)
{
    GLfloat const Vertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f,
        -1.0f, 1.0f};

    GLuint const Elements[] = {
        0, 1, 2, 0, 2, 3};

    glGenVertexArrays(1, VAO);
    glBindVertexArray(*VAO);

    glGenBuffers(1, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Elements), Elements, GL_STATIC_DRAW);
}

GLuint createShader(GLuint *VertexShader, GLuint *FragmentShader, GLint *PositionAttribute)
{
    char const *VertexShaderSource = R"GLSL(
		#version 150
		in vec2 position;
		void main()
		{
			gl_Position = vec4(position, 0.0, 1.0);
		}
	)GLSL";

    char const *FragmentShaderSource = R"GLSL(
        #version 150
        out vec4 outColor;
        uniform sampler2D image;
        uniform uint width;
        uniform uint height;
        uniform int x_offset;
        uniform int y_offset;
        uniform float a;
        uniform float b;
        uniform float c;
        uniform float d;

        vec4 bilinearInterpolation(vec3 p1, vec3 p2, vec3 p3, vec3 p4, float dx, float dy) {
            vec3 top = mix(p1, p2, dx);
            vec3 bottom = mix(p3, p4, dx);
            return vec4(mix(top, bottom, dy), 1);
        }

        void main()
        {
            vec2 new_coord = gl_FragCoord.xy;

            vec3 coord = vec3(new_coord + vec2(x_offset, y_offset), 1);

            coord *= mat3(a, c, 0, b, d, 0, 0, 0, 1);

            if(coord.x > 0 && coord.x < width && coord.y > 0 && coord.y < height) {
                vec2 xy = vec2(floor(coord.x), floor(coord.y));
                vec2 size = vec2(width, height);

                vec3 p1 = texture2D(image, xy / size).rgb,
                     p2 = texture2D(image, (xy + vec2(1, 0)) / size).rgb,
                     p3 = texture2D(image, (xy + vec2(0, 1)) / size).rgb,
                     p4 = texture2D(image, (xy + vec2(1, 1)) / size).rgb;

                vec2 dc = coord.xy - xy;

                outColor = bilinearInterpolation(p1, p2, p3, p4, dc.x, dc.y);
            }
        }
    )GLSL";

    GLint Compiled;
    *VertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(*VertexShader, 1, &VertexShaderSource, NULL);
    glCompileShader(*VertexShader);
    glGetShaderiv(*VertexShader, GL_COMPILE_STATUS, &Compiled);
    if (!Compiled)
    {
        std::cerr << "Failed to compile vertex shader" << std::endl;
        PrintShaderInfoLog(*VertexShader);
        exit(1);
    }

    *FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(*FragmentShader, 1, &FragmentShaderSource, NULL);
    glCompileShader(*FragmentShader);
    glGetShaderiv(*FragmentShader, GL_COMPILE_STATUS, &Compiled);
    if (!Compiled)
    {
        std::cout << "Failed to compile fragment shader" << std::endl;
        PrintShaderInfoLog(*FragmentShader);
        exit(1);
    }

    GLuint ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, *VertexShader);
    glAttachShader(ShaderProgram, *FragmentShader);
    glBindFragDataLocation(ShaderProgram, 0, "outColor");
    glLinkProgram(ShaderProgram);
    glUseProgram(ShaderProgram);

    *PositionAttribute = glGetAttribLocation(ShaderProgram, "position");
    glEnableVertexAttribArray(*PositionAttribute);

    return ShaderProgram;
}

void configureShader(int width, int height, int x_offset, int y_offset,
                     std::vector<std::vector<double>> invMatrix,
                     bmp::Bitmap input, GLuint ShaderProgram)
{
    GLuint textureID[1];
    glGenTextures(1, textureID);

    glBindTexture(GL_TEXTURE_2D, textureID[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)&input.m_pixels[0]);

    GLint imageLoc = glGetUniformLocation(ShaderProgram, "image");
    glUniform1i(imageLoc, 0);

    GLint widthLoc = glGetUniformLocation(ShaderProgram, "width");
    glUniform1ui(widthLoc, width);

    GLint heightLoc = glGetUniformLocation(ShaderProgram, "height");
    glUniform1ui(heightLoc, height);

    GLint x_offsetLoc = glGetUniformLocation(ShaderProgram, "x_offset");
    glUniform1i(x_offsetLoc, x_offset);

    GLint y_offsetLoc = glGetUniformLocation(ShaderProgram, "y_offset");
    glUniform1i(y_offsetLoc, y_offset);

    GLint aLoc = glGetUniformLocation(ShaderProgram, "a");
    glUniform1f(aLoc, invMatrix[0][0]);

    GLint bLoc = glGetUniformLocation(ShaderProgram, "b");
    glUniform1f(bLoc, invMatrix[0][1]);

    GLint cLoc = glGetUniformLocation(ShaderProgram, "c");
    glUniform1f(cLoc, invMatrix[1][0]);

    GLint dLoc = glGetUniformLocation(ShaderProgram, "d");
    glUniform1f(dLoc, invMatrix[1][1]);

    glActiveTexture(GL_TEXTURE0);
}