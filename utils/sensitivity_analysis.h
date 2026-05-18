#ifndef SENSITIVITY_ANALYSIS_H
#define SENSITIVITY_ANALYSIS_H

struct SensitivityResult {
  double dZdW;
  double dZdh;
  double dZder;
};

class SensitivityAnalysis {
public:
  static SensitivityResult computeSensitivities(double W, double t, double h,
                                                double er);
};

#endif