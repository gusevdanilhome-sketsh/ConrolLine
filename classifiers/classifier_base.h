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
};

#endif