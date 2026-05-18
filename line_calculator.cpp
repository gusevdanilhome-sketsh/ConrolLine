#include "line_calculator.h"
#include <QTextBrowser>
#include <cmath>
#include <iomanip>
#include <sstream>

static constexpr double PI = 3.14159265358979323846;
static constexpr double C0 = 299792458.0;
static constexpr double MU0 = 4.0e-7 * PI;

// Вспомогательная функция проверки геометрических параметров
static bool isValidGeometry(double width, double thickness, double height) {
  return (width > 0.0) && (thickness > 0.0) && (height > 0.0);
}

LineCalculator::LineCalculator()
    : width_m_(0.0002), thickness_m_(35e-6), substrate_height_m_(0.001),
      er_(4.6), tan_delta_(0.02), sigma_(5.8e7) {}

void LineCalculator::setWidth(double w_m) { width_m_ = w_m; }
void LineCalculator::setThickness(double t_m) { thickness_m_ = t_m; }
void LineCalculator::setSubstrateHeight(double h_m) {
  substrate_height_m_ = h_m;
}
void LineCalculator::setPermittivity(double er) { er_ = er; }
void LineCalculator::setLossTangent(double tand) { tan_delta_ = tand; }
void LineCalculator::setConductivity(double sigma) { sigma_ = sigma; }

double LineCalculator::calculateImpedance(double freq_hz) const {
  if (!isValidGeometry(width_m_, thickness_m_, substrate_height_m_) ||
      er_ < 1.0) {
    return 0.0;
  }
  double er_eff = calculateEffectivePermittivity(freq_hz);
  double wh_eff = effectiveWidth() / substrate_height_m_;
  double A = wh_eff + 1.393 + 0.667 * std::log(wh_eff + 1.444);
  return 120.0 * PI / (std::sqrt(er_eff) * A);
}

double LineCalculator::calculateEffectivePermittivity(double freq_hz) const {
  if (substrate_height_m_ <= 0.0)
    return er_;
  double wh = width_m_ / substrate_height_m_;
  if (wh <= 0.0)
    return er_;
  double er_eff0 =
      (er_ + 1.0) / 2.0 + (er_ - 1.0) / (2.0 * std::sqrt(1.0 + 12.0 / wh));
  if (wh < 1.0) {
    er_eff0 = (er_ + 1.0) / 2.0 + (er_ - 1.0) / 2.0 *
                                      (1.0 / std::sqrt(1.0 + 12.0 / wh) +
                                       0.04 * (1.0 - wh) * (1.0 - wh));
  }
  double Z0_0 = calculateImpedance(0.0);
  double fp = Z0_0 / (2.0 * MU0 * substrate_height_m_);
  double m = 1.8;
  double er_eff = er_ - (er_ - er_eff0) / (1.0 + std::pow(freq_hz / fp, m));
  return er_eff;
}

double LineCalculator::calculateAttenuation(double freq_hz) const {
  if (!isValidGeometry(width_m_, thickness_m_, substrate_height_m_)) {
    return 0.0;
  }
  double Rs = skinResistance(freq_hz);
  double Z0 = calculateImpedance(freq_hz);
  if (Z0 <= 0.0)
    return 0.0;
  double alpha_c = Rs / (width_m_ * Z0);
  double er_eff = calculateEffectivePermittivity(freq_hz);
  double lambda0 = C0 / freq_hz;
  double alpha_d_db = 27.3 * er_ / std::sqrt(er_eff) * (er_eff - 1.0) /
                      (er_ - 1.0) * tan_delta_ / lambda0;
  double alpha_d_np = alpha_d_db / 8.686;
  return alpha_c + alpha_d_np;
}

double LineCalculator::skinResistance(double freq_hz) const {
  double omega = 2.0 * PI * freq_hz;
  return std::sqrt(omega * MU0 / (2.0 * sigma_));
}

double LineCalculator::effectiveWidth() const {
  double weff = width_m_;
  if ((thickness_m_ > 0.0) && (substrate_height_m_ > 0.0) && (width_m_ > 0.0)) {
    double dt =
        thickness_m_ / (2.0 * PI * width_m_) *
        std::log(1.0 + 4.0 * std::exp(1.0) /
                           std::pow(thickness_m_ / substrate_height_m_, 2.0));
    weff = width_m_ + dt;
  }
  return weff;
}

void LineCalculator::updateOutputs(QTextBrowser *browser,
                                   double freq_hz) const {
  if (!browser)
    return;

  if (!isValidGeometry(width_m_, thickness_m_, substrate_height_m_) ||
      er_ < 1.0 || sigma_ <= 0.0) {
    browser->setText(
        "Ошибка: недопустимые геометрические или электрофизические параметры "
        "линии (размеры должны быть > 0, εr ≥ 1).");
    return;
  }

  double Z0 = calculateImpedance(freq_hz);
  double er_eff = calculateEffectivePermittivity(freq_hz);
  double alpha = calculateAttenuation(freq_hz);
  double lambda_g = C0 / (freq_hz * std::sqrt(er_eff));
  std::stringstream ss;
  ss << std::fixed << std::setprecision(4);
  ss << "Параметры линии при f = " << (freq_hz / 1e9) << " ГГц:\n";
  ss << "Волновое сопротивление Z0 = " << Z0 << " Ом\n";
  ss << "Эффективная диэлектрическая проницаемость ε_eff = " << er_eff << "\n";
  ss << "Погонное затухание α = " << alpha * 100.0 << " Нп/м (x100)\n";
  ss << "Длина волны в линии λ_g = " << lambda_g * 1000.0 << " мм";
  browser->setText(QString::fromStdString(ss.str()));
}