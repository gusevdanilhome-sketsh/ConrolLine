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

  // 1. Вычисляем средние для каждого класса и общее среднее
  means_.assign(num_classes_, std::vector<double>(num_features_, 0.0));
  std::vector<int> counts(num_classes_, 0);
  std::vector<double> globalMean(num_features_, 0.0);
  int total = (int)X.size();
  for (int i = 0; i < total; ++i) {
    int c = y[i];
    counts[c]++;
    for (int j = 0; j < num_features_; ++j) {
      means_[c][j] += X[i][j];
      globalMean[j] += X[i][j];
    }
  }
  for (int c = 0; c < num_classes_; ++c) {
    if (counts[c] > 0) {
      for (int j = 0; j < num_features_; ++j)
        means_[c][j] /= counts[c];
    }
  }
  for (int j = 0; j < num_features_; ++j)
    globalMean[j] /= total;

  // 2. Вычисляем внутриклассовую матрицу рассеяния Sw и межклассовую Sb
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

  // 3. Вычисляем общую ковариационную матрицу (предполагаем одинаковую для всех
  // классов) Для LDA используем Sw, делённую на (total - num_classes)
  for (int j = 0; j < num_features_; ++j)
    for (int k = 0; k < num_features_; ++k)
      Sw[j][k] /= (total - num_classes_);

  // 4. Обращаем её (методом Гаусса-Жордана через наши утилиты)
  try {
    invCov_ = MatrixUtils::inverse(Sw);
  } catch (const std::exception &e) {
    // Если матрица вырождена, используем диагональный сдвиг (регуляризация)
    for (int j = 0; j < num_features_; ++j)
      Sw[j][j] += 1e-6;
    invCov_ = MatrixUtils::inverse(Sw);
  }

  // 5. Априорные вероятности классов
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
      // Вычисляем квадратичную форму (x - mean)^T * invCov * (x - mean)
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