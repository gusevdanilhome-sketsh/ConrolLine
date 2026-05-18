// data_generator.h
#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "microstrip_model.h"
#include <Eigen/Dense>
#include <vector>

namespace MicrowaveNDT {

class DataGenerator {
public:
  DataGenerator(const MicrostripLineModel &model);
  void setNoiseStd(double sigma) { noise_std_ = sigma; }
  // Генерация выборки: строки = образцы, столбцы = признаки (6*M), labels -
  // вектор классов
  void generateDataset(int samplesPerClass, const std::vector<double> &freqs_hz,
                       Eigen::MatrixXd &features, Eigen::VectorXi &labels);

private:
  MicrostripLineModel model_;
  double noise_std_ = 0.05;
  std::complex<double> defectImpedance(int defectClass, double magnitudeNorm,
                                       double freq_hz) const;
  std::complex<double> reflectionCoeff(double Zdef, double Z0) const;
};

} // namespace MicrowaveNDT
#endif