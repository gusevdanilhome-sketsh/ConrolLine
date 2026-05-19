#ifndef NAIVE_BAYES_H
#define NAIVE_BAYES_H

#include "classifier_base.h"
#include <vector>

class NaiveBayes : public ClassifierBase {
public:
  void train(const std::vector<std::vector<double>> &X,
             const std::vector<int> &y) override;
  std::vector<int>
  predict(const std::vector<std::vector<double>> &X) const override;
  const std::vector<std::vector<double>> &getMeans() const { return means_; }
  const std::vector<std::vector<double>> &getVariances() const {
    return variances_;
  }
  const std::vector<double> &getPriors() const { return priors_; }
  void setMeans(const std::vector<std::vector<double>> &m) { means_ = m; }
  void setVariances(const std::vector<std::vector<double>> &v) {
    variances_ = v;
  }
  void setPriors(const std::vector<double> &p) { priors_ = p; }
  void setNumClasses(int nc) { num_classes_ = nc; }
  void setNumFeatures(int nf) { num_features_ = nf; }
  void setTrained(bool t) { trained_ = t; }
  int getNumClasses() const { return num_classes_; }
  int getNumFeatures() const { return num_features_; }
  bool isTrained() const { return trained_; }

private:
  std::vector<std::vector<double>> means_;
  std::vector<std::vector<double>> variances_;
  std::vector<double> priors_;
  int num_classes_ = 0;
  int num_features_ = 0;
  bool trained_ = false;
};

#endif