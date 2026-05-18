#ifndef CLASSIFIERS_H
#define CLASSIFIERS_H

#include <Eigen/Dense>
#include <vector>

namespace MicrowaveNDT {

class ClassifierBase {
public:
  virtual ~ClassifierBase() = default;
  virtual void train(const Eigen::MatrixXd &X, const Eigen::VectorXi &y) = 0;
  virtual Eigen::VectorXi predict(const Eigen::MatrixXd &X) const = 0;
};

class LogisticRegression : public ClassifierBase {
public:
  void setParams(double lr = 0.01, int epochs = 500, double reg = 1.0);
  void train(const Eigen::MatrixXd &X, const Eigen::VectorXi &y) override;
  Eigen::VectorXi predict(const Eigen::MatrixXd &X) const override;

private:
  Eigen::MatrixXd weights_;
  Eigen::VectorXd bias_;
  double learning_rate_ = 0.01;
  int max_epochs_ = 500;
  double lambda_ = 1.0;
  int num_classes_ = 0;
  int num_features_ = 0;
  void softmax(Eigen::MatrixXd &logits) const;
};

class LinearDiscriminantAnalysis : public ClassifierBase {
public:
  void train(const Eigen::MatrixXd &X, const Eigen::VectorXi &y) override;
  Eigen::VectorXi predict(const Eigen::MatrixXd &X) const override;

private:
  Eigen::MatrixXd means_;
  Eigen::MatrixXd projection_;
  Eigen::VectorXd priors_;
  int num_classes_ = 0;
};

class GaussianNaiveBayes : public ClassifierBase {
public:
  void train(const Eigen::MatrixXd &X, const Eigen::VectorXi &y) override;
  Eigen::VectorXi predict(const Eigen::MatrixXd &X) const override;

private:
  std::vector<Eigen::VectorXd> class_means_;
  std::vector<Eigen::VectorXd> class_vars_;
  Eigen::VectorXd priors_;
  int num_classes_ = 0;
};

} // namespace MicrowaveNDT
#endif