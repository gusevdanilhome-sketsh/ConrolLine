#ifndef FEATURE_IMPORTANCE_H
#define FEATURE_IMPORTANCE_H

#include <QString>
#include <vector>


class LogisticRegression;

class FeatureImportance {
public:
  struct ImportanceItem {
    int featureIndex;
    double importance;
    QString description;
  };
  static std::vector<ImportanceItem>
  computeLogisticRegressionImportance(const LogisticRegression &lr,
                                      int nFeatures);
};

#endif