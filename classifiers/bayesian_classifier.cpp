#include "bayesian_classifier.h"
#include "models/microstrip_full.h"
#include "data/data_generator.h"
#include <cmath>
#include <algorithm>
#include <numeric>

BayesianClassifier::BayesianClassifier() {}

void BayesianClassifier::setNoiseStd(double sigma) {
  noiseStd_ = sigma;
}

void BayesianClassifier::setParameterGrid() {
  buildClassModels();
}

void BayesianClassifier::buildClassModels() {
  // Для каждого класса строим сетку параметров и соответствующие векторы признаков
  // Используем DataGenerator для вычисления f(y,θ)
  LineCalculator calc;
  MicrostripFullModel fullModel;
  DataGenerator gen(calc);
  std::vector<double> freqs;
  for (int f = 1; f <= 10; ++f) freqs.push_back(f * 1e9);
  
  classModels_.resize(5);
  
  // Класс 0: без дефекта (один вектор)
  ClassModel &c0 = classModels_[0];
  c0.featureVectors.push_back(gen.computeFeaturesForParams(0, 0.0, freqs));
  c0.weights.push_back(1.0);
  
  // Класс 1: индуктивный дефект (Ld, Rd)
  std::vector<double> Ld_vals = {0.05e-9, 0.1e-9, 0.2e-9, 0.3e-9, 0.5e-9};
  std::vector<double> Rd_vals = {0.01, 0.03, 0.05, 0.07, 0.1};
  double total = Ld_vals.size() * Rd_vals.size();
  for (double Ld : Ld_vals) {
    for (double Rd : Rd_vals) {
      c0.featureVectors.push_back(gen.computeFeaturesForDefect(1, Ld, Rd, freqs));
      c0.weights.push_back(1.0 / total);
    }
  }
  
  // Класс 2: последовательная ёмкость
  std::vector<double> Cd_vals = {0.01e-12, 0.05e-12, 0.1e-12, 0.15e-12, 0.2e-12};
  total = Cd_vals.size();
  for (double Cd : Cd_vals) {
    c0.featureVectors.push_back(gen.computeFeaturesForDefect(2, Cd, 0.0, freqs));
    c0.weights.push_back(1.0 / total);
  }
  
  // Класс 3: шунтирующая ёмкость
  std::vector<double> Csh_vals = {0.02e-12, 0.05e-12, 0.1e-12, 0.2e-12, 0.3e-12};
  total = Csh_vals.size();
  for (double Csh : Csh_vals) {
    c0.featureVectors.push_back(gen.computeFeaturesForDefect(3, Csh, 0.0, freqs));
    c0.weights.push_back(1.0 / total);
  }
  
  // Класс 4: изменение диэлектрической проницаемости (шунтирующая ёмкость с другим диапазоном)
  std::vector<double> Csh2_vals = {0.01e-12, 0.05e-12, 0.1e-12, 0.15e-12, 0.25e-12};
  total = Csh2_vals.size();
  for (double Csh : Csh2_vals) {
    c0.featureVectors.push_back(gen.computeFeaturesForDefect(4, Csh, 0.0, freqs));
    c0.weights.push_back(1.0 / total);
  }
  
  numFeatures_ = (int)c0.featureVectors[0].size();
}

void BayesianClassifier::train(const std::vector<std::vector<double>> &X,
                               const std::vector<int> &y) {
  // Для байесовского классификатора обучение сводится к оценке априорных вероятностей
  priors_.assign(numClasses_, 0.0);
  for (int label : y) priors_[label]++;
  int total = (int)y.size();
  for (int c = 0; c < numClasses_; ++c) priors_[c] /= total;
  trained_ = true;
}

double BayesianClassifier::computeLogLikelihood(const std::vector<double>& x,
                                                const ClassModel& model) const {
  double maxLog = -1e100;
  double invVar = 1.0 / (noiseStd_ * noiseStd_);
  for (size_t k = 0; k < model.featureVectors.size(); ++k) {
    double sumSq = 0.0;
    for (int j = 0; j < numFeatures_; ++j) {
      double diff = x[j] - model.featureVectors[k][j];
      sumSq += diff * diff;
    }
    double logLik = -0.5 * sumSq * invVar - 0.5 * numFeatures_ * std::log(2 * M_PI * noiseStd_ * noiseStd_);
    logLik += std::log(model.weights[k]);
    if (logLik > maxLog) maxLog = logLik;
  }
  return maxLog;
}

std::vector<int>
BayesianClassifier::predict(const std::vector<std::vector<double>> &X) const {
  auto probs = predictProbabilities(X);
  std::vector<int> pred(X.size());
  for (size_t i = 0; i < X.size(); ++i) {
    int bestClass = 0;
    double bestProb = probs[i][0];
    for (int c = 1; c < numClasses_; ++c) {
      if (probs[i][c] > bestProb) {
        bestProb = probs[i][c];
        bestClass = c;
      }
    }
    pred[i] = bestClass;
  }
  return pred;
}

std::vector<std::vector<double>>
BayesianClassifier::predictProbabilities(const std::vector<std::vector<double>> &X) const {
  if (!trained_ || X.empty()) return {};
  std::vector<std::vector<double>> probs(X.size(), std::vector<double>(numClasses_, 0.0));
  for (size_t i = 0; i < X.size(); ++i) {
    std::vector<double> logProbs(numClasses_);
    for (int c = 0; c < numClasses_; ++c) {
      logProbs[c] = std::log(priors_[c]) + computeLogLikelihood(X[i], classModels_[c]);
    }
    double maxLog = *std::max_element(logProbs.begin(), logProbs.end());
    double sumExp = 0.0;
    for (int c = 0; c < numClasses_; ++c) {
      probs[i][c] = std::exp(logProbs[c] - maxLog);
      sumExp += probs[i][c];
    }
    for (int c = 0; c < numClasses_; ++c) probs[i][c] /= sumExp;
  }
  return probs;
}