#ifndef LDA_H
#define LDA_H

#include "classifier_base.h"

class LDA : public ClassifierBase {
public:
  void train(const std::vector<std::vector<double>> &X,
             const std::vector<int> &y) override;
  std::vector<int>
  predict(const std::vector<std::vector<double>> &X) const override;
};

#endif