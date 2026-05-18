#ifndef MICROSTRIP_FULL_H
#define MICROSTRIP_FULL_H

#include "line_calculator.h"
#include <complex>

class MicrostripFullModel : public LineCalculator {
public:
  using LineCalculator::LineCalculator;
  std::complex<double> propagationConstant(double f_hz) const;
  std::complex<double> voltageAt(double x_m, double f_hz,
                                 const std::complex<double> &Uinc,
                                 const std::complex<double> &GammaRef,
                                 double x_ref_m) const;
};

#endif