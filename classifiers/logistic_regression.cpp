#include "logistic_regression.h"
#include <algorithm>
#include <random>


void LogisticRegression::train(const std::vector<std::vector<double>> &X,
                               const std::vector<int> &y) {
  if (X.empty())
    return;
  num_features_ = (int)X[0].size();
  num_classes_ = *std::max_element(y.begin(), y.end()) + 1;
  // Простая заглушка: веса не обучаем
}

std::vector<int>
LogisticRegression::predict(const std::vector<std::vector<double>> &X) const {
  std::vector<int> pred(X.size(), 0);
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 4);
  for (size_t i = 0; i < X.size(); ++i) {
    pred[i] = dis(gen); // случайное угадывание
  }
  return pred;
}