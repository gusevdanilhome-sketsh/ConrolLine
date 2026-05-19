#include "data_generator.h"
#include <algorithm>
#include <cmath>
#include <random>


static constexpr double PI = 3.141592653589793;

DataGenerator::DataGenerator(const LineCalculator &calc)
    : calculator_(calc), fullModel_(), noise_std_(0.05) {}

void DataGenerator::setQuantization(int bits, bool rawMode) {
  adcBits_ = bits;
  rawAdcMode_ = rawMode;
}

void DataGenerator::setTolerances(double w_tol, double t_tol, double h_tol) {
  w_tol_ = w_tol;
  t_tol_ = t_tol;
  h_tol_ = h_tol;
}

void DataGenerator::setVariationEnabled(bool enable) { varyLine_ = enable; }

void DataGenerator::setNoiseStd(double sigma) { noise_std_ = sigma; }

std::complex<double> DataGenerator::defectImpedance(int cls, double param1,
                                                    double param2,
                                                    double f) const {
  double omega = 2.0 * PI * f;
  switch (cls) {
  case 1:
    return {param2, omega * param1}; // Ld, Rd
  case 2:
    return {0.0, -1.0 / (omega * param1 + 1e-15)}; // Cd
  case 3:
    return {0.0, -1.0 / (omega * param1 + 1e-15)}; // Csh
  case 4:
    return {0.0, -1.0 / (omega * param1 + 1e-15)}; // Csh (for εr)
  default:
    return {calculator_.calcZ0(f), 0.0};
  }
}

std::complex<double> DataGenerator::defectImpedanceEx(int cls, double param1,
                                                      double param2,
                                                      double f) const {
  // Аналогично defectImpedance, но может иметь другую логику (оставим для
  // совместимости)
  return defectImpedance(cls, param1, param2, f);
}

void DataGenerator::computeElectrodeVoltages(double f, double Z0,
                                             const std::complex<double> &Gamma,
                                             double a,
                                             std::complex<double> U[4]) const {
  std::complex<double> Uinc(1.0, 0.0);
  double xd = 0.0;
  double x1 = -a / 2, x2 = a / 2, x3 = a / 2, x4 = -a / 2;
  U[0] = fullModel_.voltageAt(x1, f, Uinc, Gamma, xd);
  U[1] = fullModel_.voltageAt(x2, f, Uinc, Gamma, xd);
  U[2] = fullModel_.voltageAt(x3, f, Uinc, Gamma, xd);
  U[3] = fullModel_.voltageAt(x4, f, Uinc, Gamma, xd);
}

std::vector<double>
DataGenerator::quantize(const std::vector<double> &features) const {
  if (adcBits_ <= 0 || rawAdcMode_ == false)
    return features;
  double maxVal = *std::max_element(features.begin(), features.end());
  double minVal = *std::min_element(features.begin(), features.end());
  double range = maxVal - minVal;
  int levels = (1 << adcBits_) - 1;
  std::vector<double> quantized = features;
  for (double &v : quantized) {
    int code = static_cast<int>(round((v - minVal) / range * levels));
    v = (rawAdcMode_) ? code : (minVal + code * range / levels);
  }
  return quantized;
}

