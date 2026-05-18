#include "lda.h"
#include "../utils/matrix_utils.h"
#include <algorithm>
#include <cmath>
#include <numeric>


void LDA::train(const std::vector<std::vector<double>> &X,
                const std::vector<int> &y) {
  if (X.empty() || y.empty())
    return;
  num_features_ = (int)X[0].size();
  num_classes_ = *std::max_element(y.begin(), y.end()) + 1;
  means_.assign(num_classes_, std::vector<double>(num_features_, 0.0));
  std::vector<int> counts(num_classes_, 0);
  int total = (int)X.size();
  for (int i = 0; i < total; ++i) {
    int c = y[i];
    counts[c]++;
    for (int j = 0; j < num_features_; ++j)
      means_[c][j] += X[i][j];
  }
  for (int c = 0; c < num_classes_; ++c) {
    if (counts[c] > 0) {
      for (int j = 0; j < num_features_; ++j)
        means_[c][j] /= counts[c];
    }
  }
  std::vector<std::vector<double>> Sw(num_features_,
                                      std::vector<double>(num_features_, 0.0));
  for (int i = 0; i < total; ++i) {
    int c = y[i];
    std::vector<double> diff(num_features_);
    for (int j = 0; j < num_features_; ++j)
      diff[j] = X[i][j] - means_[c][j];
    for (int j = 0; j < num_features_; ++j)
      for (int k = 0; k < num_features_; ++k)
        Sw[j][k] += diff[j] * diff[k];
  }
  for (int j = 0; j < num_features_; ++j)
    for (int k = 0; k < num_features_; ++k)
      Sw[j][k] /= (total - num_classes_);
  try {
    invCov_ = MatrixUtils::inverse(Sw);
  } catch (...) {
    for (int j = 0; j < num_features_; ++j)
      Sw[j][j] += 1e-6;
    invCov_ = MatrixUtils::inverse(Sw);
  }
  priors_.resize(num_classes_);
  for (int c = 0; c < num_classes_; ++c)
    priors_[c] = (double)counts[c] / total;
  trained_ = true;
}

std::vector<int> LDA::predict(const std::vector<std::vector<double>> &X) const {
  if (!trained_ || X.empty())
    return std::vector<int>(X.size(), 0);
  std::vector<int> predictions(X.size());
  for (size_t i = 0; i < X.size(); ++i) {
    double bestScore = -1e100;
    int bestClass = 0;
    for (int c = 0; c < num_classes_; ++c) {
      std::vector<double> diff(num_features_);
      for (int j = 0; j < num_features_; ++j)
        diff[j] = X[i][j] - means_[c][j];
      double quad = 0.0;
      for (int j = 0; j < num_features_; ++j) {
        double temp = 0.0;
        for (int k = 0; k < num_features_; ++k)
          temp += invCov_[j][k] * diff[k];
        quad += diff[j] * temp;
      }
      double score = -0.5 * quad + std::log(priors_[c]);
      if (score > bestScore) {
        bestScore = score;
        bestClass = c;
      }
    }
    predictions[i] = bestClass;
  }
  return predictions;
}