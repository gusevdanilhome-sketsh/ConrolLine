#include "model_serializer.h"
#include "classifiers/lda.h"
#include "classifiers/logistic_regression.h"
#include "classifiers/naive_bayes.h"
#include <fstream>
#include <iostream>
#include <sstream>


// Вспомогательная функция для записи вектора
template <typename T>
static void writeVector(std::ofstream &out, const std::vector<T> &vec) {
  size_t sz = vec.size();
  out.write(reinterpret_cast<const char *>(&sz), sizeof(sz));
  out.write(reinterpret_cast<const char *>(vec.data()), sz * sizeof(T));
}

template <typename T>
static void readVector(std::ifstream &in, std::vector<T> &vec) {
  size_t sz;
  in.read(reinterpret_cast<char *>(&sz), sizeof(sz));
  vec.resize(sz);
  in.read(reinterpret_cast<char *>(vec.data()), sz * sizeof(T));
}

// Для двумерных векторов
template <typename T>
static void writeMatrix(std::ofstream &out,
                        const std::vector<std::vector<T>> &mat) {
  size_t rows = mat.size();
  out.write(reinterpret_cast<const char *>(&rows), sizeof(rows));
  for (const auto &row : mat) {
    size_t cols = row.size();
    out.write(reinterpret_cast<const char *>(&cols), sizeof(cols));
    out.write(reinterpret_cast<const char *>(row.data()), cols * sizeof(T));
  }
}

template <typename T>
static void readMatrix(std::ifstream &in, std::vector<std::vector<T>> &mat) {
  size_t rows;
  in.read(reinterpret_cast<char *>(&rows), sizeof(rows));
  mat.resize(rows);
  for (size_t i = 0; i < rows; ++i) {
    size_t cols;
    in.read(reinterpret_cast<char *>(&cols), sizeof(cols));
    mat[i].resize(cols);
    in.read(reinterpret_cast<char *>(mat[i].data()), cols * sizeof(T));
  }
}

bool ModelSerializer::saveLogisticRegression(const LogisticRegression &lr,
                                             const std::string &filename) {
  std::ofstream out(filename, std::ios::binary);
  if (!out)
    return false;
  // Магическое число
  int magic = 0x4C525047; // "LRPG"
  out.write(reinterpret_cast<const char *>(&magic), sizeof(magic));
  int version = 1;
  out.write(reinterpret_cast<const char *>(&version), sizeof(version));

  auto &weights = lr.getWeights();
  auto &biases = lr.getBiases();
  int numClasses = lr.getNumClasses();
  int numFeatures = lr.getNumFeatures();
  out.write(reinterpret_cast<const char *>(&numClasses), sizeof(numClasses));
  out.write(reinterpret_cast<const char *>(&numFeatures), sizeof(numFeatures));
  writeMatrix(out, weights);
  writeVector(out, biases);
  return true;
}

bool ModelSerializer::loadLogisticRegression(LogisticRegression &lr,
                                             const std::string &filename) {
  std::ifstream in(filename, std::ios::binary);
  if (!in)
    return false;
  int magic;
  in.read(reinterpret_cast<char *>(&magic), sizeof(magic));
  if (magic != 0x4C525047)
    return false;
  int version;
  in.read(reinterpret_cast<char *>(&version), sizeof(version));
  int numClasses, numFeatures;
  in.read(reinterpret_cast<char *>(&numClasses), sizeof(numClasses));
  in.read(reinterpret_cast<char *>(&numFeatures), sizeof(numFeatures));
  std::vector<std::vector<double>> weights;
  std::vector<double> biases;
  readMatrix(in, weights);
  readVector(in, biases);
  lr.setWeights(weights);
  lr.setBiases(biases);
  return true;
}

bool ModelSerializer::saveLDA(const LDA &lda, const std::string &filename) {
  std::ofstream out(filename, std::ios::binary);
  if (!out)
    return false;
  int magic = 0x4C444147; // "LDAG"
  out.write(reinterpret_cast<const char *>(&magic), sizeof(magic));
  int version = 1;
  out.write(reinterpret_cast<const char *>(&version), sizeof(version));
  int numClasses = lda.getNumClasses();
  int numFeatures = lda.getNumFeatures();
  out.write(reinterpret_cast<const char *>(&numClasses), sizeof(numClasses));
  out.write(reinterpret_cast<const char *>(&numFeatures), sizeof(numFeatures));
  writeMatrix(out, lda.getMeans());
  writeMatrix(out, lda.getInvCov());
  writeVector(out, lda.getPriors());
  return true;
}

bool ModelSerializer::loadLDA(LDA &lda, const std::string &filename) {
  std::ifstream in(filename, std::ios::binary);
  if (!in)
    return false;
  int magic;
  in.read(reinterpret_cast<char *>(&magic), sizeof(magic));
  if (magic != 0x4C444147)
    return false;
  int version;
  in.read(reinterpret_cast<char *>(&version), sizeof(version));
  int numClasses, numFeatures;
  in.read(reinterpret_cast<char *>(&numClasses), sizeof(numClasses));
  in.read(reinterpret_cast<char *>(&numFeatures), sizeof(numFeatures));
  std::vector<std::vector<double>> means, invCov;
  std::vector<double> priors;
  readMatrix(in, means);
  readMatrix(in, invCov);
  readVector(in, priors);
  lda.setNumClasses(numClasses);
  lda.setNumFeatures(numFeatures);
  lda.setMeans(means);
  lda.setInvCov(invCov);
  lda.setPriors(priors);
  lda.setTrained(true);
  return true;
}

bool ModelSerializer::saveNaiveBayes(const NaiveBayes &nb,
                                     const std::string &filename) {
  std::ofstream out(filename, std::ios::binary);
  if (!out)
    return false;
  int magic = 0x4E424147; // "NBAG"
  out.write(reinterpret_cast<const char *>(&magic), sizeof(magic));
  int version = 1;
  out.write(reinterpret_cast<const char *>(&version), sizeof(version));
  int numClasses = nb.getNumClasses();
  int numFeatures = nb.getNumFeatures();
  out.write(reinterpret_cast<const char *>(&numClasses), sizeof(numClasses));
  out.write(reinterpret_cast<const char *>(&numFeatures), sizeof(numFeatures));
  writeMatrix(out, nb.getMeans());
  writeMatrix(out, nb.getVariances());
  writeVector(out, nb.getPriors());
  return true;
}

bool ModelSerializer::loadNaiveBayes(NaiveBayes &nb,
                                     const std::string &filename) {
  std::ifstream in(filename, std::ios::binary);
  if (!in)
    return false;
  int magic;
  in.read(reinterpret_cast<char *>(&magic), sizeof(magic));
  if (magic != 0x4E424147)
    return false;
  int version;
  in.read(reinterpret_cast<char *>(&version), sizeof(version));
  int numClasses, numFeatures;
  in.read(reinterpret_cast<char *>(&numClasses), sizeof(numClasses));
  in.read(reinterpret_cast<char *>(&numFeatures), sizeof(numFeatures));
  std::vector<std::vector<double>> means, vars;
  std::vector<double> priors;
  readMatrix(in, means);
  readMatrix(in, vars);
  readVector(in, priors);
  nb.setNumClasses(numClasses);
  nb.setNumFeatures(numFeatures);
  nb.setMeans(means);
  nb.setVariances(vars);
  nb.setPriors(priors);
  nb.setTrained(true);
  return true;
}