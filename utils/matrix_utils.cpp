#include "matrix_utils.h"
#include <cmath>
#include <stdexcept>

namespace MatrixUtils {

std::vector<std::vector<double>>
multiply(const std::vector<std::vector<double>> &A,
         const std::vector<std::vector<double>> &B) {
  if (A.empty() || B.empty() || A[0].size() != B.size())
    throw std::invalid_argument("Matrix dimensions mismatch");
  size_t m = A.size(), n = A[0].size(), p = B[0].size();
  std::vector<std::vector<double>> C(m, std::vector<double>(p, 0.0));
  for (size_t i = 0; i < m; ++i)
    for (size_t j = 0; j < p; ++j)
      for (size_t k = 0; k < n; ++k)
        C[i][j] += A[i][k] * B[k][j];
  return C;
}

std::vector<std::vector<double>>
transpose(const std::vector<std::vector<double>> &A) {
  if (A.empty())
    return {};
  size_t m = A.size(), n = A[0].size();
  std::vector<std::vector<double>> AT(n, std::vector<double>(m));
  for (size_t i = 0; i < m; ++i)
    for (size_t j = 0; j < n; ++j)
      AT[j][i] = A[i][j];
  return AT;
}

std::vector<std::vector<double>>
inverse(const std::vector<std::vector<double>> &A) {
  size_t n = A.size();
  if (n == 0)
    return {};
  std::vector<std::vector<double>> aug(n, std::vector<double>(2 * n, 0.0));
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = 0; j < n; ++j)
      aug[i][j] = A[i][j];
    aug[i][n + i] = 1.0;
  }
  for (size_t i = 0; i < n; ++i) {
    size_t pivot = i;
    while (pivot < n && std::abs(aug[pivot][i]) < 1e-12)
      ++pivot;
    if (pivot == n)
      throw std::runtime_error("Matrix is singular");
    if (pivot != i)
      std::swap(aug[i], aug[pivot]);
    double diag = aug[i][i];
    for (size_t j = 0; j < 2 * n; ++j)
      aug[i][j] /= diag;
    for (size_t k = 0; k < n; ++k) {
      if (k != i && std::abs(aug[k][i]) > 1e-12) {
        double factor = aug[k][i];
        for (size_t j = 0; j < 2 * n; ++j)
          aug[k][j] -= factor * aug[i][j];
      }
    }
  }
  std::vector<std::vector<double>> inv(n, std::vector<double>(n));
  for (size_t i = 0; i < n; ++i)
    for (size_t j = 0; j < n; ++j)
      inv[i][j] = aug[i][n + j];
  return inv;
}

double determinantDiagonal(const std::vector<double> &diag) {
  double det = 1.0;
  for (double v : diag)
    det *= v;
  return det;
}

} // namespace MatrixUtils