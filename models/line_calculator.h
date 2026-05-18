#ifndef LINE_CALCULATOR_H
#define LINE_CALCULATOR_H

class LineCalculator {
public:
  LineCalculator();
  void setParams(double w, double t, double h, double er, double tand,
                 double sigma);
  double calcZ0(double freq_hz) const;
  double calcEffPermittivity(double freq_hz) const;
  double calcAttenuation(double freq_hz) const;

private:
  double width_m_, thickness_m_, height_m_, er_, tand_, sigma_;
};

#endif