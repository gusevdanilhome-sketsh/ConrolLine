#ifndef WHITENING_H
#define WHITENING_H

#include <vector>

class Whitening {
public:
  Whitening() : fitted_(false) {}

  void fit(const std::vector<std::vector<double>> &X);
  std::vector<std::vector<double>>
  transform(const std::vector<std::vector<double>> &X) const;
  std::vector<std::vector<double>>
  inverseTransform(const std::vector<std::vector<double>> &X) const;

  bool isFitted() const { return fitted_; }

private:
  std::vector<double> mean_;
  std::vector<std::vector<double>> L_;
  int nFeatures_ = 0;
  bool fitted_ = false;
};

#endif