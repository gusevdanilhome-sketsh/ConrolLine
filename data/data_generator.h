#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "models/line_calculator.h"
#include <complex>
#include <vector>


class DataGenerator {
public:
  DataGenerator(const LineCalculator &calc);
  void setNoiseStd(double sigma);
  void generate(int samplesPerClass, const std::vector<double> &freqs,
                std::vector<std::vector<double>> &features,
                std::vector<int> &labels);

private:
  LineCalculator calculator_;
  double noise_std_;
  std::complex<double> defectImpedance(int cls, double mag, double f) const;
};

#endif