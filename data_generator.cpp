// data_generator.cpp
#include "data_generator.h"
#include <cmath>
#include <random>

namespace MicrowaveNDT {

DataGenerator::DataGenerator(const MicrostripLineModel &model)
    : model_(model) {}

std::complex<double> DataGenerator::defectImpedance(int cls, double magNorm,
                                                    double f_hz) const {
  double omega = 2.0 * M_PI * f_hz;
  switch (cls) {
  case 1: // индуктивный
    return std::complex<double>(0.01 * magNorm, omega * 0.2e-9 * magNorm);
  case 2: // ёмкостной последовательный
    return std::complex<double>(0.0, -1.0 / (omega * 0.1e-12 * magNorm));
  case 3: // шунтирующая ёмкость
    return std::complex<double>(0.0, -1.0 / (omega * 0.15e-12 * magNorm));
  case 4: // изменение диэлектрика
    return std::complex<double>(0.0, -1.0 / (omega * 0.12e-12 * magNorm));
  default:
    return std::complex<double>(model_.calcZ0(f_hz), 0.0);
  }
}

std::complex<double> DataGenerator::reflectionCoeff(double Zdef,
                                                    double Z0) const {
  return (Zdef - Z0) / (Zdef + Z0);
}

void DataGenerator::generateDataset(int samplesPerClass,
                                    const std::vector<double> &freqs_hz,
                                    Eigen::MatrixXd &features,
                                    Eigen::VectorXi &labels) {
  int M = (int)freqs_hz.size();
  int Nfeat = 6 * M; // 3 канала * I/Q
  int totalSamples = samplesPerClass * 5;
  features.resize(totalSamples, Nfeat);
  labels.resize(totalSamples);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<> noise(0.0, noise_std_);
  int row = 0;
  double a = 0.001; // сторона квадрата головки 1 мм
  for (int cls = 0; cls < 5; ++cls) {
    for (int s = 0; s < samplesPerClass; ++s) {
      double magNorm =
          (cls == 0) ? 0.0 : (double)s / samplesPerClass; // от 0 до 1
      Eigen::VectorXd feat(Nfeat);
      int featIdx = 0;
      for (double f : freqs_hz) {
        double Z0 = model_.calcZ0(f);
        std::complex<double> Zdef = defectImpedance(cls, magNorm, f);
        std::complex<double> Gamma =
            (cls == 0) ? 0.0 : reflectionCoeff(std::abs(Zdef), Z0);
        // Координаты электродов (центр головки в x=0, дефект в x=0)
        std::complex<double> Uinc(1.0, 0.0);
        double xd = 0.0;
        std::complex<double> U1 = model_.voltageAt(-a / 2, f, Uinc, Gamma, xd);
        std::complex<double> U2 = model_.voltageAt(a / 2, f, Uinc, Gamma, xd);
        std::complex<double> U3 = U2;
        std::complex<double> U4 = U1;
        // Суммарный и разностные каналы (матрица M)
        std::complex<double> S = U1 + U2 + U3 + U4;
        std::complex<double> Dx = (U1 + U2) - (U3 + U4);
        std::complex<double> Dy = (U1 - U2) - (U3 - U4);
        // Добавление шума
        S += std::complex<double>(noise(gen), noise(gen));
        Dx += std::complex<double>(noise(gen), noise(gen));
        Dy += std::complex<double>(noise(gen), noise(gen));
        feat[featIdx++] = S.real();
        feat[featIdx++] = S.imag();
        feat[featIdx++] = Dx.real();
        feat[featIdx++] = Dx.imag();
        feat[featIdx++] = Dy.real();
        feat[featIdx++] = Dy.imag();
      }
      features.row(row) = feat;
      labels(row) = cls;
      ++row;
    }
  }
}
} // namespace MicrowaveNDT