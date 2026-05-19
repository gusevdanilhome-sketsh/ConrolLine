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
  std::vector<std::vector<double>>
  predictProbabilities(const std::vector<std::vector<double>> &X) const;
  std::vector<int> predictWithLoss(
      const std::vector<std::vector<double>> &X,
      const std::vector<std::vector<double>> &lossMatrix) const override;
  const std::vector<std::vector<double>> &getWeights() const {
    return weights_;
  }
  const std::vector<double> &getBiases() const { return biases_; }
  int getNumClasses() const { return num_classes_; }
  int getNumFeatures() const { return num_features_; }
  // Также добавьте метод setWeights для загрузки
  void setWeights(const std::vector<std::vector<double>> &w) {
    weights_ = w;
    num_classes_ = w.size();
    if (!w.empty())
      num_features_ = w[0].size();
  }
  void setBiases(const std::vector<double> &b) { biases_ = b; }
  std::vector<std::vector<double>> weights_;
  std::vector<double> biases_;
  void softmax(std::vector<double> &logits) const;
  double computeLoss(const std::vector<std::vector<double>> &X,
                     const std::vector<int> &y) const;
  void computeGradients(const std::vector<std::vector<double>> &batchX,
                        const std::vector<int> &batchY,
                        std::vector<std::vector<double>> &gradW,
                        std::vector<double> &gradB) const;

private:
  double learning_rate_;
  int max_epochs_;
  double lambda_;
  int batch_size_;
  int num_classes_;
  int num_features_;
};

#endif