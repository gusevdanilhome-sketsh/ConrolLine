#ifndef MATRIX_UTILS_H
#define MATRIX_UTILS_H

#include <vector>

namespace MatrixUtils {
std::vector<std::vector<double>>
multiply(const std::vector<std::vector<double>> &A,
         const std::vector<std::vector<double>> &B);
std::vector<std::vector<double>>
transpose(const std::vector<std::vector<double>> &A);
std::vector<std::vector<double>>
inverse(const std::vector<std::vector<double>> &A);
double determinantDiagonal(const std::vector<double> &diag);
} // namespace MatrixUtils

#endif