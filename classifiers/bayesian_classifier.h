#ifndef BAYESIAN_CLASSIFIER_H
#define BAYESIAN_CLASSIFIER_H

#include "classifier_base.h"
#include <complex>
#include <vector>


class BayesianClassifier : public ClassifierBase {
public:
  BayesianClassifier();
  void setParameterGrid();
  void setNoiseStd(double sigma);
  void train(const std::vector<std::vector<double>> &X,
             const std::vector<int> &y) override;
  std::vector<int>
  predict(const std::vector<std::vector<double>> &X) const override;
  std::vector<std::vector<double>>
  predictProbabilities(const std::vector<std::vector<double>> &X) const;

private:
  struct ClassModel {
    std::vector<std::vector<double>> featureVectors; // f(y, θ_k)
    std::vector<double> weights;                     // w_k
  };
  std::vector<ClassModel> classModels_;
  int numClasses_ = 5;
  int numFeatures_ = 0;
  double noiseStd_ = 0.05;
  bool trained_ = false;

  std::vector<double> priors_;
  void buildClassModels();
  double computeLogLikelihood(const std::vector<double> &x,
                              const ClassModel &model) const;
};

#endif