#include "feature_importance.h"
#include "classifiers/logistic_regression.h"
#include <algorithm>
#include <cmath>

std::vector<FeatureImportance::ImportanceItem>
FeatureImportance::computeLogisticRegressionImportance(
    const LogisticRegression &lr, int nFeatures) {

  auto weights = lr.getWeights();
  int numClasses = weights.size();
  if (numClasses == 0 || nFeatures == 0)
    return {};

  // Вычисляем средний абсолютный вес для каждого признака
  std::vector<double> avgAbsWeight(nFeatures, 0.0);
  for (int c = 0; c < numClasses; ++c) {
    for (int j = 0; j < nFeatures; ++j) {
      avgAbsWeight[j] += std::abs(weights[c][j]);
    }
  }
  for (int j = 0; j < nFeatures; ++j) {
    avgAbsWeight[j] /= numClasses;
  }

  std::vector<ImportanceItem> items;
  for (int j = 0; j < nFeatures; ++j) {
    items.push_back({j, avgAbsWeight[j], QString("Признак %1").arg(j)});
  }
  std::sort(items.begin(), items.end(),
            [](const ImportanceItem &a, const ImportanceItem &b) {
              return a.importance > b.importance;
            });
  return items;
}