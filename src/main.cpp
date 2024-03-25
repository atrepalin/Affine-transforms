#include <iostream>
#include <vector>
#include <math.h>
#include <algorithm>
#include "matrix.hpp"
#include "BitmapPlusPlus.hpp"

template <class T, class F>
decltype(auto) map(const std::vector<T> a, const F fn)
{
    std::vector<decltype(fn(a[0]))> result = {};
    std::transform(a.cbegin(), a.cend(), std::back_inserter(result), fn);
    return result;
}

int main(int argc, char *argv[])
{
    double angle,
        horizontal_scale,
        vertical_scale,
        scale,
        horizontal_skew,
        vertical_skew;

    angle *= M_PI / 180;
    horizontal_skew *= M_PI / 180;
    vertical_skew *= M_PI / 180;

    double a, b, c, d;

    a = horizontal_scale * cos(angle),
    b = -horizontal_scale * (sin(angle) + tan(horizontal_skew)),
    c = vertical_scale * (sin(angle) + tan(vertical_skew)),
    d = vertical_scale * cos(angle);

    std::vector<std::vector<double>> matrix = {
        {a, b, 0},
        {c, d, 0},
        {0, 0, 1}};

    auto invMatrix = inverseMatrix(matrix);

    bmp::Bitmap input;

    input.load("test.bmp");

    double width = input.width(),
           height = input.height();

    std::vector<std::vector<std::vector<double>>> corners = {
        {{0, 0, 1}},
        {{width, 0, 1}},
        {{0, height, 1}},
        {{width, height, 1}}};

    auto transformedCorners = map(corners, [matrix](const auto corner)
                                  { return multiplyMatrices(corner, matrix); });

    auto x = map(transformedCorners, [](const auto corner)
                 { return (int)ceil(corner[0][0]); }),
         y = map(transformedCorners, [](const auto corner)
                 { return (int)ceil(corner[0][1]); });

    auto horizontal = std::minmax_element(std::begin(x), std::end(x)),
         vertical = std::minmax_element(std::begin(y), std::end(y));

    int new_width = *horizontal.second - *horizontal.first,
        new_height = *vertical.second - *vertical.first,
        x_offset = *horizontal.first,
        y_offset = *vertical.first;

    bmp::Bitmap output(new_width, new_height);

    for (int new_x = 0; new_x < new_width; ++new_x)
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
    }

    output.save("output.bmp");
}
