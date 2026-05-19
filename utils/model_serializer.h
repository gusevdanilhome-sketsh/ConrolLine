#ifndef MODEL_SERIALIZER_H
#define MODEL_SERIALIZER_H

#include <string>
#include <vector>

class LogisticRegression;
class LDA;
class NaiveBayes;

class ModelSerializer {
public:
  // Logistic Regression
  static bool saveLogisticRegression(const LogisticRegression &lr,
                                     const std::string &filename);
  static bool loadLogisticRegression(LogisticRegression &lr,
                                     const std::string &filename);

  // LDA
  static bool saveLDA(const LDA &lda, const std::string &filename);
  static bool loadLDA(LDA &lda, const std::string &filename);

  // Naive Bayes
  static bool saveNaiveBayes(const NaiveBayes &nb, const std::string &filename);
  static bool loadNaiveBayes(NaiveBayes &nb, const std::string &filename);
};

#endif