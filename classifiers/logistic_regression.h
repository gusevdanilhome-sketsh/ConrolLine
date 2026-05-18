#ifndef LOGISTIC_REGRESSION_H
#define LOGISTIC_REGRESSION_H

#include "classifier_base.h"
#include <vector>

class LogisticRegression : public ClassifierBase {
public:
  void train(const std::vector<std::vector<double>> &X,
             const std::vector<int> &y) override;
  std::vector<int>
  predict(const std::vector<std::vector<double>> &X) const override;

private:
  std::vector<std::vector<double>> weights_;
  std::vector<double> bias_;
  int num_classes_ = 0;
  int num_features_ = 0;
};

#endif