#ifndef LDA_H
#define LDA_H

#include "classifier_base.h"
#include <vector>

class LDA : public ClassifierBase {
public:
  void train(const std::vector<std::vector<double>> &X,
             const std::vector<int> &y) override;
  std::vector<int>
  predict(const std::vector<std::vector<double>> &X) const override;

private:
  std::vector<std::vector<double>> means_; // num_classes x num_features
  std::vector<double> priors_;
  std::vector<std::vector<double>>
      invCov_; // общая обратная ковариационная матрица
  int num_classes_ = 0;
  int num_features_ = 0;
  bool trained_ = false;
};

#endif