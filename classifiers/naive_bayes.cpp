#include "naive_bayes.h"
#include <algorithm>
#include <cmath>
#include <numeric>


void NaiveBayes::train(const std::vector<std::vector<double>> &X,
                       const std::vector<int> &y) {
  if (X.empty() || y.empty())
    return;
  num_features_ = (int)X[0].size();
  num_classes_ = *std::max_element(y.begin(), y.end()) + 1;
  means_.assign(num_classes_, std::vector<double>(num_features_, 0.0));
  variances_.assign(num_classes_, std::vector<double>(num_features_, 0.0));
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
  for (int i = 0; i < total; ++i) {
    int c = y[i];
    for (int j = 0; j < num_features_; ++j) {
      double diff = X[i][j] - means_[c][j];
      variances_[c][j] += diff * diff;
    }
  }
  for (int c = 0; c < num_classes_; ++c) {
    if (counts[c] > 1) {
      for (int j = 0; j < num_features_; ++j)
        variances_[c][j] /= (counts[c] - 1);
    } else {
      for (int j = 0; j < num_features_; ++j)
        variances_[c][j] = 1e-9;
    }
  }
  priors_.resize(num_classes_);
  for (int c = 0; c < num_classes_; ++c)
    priors_[c] = (double)counts[c] / total;
  trained_ = true;
}

std::vector<int>
NaiveBayes::predict(const std::vector<std::vector<double>> &X) const {
  if (!trained_ || X.empty())
    return std::vector<int>(X.size(), 0);
  std::vector<int> predictions(X.size());
  for (size_t i = 0; i < X.size(); ++i) {
    double bestLogProb = -1e100;
    int bestClass = 0;
    for (int c = 0; c < num_classes_; ++c) {
      double logProb = std::log(priors_[c]);
      for (int j = 0; j < num_features_; ++j) {
        double diff = X[i][j] - means_[c][j];
        double var = variances_[c][j];
        logProb -= 0.5 * std::log(2 * M_PI * var);
        logProb -= 0.5 * diff * diff / var;
      }
      if (logProb > bestLogProb) {
        bestLogProb = logProb;
        bestClass = c;
      }
    }
    predictions[i] = bestClass;
  }
  return predictions;
}