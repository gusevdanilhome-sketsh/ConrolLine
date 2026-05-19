#include "hyperparameter_tuner.h"
#include "classifiers/logistic_regression.h"
#include "utils/metrics.h"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>


HyperparameterTuner::BestParams HyperparameterTuner::tuneLogisticRegression(
    const std::vector<std::vector<double>> &X, const std::vector<int> &y,
    const Config &cfg) {

  BestParams best;
  best.bestScore = -1.0;
  int n = X.size();

  // Подготовка индексов для K-fold
  std::vector<int> indices(n);
  std::iota(indices.begin(), indices.end(), 0);
  std::shuffle(indices.begin(), indices.end(),
               std::mt19937{std::random_device{}()});

  int foldSize = n / cfg.kFold;

  for (double lr : cfg.learningRates) {
    for (double lambda : cfg.lambdas) {
      for (int epochs : cfg.epochs) {
        if (cfg.verbose) {
          std::cout << "Testing LR=" << lr << " λ=" << lambda
                    << " epochs=" << epochs << std::endl;
        }
        double foldScores = 0.0;
        for (int fold = 0; fold < cfg.kFold; ++fold) {
          int start = fold * foldSize;
          int end = (fold == cfg.kFold - 1) ? n : start + foldSize;
          std::vector<int> trainIndices, testIndices;
          for (int i = 0; i < n; ++i) {
            if (i >= start && i < end)
              testIndices.push_back(indices[i]);
            else
              trainIndices.push_back(indices[i]);
          }
          std::vector<std::vector<double>> X_train, X_test;
          std::vector<int> y_train, y_test;
          for (int idx : trainIndices) {
            X_train.push_back(X[idx]);
            y_train.push_back(y[idx]);
          }
          for (int idx : testIndices) {
            X_test.push_back(X[idx]);
            y_test.push_back(y[idx]);
          }

          LogisticRegression model;
          model.setHyperparameters(lr, epochs, lambda, 32);
          model.train(X_train, y_train);
          auto pred = model.predict(X_test);
          auto metrics = Metrics::computeAll(pred, y_test, 5);
          foldScores += metrics.macroF1;
        }
        double avgScore = foldScores / cfg.kFold;
        if (avgScore > best.bestScore) {
          best.bestScore = avgScore;
          best.learningRate = lr;
          best.lambda = lambda;
          best.epochs = epochs;
        }
      }
    }
  }
  if (cfg.verbose) {
    std::cout << "Best macro F1 = " << best.bestScore
              << " with LR=" << best.learningRate << " λ=" << best.lambda
              << " epochs=" << best.epochs << std::endl;
  }
  return best;
}