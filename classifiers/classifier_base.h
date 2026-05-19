#ifndef CLASSIFIER_BASE_H
#define CLASSIFIER_BASE_H

#include <vector>

class ClassifierBase {
public:
  virtual ~ClassifierBase() = default;
  virtual void train(const std::vector<std::vector<double>> &X,
                     const std::vector<int> &y) = 0;
  virtual std::vector<int>
  predict(const std::vector<std::vector<double>> &X) const = 0;
  virtual std::vector<int>
  predictWithLoss(const std::vector<std::vector<double>> &X,
                  const std::vector<std::vector<double>> &lossMatrix) const {
    // по умолчанию используем обычное предсказание
    return predict(X);
  }
};

#endif