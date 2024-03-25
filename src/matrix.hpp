#include <iostream>
#include <vector>

void printMatrix(const std::vector<std::vector<double>> &matrix)
{
    for (const auto &row : matrix)
    {
        for (double element : row)
            std::cout << element << " ";

        std::cout << std::endl;
    }
    std::cout << std::endl;
}

std::vector<std::vector<double>> inverseMatrix(const std::vector<std::vector<double>> &matrix)
{
    int n = matrix.size();
    std::vector<std::vector<double>> tempMatrix = matrix;

    std::vector<std::vector<double>> identityMatrix(n, std::vector<double>(n, 0));

    for (int i = 0; i < n; ++i)
        identityMatrix[i][i] = 1;

    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            if (i != j)
            {
                double ratio = tempMatrix[j][i] / tempMatrix[i][i];
                for (int k = 0; k < n; ++k)
                {
                    tempMatrix[j][k] -= ratio * tempMatrix[i][k];
                    identityMatrix[j][k] -= ratio * identityMatrix[i][k];
                }
            }
        }
    }

    for (int i = 0; i < n; ++i)
    {
        double divisor = tempMatrix[i][i];
        for (int j = 0; j < n; ++j)
        {
            tempMatrix[i][j] /= divisor;
            identityMatrix[i][j] /= divisor;
        }
    }

    return identityMatrix;
}

std::vector<std::vector<double>> multiplyMatrices(const std::vector<std::vector<double>> &firstMatrix,
                                                  const std::vector<std::vector<double>> &secondMatrix)
{
    int rowFirst = firstMatrix.size();
    int columnFirst = firstMatrix[0].size();
    int rowSecond = secondMatrix.size();
    int columnSecond = secondMatrix[0].size();

    std::vector<std::vector<double>> result(rowFirst, std::vector<double>(columnSecond, 0));

    for (int i = 0; i < rowFirst; ++i)
    {
        for (int j = 0; j < columnSecond; ++j)
        {
            for (int k = 0; k < columnFirst; ++k)
                result[i][j] += firstMatrix[i][k] * secondMatrix[k][j];
        }
    }

    return result;
}