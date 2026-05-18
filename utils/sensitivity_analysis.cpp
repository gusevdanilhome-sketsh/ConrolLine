#include "sensitivity_analysis.h"
#include "models/line_calculator.h"
#include <cmath>

SensitivityResult SensitivityAnalysis::computeSensitivities(double W, double t,
                                                            double h,
                                                            double er) {
  const double delta = 1e-6;
  LineCalculator calc;
  SensitivityResult res;
  double Z0_nom;
  calc.setParams(W, t, h, er, 0.02, 5.8e7);
  Z0_nom = calc.calcZ0(1e9);

  // dZ/dW
  calc.setParams(W + delta, t, h, er, 0.02, 5.8e7);
  double Z0_plus = calc.calcZ0(1e9);
  res.dZdW = (Z0_plus - Z0_nom) / delta;

  // dZ/dh
  calc.setParams(W, t, h + delta, er, 0.02, 5.8e7);
  Z0_plus = calc.calcZ0(1e9);
  res.dZdh = (Z0_plus - Z0_nom) / delta;

  // dZ/der
  calc.setParams(W, t, h, er + delta, 0.02, 5.8e7);
  Z0_plus = calc.calcZ0(1e9);
  res.dZder = (Z0_plus - Z0_nom) / delta;

  return res;
}