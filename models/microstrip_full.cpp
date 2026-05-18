#include "microstrip_full.h"
#include <cmath>

static constexpr double C0 = 299792458.0;

std::complex<double>
MicrostripFullModel::propagationConstant(double f_hz) const {
  double beta = 2.0 * M_PI * f_hz * std::sqrt(calcEffPermittivity(f_hz)) / C0;
  double alpha = calcAttenuation(f_hz);
  return std::complex<double>(alpha, beta);
}

std::complex<double> MicrostripFullModel::voltageAt(
    double x_m, double f_hz, const std::complex<double> &Uinc,
    const std::complex<double> &GammaRef, double x_ref_m) const {
  auto gamma = propagationConstant(f_hz);
  std::complex<double> phaseInc = std::exp(-gamma * (x_m - x_ref_m));
  std::complex<double> phaseRef = std::exp(gamma * (x_m - x_ref_m));
  return Uinc * (phaseInc + GammaRef * phaseRef);
}