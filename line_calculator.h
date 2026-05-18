#ifndef LINE_CALCULATOR_H
#define LINE_CALCULATOR_H

#include <QString>

class QTextBrowser;

class LineCalculator {
public:
  LineCalculator();

  void setWidth(double w_m);
  void setThickness(double t_m);
  void setSubstrateHeight(double h_m);
  void setPermittivity(double er);
  void setLossTangent(double tand);
  void setConductivity(double sigma);

  double calculateImpedance(double freq_hz) const;
  double calculateEffectivePermittivity(double freq_hz) const;
  double calculateAttenuation(double freq_hz) const; // Нп/м

  void updateOutputs(QTextBrowser *browser, double freq_hz = 1.0e9) const;

private:
  double width_m_;
  double thickness_m_;
  double substrate_height_m_;
  double er_;
  double tan_delta_;
  double sigma_;

  double skinResistance(double freq_hz) const;
  double effectiveWidth() const;
};

#endif // LINE_CALCULATOR_H