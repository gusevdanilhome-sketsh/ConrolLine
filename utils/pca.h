#ifndef PCA_H
#define PCA_H

#include <vector>

class PCA {
public:
  void fit(const std::vector<std::vector<double>> &data);
  std::vector<std::vector<double>>
  transform(const std::vector<std::vector<double>> &data, int nComponents);

private:
  std::vector<std::vector<double>> components_;
  std::vector<double> mean_;
};

#endif