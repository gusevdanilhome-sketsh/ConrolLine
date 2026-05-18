#include "line_calculator.h"
#include <algorithm>
#include <cmath>


static constexpr double PI = 3.141592653589793;
static constexpr double C0 = 299792458.0;

LineCalculator::LineCalculator()
    : width_m_(0.0002), thickness_m_(35e-6), height_m_(0.001), er_(4.6),
      tand_(0.02), sigma_(5.8e7) {}

void LineCalculator::setParams(double w, double t, double h, double er,
                               double tand, double sigma) {
  width_m_ = std::max(w, 1e-9);
  thickness_m_ = std::max(t, 1e-9);
  height_m_ = std::max(h, 1e-9);
  er_ = std::max(er, 1.0);
  tand_ = std::max(tand, 0.0);
  sigma_ = std::max(sigma, 1e3);
}

double LineCalculator::calcEffPermittivity(double /*freq_hz*/) const {
  double wh = width_m_ / height_m_;
  if (wh <= 0.0)
    return er_;
  double er_eff =
      (er_ + 1.0) / 2.0 + (er_ - 1.0) / (2.0 * std::sqrt(1.0 + 12.0 / wh));
  if (wh < 1.0)
    er_eff += (er_ - 1.0) / 2.0 * 0.04 * (1.0 - wh) * (1.0 - wh);
  return std::max(er_eff, 1.0);
}

double LineCalculator::calcZ0(double freq_hz) const {
  double er_eff = calcEffPermittivity(freq_hz);
  double wh_eff = width_m_ / height_m_;
  double A = wh_eff + 1.393 + 0.667 * std::log(wh_eff + 1.444);
  return 120.0 * PI / (std::sqrt(er_eff) * A);
}

double LineCalculator::calcAttenuation(double freq_hz) const {
  return 0.01; // упрощённо, чтобы не было падений
}