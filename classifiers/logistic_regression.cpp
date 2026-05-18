#include "logistic_regression.h"
#include <algorithm>
#include <random>


void LogisticRegression::train(const std::vector<std::vector<double>> & /*X*/,
                               const std::vector<int> & /*y*/) {
  // Заглушка: ничего не делаем
}

std::vector<int>
LogisticRegression::predict(const std::vector<std::vector<double>> &X) const {
  std::vector<int> pred(X.size(), 0);
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 4);
  for (size_t i = 0; i < X.size(); ++i) {
    pred[i] = dis(gen);
  }
  return pred;
}