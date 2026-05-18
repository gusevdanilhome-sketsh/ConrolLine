#ifndef LOGISTIC_REGRESSION_H
#define LOGISTIC_REGRESSION_H

#include "classifier_base.h"
#include <vector>

class LogisticRegression : public ClassifierBase {
public:
  LogisticRegression();
  void setHyperparameters(double learningRate, int maxEpochs, double lambda,
                          int batchSize);
  void train(const std::vector<std::vector<double>> &X,
             const std::vector<int> &y) override;
  std::vector<int>
  predict(const std::vector<std::vector<double>> &X) const override;

private:
  double learning_rate_;
  int max_epochs_;
  double lambda_; // L2 regularization strength
  int batch_size_;
  int num_classes_;
  int num_features_;
  std::vector<std::vector<double>> weights_; // num_classes x num_features
  std::vector<double> biases_;

  // Вспомогательные функции
  void softmax(std::vector<double> &logits) const;
  double computeLoss(const std::vector<std::vector<double>> &X,
                     const std::vector<int> &y) const;
  void computeGradients(const std::vector<std::vector<double>> &batchX,
                        const std::vector<int> &batchY,
                        std::vector<std::vector<double>> &gradW,
                        std::vector<double> &gradB) const;
};

#endif