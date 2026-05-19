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

  // Геттеры для доступа к защищённым членам
  double getWidth() const { return width_m_; }
  double getThickness() const { return thickness_m_; }
  double getHeight() const { return height_m_; }
  double getEr() const { return er_; }
  double getTand() const { return tand_; }
  double getSigma() const { return sigma_; }

protected:
  double width_m_, thickness_m_, height_m_, er_, tand_, sigma_;
};

#endif