#ifndef METRICS_H
#define METRICS_H

#include <QChartView>
#include <QString>
#include <vector>


struct MetricsResult {
  double accuracy;
  std::vector<double> precision;
  std::vector<double> recall;
  std::vector<double> f1;
  double macroF1;
};

class Metrics {
public:
  static MetricsResult computeAll(const std::vector<int> &pred,
                                  const std::vector<int> &trueLabels,
                                  int numClasses);
  static std::vector<std::vector<int>>
  confusionMatrix(const std::vector<int> &pred,
                  const std::vector<int> &trueLabels, int numClasses);
  static QString formatMetrics(const std::vector<int> &pred,
                               const std::vector<int> &trueLabels);
  static QString formatHtmlMetrics(const std::vector<int> &pred,
                                   const std::vector<int> &trueLabels);
  static QChartView *
  plotConfusionMatrix(const std::vector<std::vector<int>> &cm, int numClasses);
  static QChartView *
  plotRocCurves(const std::vector<std::vector<double>> &probabilities,
                const std::vector<int> &trueLabels, int numClasses);
};

#endif