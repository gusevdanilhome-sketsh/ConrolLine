#ifndef NAIVE_BAYES_H
#define NAIVE_BAYES_H

#include "classifier_base.h"

class NaiveBayes : public ClassifierBase {
public:
  void train(const std::vector<std::vector<double>> &X,
             const std::vector<int> &y) override;
  std::vector<int>
  predict(const std::vector<std::vector<double>> &X) const override;
};

#endif