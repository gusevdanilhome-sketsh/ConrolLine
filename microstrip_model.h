#ifndef MICROSTRIP_MODEL_H
#define MICROSTRIP_MODEL_H

#include <complex>
#include <vector>

namespace MicrowaveNDT {

class MicrostripLineModel {
public:
  MicrostripLineModel();
  void setParams(double w_m, double t_m, double h_m, double er, double tand,
                 double sigma);
  double calcZ0(double f_hz) const;
  double calcEffPermittivity(double f_hz) const;
  double calcAttenuation(double f_hz) const;
  std::complex<double> propConst(double f_hz) const;
  std::complex<double> voltageAt(double x_m, double f_hz,
                                 const std::complex<double> &Uinc,
                                 const std::complex<double> &GammaRef,
                                 double x_ref_m) const;
  // Доступ к номиналам
  double width() const { return width_m_; }
  double thickness() const { return thickness_m_; }
  double height() const { return substrate_height_m_; }
  double epsilonR() const { return er_; }

private:
  double width_m_, thickness_m_, substrate_height_m_, er_, tan_delta_, sigma_;
  double skinResistance(double f_hz) const;
  double effectiveWidth() const;
};

} // namespace MicrowaveNDT
#endif