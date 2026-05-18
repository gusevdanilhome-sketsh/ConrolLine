#ifndef NAIVE_BAYES_H
#define NAIVE_BAYES_H

#include "classifier_base.h"
#include <vector>

class NaiveBayes : public ClassifierBase {
public:
  void train(const std::vector<std::vector<double>> &X,
             const std::vector<int> &y) override;
  std::vector<int>
  predict(const std::vector<std::vector<double>> &X) const override;

private:
  std::vector<std::vector<double>> means_;
  std::vector<std::vector<double>> variances_;
  std::vector<double> priors_;
  int num_classes_ = 0;
  int num_features_ = 0;
  bool trained_ = false;
};

#endif