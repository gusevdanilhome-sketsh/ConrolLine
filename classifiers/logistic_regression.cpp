#include "logistic_regression.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

LogisticRegression::LogisticRegression()
    : learning_rate_(0.01), max_epochs_(500), lambda_(0.01), batch_size_(32),
      num_classes_(0), num_features_(0) {}

void LogisticRegression::setHyperparameters(double learningRate, int maxEpochs,
                                            double lambda, int batchSize) {
  learning_rate_ = learningRate;
  max_epochs_ = maxEpochs;
  lambda_ = lambda;
  batch_size_ = batchSize;
}

void LogisticRegression::softmax(std::vector<double> &logits) const {
  double maxVal = *std::max_element(logits.begin(), logits.end());
  double sum = 0.0;
  for (double &v : logits) {
    v = std::exp(v - maxVal);
    sum += v;
  }
  for (double &v : logits)
    v /= sum;
}

double
LogisticRegression::computeLoss(const std::vector<std::vector<double>> &X,
                                const std::vector<int> &y) const {
  double loss = 0.0;
  int n = (int)X.size();
  for (int i = 0; i < n; ++i) {
    std::vector<double> scores(num_classes_, 0.0);
    for (int c = 0; c < num_classes_; ++c) {
      double dot = biases_[c];
      for (int j = 0; j < num_features_; ++j)
        dot += weights_[c][j] * X[i][j];
      scores[c] = dot;
    }
    softmax(scores);
    loss -= std::log(scores[y[i]] + 1e-15);
  }
  double regLoss = 0.0;
  for (int c = 0; c < num_classes_; ++c)
    for (int j = 0; j < num_features_; ++j)
      regLoss += weights_[c][j] * weights_[c][j];
  return loss / n + 0.5 * lambda_ * regLoss;
}

void LogisticRegression::computeGradients(
    const std::vector<std::vector<double>> &batchX,
    const std::vector<int> &batchY, std::vector<std::vector<double>> &gradW,
    std::vector<double> &gradB) const {
  int batchSize = (int)batchX.size();
  gradW.assign(num_classes_, std::vector<double>(num_features_, 0.0));
  gradB.assign(num_classes_, 0.0);
  for (int i = 0; i < batchSize; ++i) {
    std::vector<double> scores(num_classes_);
    for (int c = 0; c < num_classes_; ++c) {
      double dot = biases_[c];
      for (int j = 0; j < num_features_; ++j)
        dot += weights_[c][j] * batchX[i][j];
      scores[c] = dot;
    }
    softmax(scores);
    for (int c = 0; c < num_classes_; ++c) {
      double error = scores[c] - (c == batchY[i] ? 1.0 : 0.0);
      for (int j = 0; j < num_features_; ++j)
        gradW[c][j] += error * batchX[i][j];
      gradB[c] += error;
    }
  }
  for (int c = 0; c < num_classes_; ++c) {
    for (int j = 0; j < num_features_; ++j)
      gradW[c][j] /= batchSize;
    gradB[c] /= batchSize;
  }
  for (int c = 0; c < num_classes_; ++c)
    for (int j = 0; j < num_features_; ++j)
      gradW[c][j] += lambda_ * weights_[c][j];
}

void LogisticRegression::train(const std::vector<std::vector<double>> &X,
                               const std::vector<int> &y) {
  if (X.empty())
    return;
  num_features_ = (int)X[0].size();
  num_classes_ = *std::max_element(y.begin(), y.end()) + 1;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<> dist(0.0, 0.01);
  weights_.resize(num_classes_, std::vector<double>(num_features_));
  biases_.assign(num_classes_, 0.0);
  for (int c = 0; c < num_classes_; ++c)
    for (int j = 0; j < num_features_; ++j)
      weights_[c][j] = dist(gen);
  std::vector<int> indices(X.size());
  std::iota(indices.begin(), indices.end(), 0);
  for (int epoch = 0; epoch < max_epochs_; ++epoch) {
    std::shuffle(indices.begin(), indices.end(), gen);
    for (size_t start = 0; start < indices.size(); start += batch_size_) {
      size_t end = std::min(start + batch_size_, indices.size());
      std::vector<std::vector<double>> batchX;
      std::vector<int> batchY;
      for (size_t k = start; k < end; ++k) {
        batchX.push_back(X[indices[k]]);
        batchY.push_back(y[indices[k]]);
      }
      std::vector<std::vector<double>> gradW;
      std::vector<double> gradB;
      computeGradients(batchX, batchY, gradW, gradB);
      for (int c = 0; c < num_classes_; ++c) {
        for (int j = 0; j < num_features_; ++j)
          weights_[c][j] -= learning_rate_ * gradW[c][j];
        biases_[c] -= learning_rate_ * gradB[c];
      }
    }
  }
}

std::vector<int>
LogisticRegression::predict(const std::vector<std::vector<double>> &X) const {
  std::vector<int> predictions(X.size());
  for (size_t i = 0; i < X.size(); ++i) {
    std::vector<double> scores(num_classes_);
    for (int c = 0; c < num_classes_; ++c) {
      double dot = biases_[c];
      for (int j = 0; j < num_features_; ++j)
        dot += weights_[c][j] * X[i][j];
      scores[c] = dot;
    }
    int bestClass = 0;
    double bestScore = scores[0];
    for (int c = 1; c < num_classes_; ++c) {
      if (scores[c] > bestScore) {
        bestScore = scores[c];
        bestClass = c;
      }
    }
    predictions[i] = bestClass;
  }
  return predictions;
}

std::vector<std::vector<double>> LogisticRegression::predictProbabilities(
    const std::vector<std::vector<double>> &X) const {
  std::vector<std::vector<double>> probs(
      X.size(), std::vector<double>(num_classes_, 0.0));
  for (size_t i = 0; i < X.size(); ++i) {
    std::vector<double> scores(num_classes_, 0.0);
    for (int c = 0; c < num_classes_; ++c) {
      double dot = biases_[c];
      for (int j = 0; j < num_features_; ++j)
        dot += weights_[c][j] * X[i][j];
      scores[c] = dot;
    }
    softmax(scores);
    probs[i] = scores;
  }
  return probs;
}

std::vector<int> LogisticRegression::predictWithLoss(
    const std::vector<std::vector<double>> &X,
    const std::vector<std::vector<double>> &lossMatrix) const {
  auto probs = predictProbabilities(X);
  std::vector<int> predictions(X.size());
  int numCl = num_classes_;
  for (size_t i = 0; i < X.size(); ++i) {
    std::vector<double> expectedLoss(numCl, 0.0);
    for (int predClass = 0; predClass < numCl; ++predClass) {
      for (int trueClass = 0; trueClass < numCl; ++trueClass) {
        expectedLoss[predClass] +=
            lossMatrix[predClass][trueClass] * probs[i][trueClass];
      }
    }
    int best = 0;
    double minLoss = expectedLoss[0];
    for (int c = 1; c < numCl; ++c) {
      if (expectedLoss[c] < minLoss) {
        minLoss = expectedLoss[c];
        best = c;
      }
    }
    predictions[i] = best;
  }
  return predictions;
}