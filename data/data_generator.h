#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "models/line_calculator.h"
#include "models/microstrip_full.h"
#include <complex>
#include <vector>

class DataGenerator {
public:
  explicit DataGenerator(const LineCalculator &calc);
  void setNoiseStd(double sigma);
  void setQuantization(int bits, bool rawMode);
  void setTolerances(double w_tol, double t_tol, double h_tol);
  void setVariationEnabled(bool enable);
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
  MicrostripFullModel fullModel_;
  double noise_std_ = 0.05;
  int adcBits_ = 12;
  bool rawAdcMode_ = false;
  double w_tol_ = 0.0, t_tol_ = 0.0, h_tol_ = 0.0;
  bool varyLine_ = false;

  // Объявления вспомогательных методов
  std::complex<double> defectImpedance(int cls, double param1, double param2,
                                       double f) const;
  std::complex<double> defectImpedanceEx(int cls, double param1, double param2,
                                         double f) const;
  void computeElectrodeVoltages(double f, double Z0,
                                const std::complex<double> &Gamma, double a,
                                std::complex<double> U[4]) const;
  std::vector<double> quantize(const std::vector<double> &features) const;
};

#endif