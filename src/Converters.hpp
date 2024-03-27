#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <algorithm>
#include "BitmapPlusPlus.hpp"
#include <thread>
#include <mutex>
#include <iomanip>
#include "OpenGL.hpp"

void printProgress(const double progress)
{
    std::cout << "[";
    int pos = 100 * progress;
    for (int i = 0; i < 100; ++i)
    {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] " << std::setprecision(1) << int(progress * 1000) / 10.0 << std::fixed << " %\r";
    std::cout.flush();
}

std::mutex mutex;

void CPU(int width, int height,
         int new_width, int new_height,
         int x_offset, int y_offset,
         std::vector<std::vector<double>> invMatrix,
         bmp::Bitmap input,
         std::string out,
         int threads_number)
{
    bmp::Bitmap output(new_width, new_height);

    int chunk_size = ceil(new_width / (double)threads_number);

    std::vector<std::thread> threads(threads_number);

    double progress = 0;
    int checkpoint = (int)ceil(new_width / 100.0);

    for (int i = 0; i < threads_number; ++i)
    {
        threads[i] = std::thread(
            [=, &output, &progress] // prettier-ignore
            {                       // prettier-ignore
                for (int new_x = i * chunk_size; new_x < std::min((i + 1) * chunk_size, new_width); ++new_x)
                {
                    for (int new_y = 0; new_y < new_height; ++new_y)
                    {
                        std::vector<std::vector<double>> transformed_coord = {{(double)(new_x + x_offset), (double)(new_y + y_offset), 1}};

                        auto coord = multiplyMatrices(transformed_coord, invMatrix);

                        int x = round(coord[0][0]),
                            y = round(coord[0][1]);

                        if (x < 0 || x >= width || y < 0 || y >= height)
                            continue;

                        output.set(new_x, new_y, input.get(x, y));
                    }

                    progress += 1.0 / new_width;

                    if (new_x % checkpoint == 0)
                    {
                        mutex.lock();
                        printProgress(progress);
                        mutex.unlock();
                    }
                }
            });
    }

    for (auto &t : threads)
        t.join();

    printProgress(1);

    output.save(out);

    std::cout << std::endl;
}

void GPU(int width, int height,
         int new_width, int new_height,
         int x_offset, int y_offset,
         std::vector<std::vector<double>> invMatrix,
         bmp::Bitmap input,
         std::string out)
{
    init(new_width, new_height);

    GLuint VAO, VBO, EBO;

    createBuffers(&VAO, &VBO, &EBO);

    GLuint VertexShader, FragmentShader;
    GLint PositionAttribute;

    GLuint ShaderProgram = createShader(&VertexShader, &FragmentShader, &PositionAttribute);

    configureShader(width, height, x_offset, y_offset,
                    invMatrix, input, ShaderProgram);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(PositionAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    int buffer_width = ceil(new_width / 4.0) * 4,
        buffer_height = ceil(new_height / 4.0) * 4;

    unsigned char *pixels = new unsigned char[buffer_width * buffer_height * 3];
    glReadPixels(0, 0, buffer_width, buffer_height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    glDeleteProgram(ShaderProgram);
    glDeleteShader(FragmentShader);
    glDeleteShader(VertexShader);

    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    bmp::Bitmap output(buffer_width, buffer_height);

    output.save(out, pixels);

    delete[] pixels;

    glfwTerminate();
}