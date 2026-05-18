#include "classifiers.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace MicrowaveNDT {

// ---------------- LogisticRegression ----------------
void LogisticRegression::setParams(double lr, int epochs, double reg) {
  learning_rate_ = lr;
  max_epochs_ = epochs;
  lambda_ = reg;
}
void LogisticRegression::train(const Eigen::MatrixXd &X,
                               const Eigen::VectorXi &y) {
  num_features_ = (int)X.cols();
  num_classes_ = y.maxCoeff() + 1;
  weights_ = Eigen::MatrixXd::Zero(num_classes_, num_features_);
  bias_ = Eigen::VectorXd::Zero(num_classes_);
  for (int epoch = 0; epoch < max_epochs_; ++epoch) {
    Eigen::MatrixXd logits =
        X * weights_.transpose() + bias_.transpose().replicate(X.rows(), 1);
    softmax(logits);
    Eigen::MatrixXd gradW = Eigen::MatrixXd::Zero(num_classes_, num_features_);
    Eigen::VectorXd gradb = Eigen::VectorXd::Zero(num_classes_);
    for (int i = 0; i < X.rows(); ++i) {
      Eigen::VectorXd oneHot = Eigen::VectorXd::Zero(num_classes_);
      oneHot(y(i)) = 1.0;
      Eigen::VectorXd delta = logits.row(i).transpose() - oneHot;
      gradW += delta * X.row(i);
      gradb += delta;
    }
    gradW /= X.rows();
    gradb /= X.rows();
    weights_ -= learning_rate_ * (gradW + lambda_ * weights_);
    bias_ -= learning_rate_ * gradb;
  }
}
void LogisticRegression::softmax(Eigen::MatrixXd &logits) const {
  for (int i = 0; i < logits.rows(); ++i) {
    double maxVal = logits.row(i).maxCoeff();
    logits.row(i) = (logits.row(i).array() - maxVal).exp();
    double sum = logits.row(i).sum();
    logits.row(i) /= sum;
  }
}
Eigen::VectorXi LogisticRegression::predict(const Eigen::MatrixXd &X) const {
  Eigen::MatrixXd scores =
      X * weights_.transpose() + bias_.transpose().replicate(X.rows(), 1);
  Eigen::VectorXi pred(X.rows());
  for (int i = 0; i < X.rows(); ++i) {
    int idx;
    scores.row(i).maxCoeff(&idx);
    pred(i) = idx;
  }
  return pred;
}

// ---------------- LinearDiscriminantAnalysis ----------------
void LinearDiscriminantAnalysis::train(const Eigen::MatrixXd &X,
                                       const Eigen::VectorXi &y) {
  num_classes_ = y.maxCoeff() + 1;
  int n = X.rows(), d = X.cols();
  means_.resize(num_classes_, d);
  priors_.resize(num_classes_);
  Eigen::VectorXd globalMean = X.colwise().mean();
  Eigen::MatrixXd Sb = Eigen::MatrixXd::Zero(d, d);
  Eigen::MatrixXd Sw = Eigen::MatrixXd::Zero(d, d);
  for (int c = 0; c < num_classes_; ++c) {
    Eigen::MatrixXd classData = Eigen::MatrixXd::Zero(0, d);
    for (int i = 0; i < n; ++i)
      if (y(i) == c) {
        Eigen::MatrixXd tmp = X.row(i);
        classData.conservativeResize(classData.rows() + 1, d);
        classData.bottomRows(1) = tmp;
      }
    int nc = classData.rows();
    priors_(c) = (double)nc / n;
    Eigen::VectorXd mean = classData.colwise().mean();
    means_.row(c) = mean;
    Eigen::MatrixXd centered = classData.rowwise() - mean.transpose();
    Sw += centered.adjoint() * centered;
    Eigen::VectorXd diff = mean - globalMean;
    Sb += nc * diff * diff.transpose();
  }
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(Sw.ldlt().solve(Sb));
  projection_ = solver.eigenvectors().rightCols(std::min(num_classes_ - 1, d));
}

Eigen::VectorXi
LinearDiscriminantAnalysis::predict(const Eigen::MatrixXd &X) const {
  Eigen::MatrixXd Xproj = X * projection_;
  Eigen::MatrixXd meansProj = means_ * projection_;
  Eigen::VectorXi pred(X.rows());
  for (int i = 0; i < X.rows(); ++i) {
    double bestDist = std::numeric_limits<double>::max();
    int bestClass = 0;
    for (int c = 0; c < num_classes_; ++c) {
      double dist = (Xproj.row(i) - meansProj.row(c)).squaredNorm() -
                    2 * std::log(priors_(c));
      if (dist < bestDist) {
        bestDist = dist;
        bestClass = c;
      }
    }
    pred(i) = bestClass;
  }
  return pred;
}

// ---------------- GaussianNaiveBayes ----------------
void GaussianNaiveBayes::train(const Eigen::MatrixXd &X,
                               const Eigen::VectorXi &y) {
  num_classes_ = y.maxCoeff() + 1;
  class_means_.resize(num_classes_);
  class_vars_.resize(num_classes_);
  priors_.resize(num_classes_);
  int n = X.rows(), d = X.cols();
  for (int c = 0; c < num_classes_; ++c) {
    Eigen::MatrixXd classData = Eigen::MatrixXd::Zero(0, d);
    for (int i = 0; i < n; ++i)
      if (y(i) == c) {
        classData.conservativeResize(classData.rows() + 1, d);
        classData.bottomRows(1) = X.row(i);
      }
    int nc = classData.rows();
    priors_(c) = (double)nc / n;
    class_means_[c] = classData.colwise().mean();
    class_vars_[c] = (classData.rowwise() - class_means_[c].transpose())
                         .array()
                         .square()
                         .colwise()
                         .mean();
    // избегаем нулевой дисперсии
    class_vars_[c] = (class_vars_[c].array() + 1e-9).matrix();
  }
}
Eigen::VectorXi GaussianNaiveBayes::predict(const Eigen::MatrixXd &X) const {
  Eigen::VectorXi pred(X.rows());
  for (int i = 0; i < X.rows(); ++i) {
    Eigen::VectorXd logProb(num_classes_);
    for (int c = 0; c < num_classes_; ++c) {
      Eigen::VectorXd diff = X.row(i).transpose() - class_means_[c];
      Eigen::VectorXd logLik =
          -0.5 * (diff.array().square() / class_vars_[c].array() +
                  std::log(2 * M_PI) + class_vars_[c].array().log());
      logProb(c) = logLik.sum() + std::log(priors_(c));
    }
    int idx;
    logProb.maxCoeff(&idx);
    pred(i) = idx;
  }
  return pred;
}
} // namespace MicrowaveNDT