#include <iostream>
#include <vector>
#include <math.h>
#include <algorithm>
#include "matrix.hpp"
#include "BitmapPlusPlus.hpp"
#include <boost/program_options.hpp>
#include "Converters.hpp"

namespace po = boost::program_options;

template <class T, class F>
decltype(auto) map(const std::vector<T> a, const F fn)
{
    std::vector<decltype(fn(a[0]))> result = {};
    std::transform(a.cbegin(), a.cend(), std::back_inserter(result), fn);
    return result;
}

std::vector<po::option> ignore_numbers(std::vector<std::string> &args)
{
    std::vector<po::option> result;
    int pos = 0;
    while (!args.empty())
    {
        const auto &arg = args[0];
        double num;
        if (boost::conversion::try_lexical_convert(arg, num))
        {
            result.push_back(po::option());
            po::option &opt = result.back();

            opt.position_key = pos++;
            opt.value.push_back(arg);
            opt.original_tokens.push_back(arg);

            args.erase(args.begin());
        }
        else
            break;
    }

    return result;
}

std::vector<std::vector<double>> genMatrix(double angle,
                                           double horizontal_scale,
                                           double vertical_scale,
                                           double scale,
                                           double horizontal_skew,
                                           double vertical_skew)
{
    angle *= M_PI / 180;
    horizontal_skew *= M_PI / 180;
    vertical_skew *= M_PI / 180;

    double
        a = horizontal_scale * cos(angle),
        b = -horizontal_scale * (sin(angle) + tan(horizontal_skew)),
        c = vertical_scale * (sin(angle) + tan(vertical_skew)),
        d = vertical_scale * cos(angle);

    return {{a, b, 0},
            {c, d, 0},
            {0, 0, 1}};
}

int main(int argc, char *argv[])
{
    double angle,
        horizontal_scale,
        vertical_scale,
        scale,
        horizontal_skew,
        vertical_skew;

    int threads_number;
    int device;

    po::options_description desc("Allowed options");
    desc
        .add_options()                                                                                                                     // prettier-ignore
        ("help,h", "produce help message")                                                                                                 // prettier-ignore
        ("angle,a", po::value<double>(&angle)->default_value(0), "rotation angle")                                                         // prettier-ignore
        ("hsc", po::value<double>(&horizontal_scale)->default_value(1), "horizontal scale factor")                                         // prettier-ignore
        ("vsc", po::value<double>(&vertical_scale)->default_value(1), "vertical scale factor")                                             // prettier-ignore
        ("scale,s", po::value<double>(&scale), "scale factor (overrides hsc and vsc)")                                                     // prettier-ignore
        ("hsk", po::value<double>(&horizontal_skew)->default_value(0), "horizontal skew angle")                                            // prettier-ignore
        ("vsk", po::value<double>(&vertical_skew)->default_value(0), "vertical skew angle")                                                // prettier-ignore
        ("hf", "horizontal flip")                                                                                                          // prettier-ignore
        ("vf", "vertical flip")                                                                                                            // prettier-ignore
        ("matrix,m", po::value<std::vector<double>>()->multitoken(), "transformation matrix (2x2 as a1 a2 b1 b2) (overrides all options)") // prettier-ignore
        ("device,d", po::value<int>(&device)->default_value(1), "render device: 1) CPU 2) GPU")                                            // prettier-ignore
        ("threads,t", po::value<int>(&threads_number)->default_value(1), "threads count (available only for CPU rendering)");              // prettier-ignore

    po::options_description hidden;
    hidden.add_options()                           // prettier-ignore
        ("input-file", po::value<std::string>())   // prettier-ignore
        ("output-file", po::value<std::string>()); // prettier-ignore

    po::options_description options;
    options.add(desc);
    options.add(hidden);

    po::positional_options_description positional;
    positional.add("input-file", 1);
    positional.add("output-file", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                  .extra_style_parser(ignore_numbers)
                  .allow_unregistered()
                  .options(options)
                  .positional(positional)
                  .run(),
              vm);

    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl
                  << "Usage: affine_transform.exe input output options" << std::endl;
        return 1;
    }

    if (!vm.count("input-file"))
    {
        std::cout << "Provide input file" << std::endl;
        return 1;
    }

    if (!vm.count("output-file"))
    {
        std::cout << "Provide output file" << std::endl;
        return 1;
    }

    std::vector<std::vector<double>> matrix;

    if (!vm.count("matrix"))
    {
        if (vm.count("scale"))
            horizontal_scale = vertical_scale = vm["scale"].as<double>();

        if (vm.count("hf"))
            horizontal_scale *= -1;

        if (vm.count("vf"))
            vertical_scale *= -1;

        double intpart;

        if (std::modf(horizontal_skew, &intpart) == 0.0 &&
            ((int)intpart % 180) == 90)
        {
            std::cout << "Invalid horizontal skew" << std::endl;
            return 1;
        }

        if (std::modf(vertical_skew, &intpart) == 0.0 &&
            ((int)intpart % 180) == 90)
        {
            std::cout << "Invalid vertical skew" << std::endl;
            return 1;
        }

        matrix = genMatrix(
            angle,
            horizontal_scale,
            vertical_scale,
            scale,
            horizontal_skew,
            vertical_skew);
    }
    else
    {
        auto vector = vm["matrix"].as<std::vector<double>>();

        if (vector.size() != 4)
        {
            std::cout << "Transform matrix is invalid" << std::endl;
            return 1;
        }

        matrix = {
            {vector[0], vector[1], 0},
            {vector[2], vector[3], 0},
            {0, 0, 1}};
    }

    auto invMatrix = inverseMatrix(matrix);

    try
    {
        bmp::Bitmap input;

        input.load(vm["input-file"].as<std::string>());

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

        std::string out = vm["output-file"].as<std::string>();

        bmp::Bitmap output;

        if (device == 1)
        {
            output = CPURender(width, height,
                               new_width, new_height,
                               x_offset, y_offset,
                               invMatrix, input,
                               threads_number);
        }
        else if (device == 2)
        {
            output = GPURender(width, height,
                               new_width, new_height,
                               x_offset, y_offset,
                               invMatrix, input);
        }
        else
        {
            std::cout << "Invalid render device" << std::endl;
            return 1;
        }

        output.save(out);

        std::cout << "Done" << std::endl;

        return 0;
    }
    catch (const bmp::Exception &e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }
}
