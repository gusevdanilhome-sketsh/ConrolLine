#ifndef HYPERPARAMETER_TUNER_H
#define HYPERPARAMETER_TUNER_H

#include <functional>
#include <vector>


class HyperparameterTuner {
public:
  struct Config {
    std::vector<double> learningRates = {0.001, 0.005, 0.01, 0.05, 0.1};
    std::vector<double> lambdas = {0.0, 0.001, 0.01, 0.1};
    std::vector<int> epochs = {200, 500, 1000};
    int kFold = 5;
    bool verbose = true;
  };

  struct BestParams {
    double learningRate;
    double lambda;
    int epochs;
    double bestScore;
  };

  // Убрано значение по умолчанию = Config()
  static BestParams
  tuneLogisticRegression(const std::vector<std::vector<double>> &X,
                         const std::vector<int> &y, const Config &cfg);
};

#endif