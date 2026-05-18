#include "data_generator.h"
#include <cmath>
#include <random>


DataGenerator::DataGenerator(const LineCalculator &calc)
    : calculator_(calc), noise_std_(0.05) {}

void DataGenerator::setNoiseStd(double sigma) { noise_std_ = sigma; }

std::complex<double> DataGenerator::defectImpedance(int cls, double mag,
                                                    double f) const {
  double omega = 2.0 * M_PI * f;
  switch (cls) {
  case 1:
    return {0.01 * mag, omega * 0.2e-9 * mag};
  case 2:
    return {0.0, -1.0 / (omega * 0.1e-12 * mag + 1e-15)};
  case 3:
    return {0.0, -1.0 / (omega * 0.15e-12 * mag + 1e-15)};
  case 4:
    return {0.0, -1.0 / (omega * 0.12e-12 * mag + 1e-15)};
  default:
    return {calculator_.calcZ0(f), 0.0};
  }
}

void DataGenerator::generate(int samplesPerClass,
                             const std::vector<double> &freqs,
                             std::vector<std::vector<double>> &features,
                             std::vector<int> &labels) {
  features.clear();
  labels.clear();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<> noise(0.0, noise_std_);

  for (int cls = 0; cls < 5; ++cls) {
    for (int s = 0; s < samplesPerClass; ++s) {
      double magNorm =
          (cls == 0) ? 0.0 : static_cast<double>(s) / samplesPerClass;
      std::vector<double> feat;
      for (double f : freqs) {
        double Z0 = calculator_.calcZ0(f);
        std::complex<double> Zdef = defectImpedance(cls, magNorm, f);
        std::complex<double> Gamma =
            (cls == 0) ? 0.0 : (Zdef - Z0) / (Zdef + Z0);
        std::complex<double> S(1.0 + Gamma.real(), Gamma.imag());
        std::complex<double> Dx(0.5 * Gamma.real(), 0.5 * Gamma.imag());
        std::complex<double> Dy(0.3 * Gamma.real(), 0.3 * Gamma.imag());
        S += std::complex<double>(noise(gen), noise(gen));
        Dx += std::complex<double>(noise(gen), noise(gen));
        Dy += std::complex<double>(noise(gen), noise(gen));
        feat.push_back(S.real());
        feat.push_back(S.imag());
        feat.push_back(Dx.real());
        feat.push_back(Dx.imag());
        feat.push_back(Dy.real());
        feat.push_back(Dy.imag());
      }
      features.push_back(feat);
      labels.push_back(cls);
    }
  }
}