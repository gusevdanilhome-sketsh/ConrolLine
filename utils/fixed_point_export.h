#ifndef FIXED_POINT_EXPORT_H
#define FIXED_POINT_EXPORT_H

#include <string>
#include <vector>

class FixedPointExport {
public:
  static bool
  exportLogisticRegressionQ15(const std::vector<std::vector<double>> &weights,
                              const std::vector<double> &biases,
                              const std::string &headerFile);
};

#endif