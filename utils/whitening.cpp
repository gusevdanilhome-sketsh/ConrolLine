#include "whitening.h"
#include <Eigen/Dense>
#include <cmath>
#include <stdexcept>


void Whitening::fit(const std::vector<std::vector<double>> &X) {
  if (X.empty())
    return;
  nFeatures_ = static_cast<int>(X[0].size());
  size_t nSamples = X.size();
  mean_.assign(nFeatures_, 0.0);
  for (const auto &row : X)
    for (int j = 0; j < nFeatures_; ++j)
      mean_[j] += row[j];
  for (int j = 0; j < nFeatures_; ++j)
    mean_[j] /= static_cast<double>(nSamples);

  Eigen::MatrixXd centered(nSamples, nFeatures_);
  for (size_t i = 0; i < nSamples; ++i)
    for (int j = 0; j < nFeatures_; ++j)
      centered(i, j) = X[i][j] - mean_[j];

  Eigen::MatrixXd cov = centered.adjoint() * centered / (nSamples - 1);
  Eigen::LLT<Eigen::MatrixXd> llt(cov);
  if (llt.info() != Eigen::Success) {
    cov += Eigen::MatrixXd::Identity(nFeatures_, nFeatures_) * 1e-6;
    llt.compute(cov);
    if (llt.info() != Eigen::Success)
      throw std::runtime_error(
          "Whitening: covariance not positive definite after regularization");
  }
  Eigen::MatrixXd L = llt.matrixL();

  L_.assign(nFeatures_, std::vector<double>(nFeatures_, 0.0));
  for (int i = 0; i < nFeatures_; ++i)
    for (int j = 0; j <= i; ++j)
      L_[i][j] = L(i, j);

  fitted_ = true;
}

std::vector<std::vector<double>>
Whitening::transform(const std::vector<std::vector<double>> &X) const {
  if (!fitted_ || X.empty())
    return {};
  int n = static_cast<int>(X.size());
  std::vector<std::vector<double>> res(n, std::vector<double>(nFeatures_));

  Eigen::MatrixXd Lmat(nFeatures_, nFeatures_);
  for (int i = 0; i < nFeatures_; ++i)
    for (int j = 0; j < nFeatures_; ++j)
      Lmat(i, j) = L_[i][j];

  for (int i = 0; i < n; ++i) {
    Eigen::VectorXd x(nFeatures_);
    for (int j = 0; j < nFeatures_; ++j)
      x(j) = X[i][j] - mean_[j];
    Eigen::VectorXd y = Lmat.triangularView<Eigen::Lower>().solve(x);
    for (int j = 0; j < nFeatures_; ++j)
      res[i][j] = y(j);
  }
  return res;
}

std::vector<std::vector<double>>
Whitening::inverseTransform(const std::vector<std::vector<double>> &X) const {
  if (!fitted_ || X.empty())
    return {};
  int n = static_cast<int>(X.size());
  std::vector<std::vector<double>> res(n, std::vector<double>(nFeatures_));

  Eigen::MatrixXd Lmat(nFeatures_, nFeatures_);
  for (int i = 0; i < nFeatures_; ++i)
    for (int j = 0; j < nFeatures_; ++j)
      Lmat(i, j) = L_[i][j];

  for (int i = 0; i < n; ++i) {
    Eigen::VectorXd y(nFeatures_);
    for (int j = 0; j < nFeatures_; ++j)
      y(j) = X[i][j];
    Eigen::VectorXd x = Lmat.triangularView<Eigen::Lower>() * y;
    for (int j = 0; j < nFeatures_; ++j)
      res[i][j] = x(j) + mean_[j];
  }
  return res;
}