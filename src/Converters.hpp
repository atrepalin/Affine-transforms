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

bmp::Pixel bilinearInterpolation(
    const bmp::Pixel &p1,
    const bmp::Pixel &p2,
    const bmp::Pixel &p3,
    const bmp::Pixel &p4,
    double d1,
    double d2,
    double d3,
    double d4)
{
    bmp::Pixel result;

    result.r = p1.r * d1 + p2.r * d2 + p3.r * d3 + p4.r * d4;
    result.g = p1.g * d1 + p2.g * d2 + p3.g * d3 + p4.g * d4;
    result.b = p1.b * d1 + p2.b * d2 + p3.b * d3 + p4.b * d4;

    return result;
}

bmp::Bitmap CPURender(int width, int height,
                      int new_width, int new_height,
                      int x_offset, int y_offset,
                      std::vector<std::vector<double>> invMatrix,
                      bmp::Bitmap input,
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
                try
                {
                    std::vector<std::vector<double>> transformed_coord;
                    for (int new_x = i * chunk_size; new_x < std::min((i + 1) * chunk_size, new_width); ++new_x)
                    {
                        for (int new_y = 0; new_y < new_height; ++new_y)
                        {
                            transformed_coord = {{(double)(new_x + x_offset), (double)(new_y + y_offset), 1}};

                            auto coord = multiplyMatrices(transformed_coord, invMatrix);

                            double x = coord[0][0],
                                   y = coord[0][1];

                            if (x < 0 || x >= width || y < 0 || y >= height)
                                continue;

                            int ix = std::floor(x),
                                iy = std::floor(y);

                            auto p1 = input.get(ix, iy),
                                 p2 = input.get(ix + (ix < width - 1), iy),
                                 p3 = input.get(ix, iy + (iy < height - 1)),
                                 p4 = input.get(ix + (ix < width - 1), iy + (iy < height - 1));

                            double t = x - ix,
                                   u = y - iy,
                                   d1 = (1 - t) * (1 - u),
                                   d2 = t * (1 - u),
                                   d3 = t * u,
                                   d4 = (1 - t) * u;

                            auto pixel = bilinearInterpolation(p1, p2, p3, p4,
                                                               d1, d2, d3, d4);

                            output.set(new_x, new_y, pixel);
                        }

                        progress += 1.0 / new_width;

                        if (new_x % checkpoint == 0)
                        {
                            const std::unique_lock<std::mutex> lock(mutex);
                            printProgress(progress);
                        }
                    }
                }
                catch (const std::runtime_error &e)
                {
                    std::cout << e.what() << std::endl;
                }
            });
    }

    for (auto &t : threads)
        t.join();

    printProgress(1);

    std::cout << std::endl;

    return output;
}

bmp::Bitmap GPURender(int width, int height,
                      int new_width, int new_height,
                      int x_offset, int y_offset,
                      std::vector<std::vector<double>> invMatrix,
                      bmp::Bitmap input)
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

    bmp::Bitmap output(buffer_width, buffer_height);
    auto pixels = reinterpret_cast<unsigned char *>(output.m_pixels.data());
    glReadPixels(0, 0, buffer_width, buffer_height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    glDeleteProgram(ShaderProgram);
    glDeleteShader(FragmentShader);
    glDeleteShader(VertexShader);

    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glfwTerminate();

    return output;
}