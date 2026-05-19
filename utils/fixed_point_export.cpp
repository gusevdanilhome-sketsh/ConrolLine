#include "fixed_point_export.h"
#include <cmath>
#include <cstdint> // для int16_t
#include <fstream>
#include <iomanip>
#include <iostream>

static int16_t doubleToQ15(double val) {
  // Q15: диапазон [-1, 1) соответствует -32768 ... 32767
  double clamped = std::max(-1.0, std::min(0.9999999, val));
  return static_cast<int16_t>(std::round(clamped * 32768.0));
}

bool FixedPointExport::exportLogisticRegressionQ15(
    const std::vector<std::vector<double>> &weights,
    const std::vector<double> &biases, const std::string &headerFile) {
  std::ofstream out(headerFile);
  if (!out)
    return false;

  int numClasses = weights.size();
  int numFeatures = weights[0].size();

  // Определяем масштабирующий коэффициент для весов (максимальное абсолютное
  // значение)
  double maxAbsWeight = 0.0;
  for (const auto &row : weights)
    for (double w : row)
      maxAbsWeight = std::max(maxAbsWeight, std::abs(w));
  // Нормализуем веса так, чтобы максимальный был <= 0.99, оставляя запас для
  // суммирования
  double scale = 0.9 / maxAbsWeight;

  out << "#ifndef CONROLLINE_WEIGHTS_Q15_H\n";
  out << "#define CONROLLINE_WEIGHTS_Q15_H\n\n";
  out << "#include <stdint.h>\n\n";

  out << "// Масштабирующий коэффициент (применять к выходу перед softmax)\n";
  out << "#define WEIGHT_SCALE_FACTOR " << std::setprecision(10) << scale
      << "f\n\n";

  out << "// Количество классов\n";
  out << "#define NUM_CLASSES " << numClasses << "\n\n";
  out << "// Количество признаков\n";
  out << "#define NUM_FEATURES " << numFeatures << "\n\n";

  out << "// Веса (Q15)\n";
  out << "static const int16_t weights_q15[NUM_CLASSES][NUM_FEATURES] = {\n";
  for (int c = 0; c < numClasses; ++c) {
    out << "    { ";
    for (int j = 0; j < numFeatures; ++j) {
      double scaled = weights[c][j] * scale;
      out << doubleToQ15(scaled);
      if (j != numFeatures - 1)
        out << ", ";
    }
    out << " }";
    if (c != numClasses - 1)
      out << ",";
    out << "\n";
  }
  out << "};\n\n";

  out << "// Смещения (Q15)\n";
  out << "static const int16_t biases_q15[NUM_CLASSES] = { ";
  for (int c = 0; c < numClasses; ++c) {
    double scaledBias = biases[c] * scale;
    out << doubleToQ15(scaledBias);
    if (c != numClasses - 1)
      out << ", ";
  }
  out << " };\n\n";

  out << "#endif // CONROLLINE_WEIGHTS_Q15_H\n";
  return true;
}