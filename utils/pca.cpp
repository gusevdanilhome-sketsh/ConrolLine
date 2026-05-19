#include "pca.h"
#include <Eigen/Dense>
#include <algorithm>
#include <numeric>

void PCA::fit(const std::vector<std::vector<double>> &data) {
  if (data.empty())
    return;
  size_t n = data.size();
  size_t dim = data[0].size();

  Eigen::MatrixXd mat(n, dim);
  for (size_t i = 0; i < n; ++i)
    for (size_t j = 0; j < dim; ++j)
      mat(i, j) = data[i][j];

  mean_.resize(dim);
  for (size_t j = 0; j < dim; ++j) {
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i)
      sum += data[i][j];
    mean_[j] = sum / n;
    mat.col(j).array() -= mean_[j];
  }

  Eigen::MatrixXd cov = (mat.adjoint() * mat) / (n - 1);
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(cov);
  Eigen::MatrixXd eigenvecs = solver.eigenvectors();

  components_.resize(dim);
  for (size_t j = 0; j < dim; ++j) {
    components_[j].resize(dim);
    for (size_t i = 0; i < dim; ++i)
      components_[j][i] = eigenvecs(i, dim - 1 - j);
  }
}

std::vector<std::vector<double>>
PCA::transform(const std::vector<std::vector<double>> &data, int nComponents) {
  std::vector<std::vector<double>> result(data.size(),
                                          std::vector<double>(nComponents));
  for (size_t i = 0; i < data.size(); ++i) {
    for (int k = 0; k < nComponents; ++k) {
      double val = 0.0;
      for (size_t j = 0; j < data[i].size(); ++j)
        val += (data[i][j] - mean_[j]) * components_[k][j];
      result[i][k] = val;
    }
  }
  return result;
}