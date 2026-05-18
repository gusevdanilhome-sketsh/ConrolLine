#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "models/line_calculator.h"
#include <complex>
#include <vector>

class DataGenerator {
public:
  explicit DataGenerator(const LineCalculator &calc);
  void setNoiseStd(double sigma);
  void generate(int samplesPerClass, const std::vector<double> &freqs,
                std::vector<std::vector<double>> &features,
                std::vector<int> &labels);
  std::vector<double>
  computeFeaturesForParams(int cls, double mag,
                           const std::vector<double> &freqs) const;
  std::vector<double>
  computeFeaturesForDefect(int cls, double param1, double param2,
                           const std::vector<double> &freqs) const;

private:
  LineCalculator calculator_;
  double noise_std_;
  std::complex<double> defectImpedance(int cls, double mag, double f) const;
  std::complex<double> defectImpedanceEx(int cls, double param1, double param2,
                                         double f) const;
};

#endif