void DataGenerator::generate(int samplesPerClass,
                             const std::vector<double> &freqs,
                             std::vector<std::vector<double>> &features,
                             std::vector<int> &labels) {
  features.clear();
  labels.clear();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<> noise(0.0, noise_std_);
  std::uniform_real_distribution<> paramDist(0.0, 1.0);

  // Диапазоны параметров дефектов
  std::vector<std::pair<double, double>> paramRanges[5];
  paramRanges[1] = {{0.05e-9, 0.5e-9}, {0.01, 0.1}}; // Ld, Rd
  paramRanges[2] = {{0.01e-12, 0.2e-12}};            // Cd
  paramRanges[3] = {{0.02e-12, 0.3e-12}};            // Csh
  paramRanges[4] = {{0.01e-12, 0.25e-12}};           // Csh for εr

  double a = 0.001; // сторона квадрата 1 мм

  for (int cls = 0; cls < 5; ++cls) {
    for (int s = 0; s < samplesPerClass; ++s) {
      // Генерация параметров линии с учётом допусков
      double w = calculator_.getWidth();
      double t = calculator_.getThickness();
      double h = calculator_.getHeight();
      double er = calculator_.getEr();
      if (varyLine_) {
        w += (paramDist(gen) * 2 - 1) * w_tol_;
        t += (paramDist(gen) * 2 - 1) * t_tol_;
        h += (paramDist(gen) * 2 - 1) * h_tol_;
        er += (paramDist(gen) * 2 - 1) * 0.1; // допуск на εr ±0.1
        fullModel_.setParams(w, t, h, er, calculator_.getTand(),
                             calculator_.getSigma());
      } else {
        fullModel_.setParams(w, t, h, er, calculator_.getTand(),
                             calculator_.getSigma());
      }
      // Параметры дефекта
      double param1 = 0.0, param2 = 0.0;
      if (cls == 1) {
        param1 = paramDist(gen) *
                     (paramRanges[1][0].second - paramRanges[1][0].first) +
                 paramRanges[1][0].first;
        param2 = paramDist(gen) *
                     (paramRanges[1][1].second - paramRanges[1][1].first) +
                 paramRanges[1][1].first;
      } else if (cls >= 2) {
        param1 = paramDist(gen) *
                     (paramRanges[cls][0].second - paramRanges[cls][0].first) +
                 paramRanges[cls][0].first;
      }
      std::vector<double> feat;
      for (double f : freqs) {
        double Z0 = fullModel_.calcZ0(f);
        std::complex<double> Zdef =
            (cls == 0) ? Z0 : defectImpedance(cls, param1, param2, f);
        std::complex<double> Gamma =
            (cls == 0) ? 0.0 : (Zdef - Z0) / (Zdef + Z0);
        std::complex<double> U[4];
        computeElectrodeVoltages(f, Z0, Gamma, a, U);
        std::complex<double> S = (U[0] + U[1] + U[2] + U[3]) / 4.0;
        std::complex<double> Dx = ((U[0] + U[1]) - (U[2] + U[3])) / 2.0;
        std::complex<double> Dy = ((U[0] - U[1]) - (U[2] - U[3])) / 2.0;
        feat.push_back(S.real());
        feat.push_back(S.imag());
        feat.push_back(Dx.real());
        feat.push_back(Dx.imag());
        feat.push_back(Dy.real());
        feat.push_back(Dy.imag());
      }
      // Добавление шума
      for (double &val : feat)
        val += noise(gen);
      // Квантование
      if (adcBits_ > 0 && rawAdcMode_) {
        feat = quantize(feat);
      }
      features.push_back(feat);
      labels.push_back(cls);
    }
  }
}

std::vector<double> DataGenerator::computeFeaturesForParams(
    int cls, double mag, const std::vector<double> &freqs) const {
  std::vector<double> feat;
  for (double f : freqs) {
    double Z0 = calculator_.calcZ0(f);
    // Для computeFeaturesForParams используем mag как параметр param1, param2 =
    // 0
    std::complex<double> Zdef =
        (cls == 0) ? Z0 : defectImpedance(cls, mag, 0.0, f);
    std::complex<double> Gamma = (cls == 0) ? 0.0 : (Zdef - Z0) / (Zdef + Z0);
    std::complex<double> U[4];
    double a = 0.001;
    computeElectrodeVoltages(f, Z0, Gamma, a, U);
    std::complex<double> S = (U[0] + U[1] + U[2] + U[3]) / 4.0;
    std::complex<double> Dx = ((U[0] + U[1]) - (U[2] + U[3])) / 2.0;
    std::complex<double> Dy = ((U[0] - U[1]) - (U[2] - U[3])) / 2.0;
    feat.push_back(S.real());
    feat.push_back(S.imag());
    feat.push_back(Dx.real());
    feat.push_back(Dx.imag());
    feat.push_back(Dy.real());
    feat.push_back(Dy.imag());
  }
  return feat;
}

std::vector<double> DataGenerator::computeFeaturesForDefect(
    int cls, double param1, double param2,
    const std::vector<double> &freqs) const {
  std::vector<double> feat;
  for (double f : freqs) {
    double Z0 = calculator_.calcZ0(f);
    std::complex<double> Zdef =
        (cls == 0) ? Z0 : defectImpedance(cls, param1, param2, f);
    std::complex<double> Gamma = (cls == 0) ? 0.0 : (Zdef - Z0) / (Zdef + Z0);
    std::complex<double> U[4];
    double a = 0.001;
    computeElectrodeVoltages(f, Z0, Gamma, a, U);
    std::complex<double> S = (U[0] + U[1] + U[2] + U[3]) / 4.0;
    std::complex<double> Dx = ((U[0] + U[1]) - (U[2] + U[3])) / 2.0;
    std::complex<double> Dy = ((U[0] - U[1]) - (U[2] - U[3])) / 2.0;
    feat.push_back(S.real());
    feat.push_back(S.imag());
    feat.push_back(Dx.real());
    feat.push_back(Dx.imag());
    feat.push_back(Dy.real());
    feat.push_back(Dy.imag());
  }
  return feat;
}