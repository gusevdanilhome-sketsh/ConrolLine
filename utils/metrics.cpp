#include "metrics.h"
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <algorithm>
#include <cmath>
#include <numeric>


MetricsResult Metrics::computeAll(const std::vector<int> &pred,
                                  const std::vector<int> &trueLabels,
                                  int numClasses) {
  MetricsResult res;
  res.precision.assign(numClasses, 0.0);
  res.recall.assign(numClasses, 0.0);
  res.f1.assign(numClasses, 0.0);
  res.auc.assign(numClasses, 0.0);
  std::vector<int> tp(numClasses, 0), fp(numClasses, 0), fn(numClasses, 0);
  for (size_t i = 0; i < pred.size(); ++i) {
    int t = trueLabels[i];
    int p = pred[i];
    if (p == t)
      tp[t]++;
    else {
      fp[p]++;
      fn[t]++;
    }
  }
  int correct = std::accumulate(tp.begin(), tp.end(), 0);
  res.accuracy = 100.0 * correct / pred.size();
  double macroF1sum = 0.0;
  for (int c = 0; c < numClasses; ++c) {
    if (tp[c] + fp[c] > 0)
      res.precision[c] = 100.0 * tp[c] / (tp[c] + fp[c]);
    if (tp[c] + fn[c] > 0)
      res.recall[c] = 100.0 * tp[c] / (tp[c] + fn[c]);
    if (res.precision[c] + res.recall[c] > 0)
      res.f1[c] = 2 * res.precision[c] * res.recall[c] /
                  (res.precision[c] + res.recall[c]);
    macroF1sum += res.f1[c];
  }
  res.macroF1 = macroF1sum / numClasses;
  return res;
}

std::vector<std::vector<int>>
Metrics::confusionMatrix(const std::vector<int> &pred,
                         const std::vector<int> &trueLabels, int numClasses) {
  std::vector<std::vector<int>> cm(numClasses, std::vector<int>(numClasses, 0));
  for (size_t i = 0; i < pred.size(); ++i) {
    cm[trueLabels[i]][pred[i]]++;
  }
  return cm;
}

QString Metrics::formatMetrics(const std::vector<int> &pred,
                               const std::vector<int> &trueLabels) {
  auto res = computeAll(pred, trueLabels, 5);
  QString out;
  out += QString("Overall accuracy: %1%\n").arg(res.accuracy, 0, 'f', 2);
  out += "Per-class metrics:\n";
  for (int c = 0; c < 5; ++c) {
    out += QString("Class %1: P=%.1f%% R=%.1f%% F1=%.1f%% AUC=%.3f\n")
               .arg(c)
               .arg(res.precision[c])
               .arg(res.recall[c])
               .arg(res.f1[c])
               .arg(res.auc[c]);
  }
  out += QString("Macro F1 = %1%\n").arg(res.macroF1, 0, 'f', 2);
  return out;
}

QString Metrics::formatHtmlMetrics(const std::vector<int> &pred,
                                   const std::vector<int> &trueLabels) {
  auto res = computeAll(pred, trueLabels, 5);
  QString html = "<b>Общая точность (Accuracy):</b> " +
                 QString::number(res.accuracy, 'f', 2) + "%<br><br>";
  html += "<table border='1' cellpadding='5' cellspacing='0'>";
  html += "<tr><th>Класс</th><th>Precision (%)</th><th>Recall "
          "(%)</th><th>F1-score (%)</th><th>AUC</th></tr>";
  for (int c = 0; c < 5; ++c) {
    html +=
        QString(
            "<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
            .arg(c)
            .arg(res.precision[c], 0, 'f', 1)
            .arg(res.recall[c], 0, 'f', 1)
            .arg(res.f1[c], 0, 'f', 1)
            .arg(res.auc[c], 0, 'f', 3);
  }
  html += "</table><br>";
  html += QString("<b>Macro F1 = %1%</b><br><b>Macro AUC = %2</b>")
              .arg(res.macroF1, 0, 'f', 2)
              .arg(std::accumulate(res.auc.begin(), res.auc.end(), 0.0) /
                       res.auc.size(),
                   0, 'f', 3);
  return html;
}

QChartView *
Metrics::plotConfusionMatrix(const std::vector<std::vector<int>> &cm,
                             int numClasses) {
  QBarSeries *series = new QBarSeries();
  for (int i = 0; i < numClasses; ++i) {
    QBarSet *set = new QBarSet(QString("True %1").arg(i));
    for (int j = 0; j < numClasses; ++j)
      *set << cm[i][j];
    series->append(set);
  }
  QChart *chart = new QChart();
  chart->addSeries(series);
  chart->setTitle("Confusion Matrix");
  chart->setAnimationOptions(QChart::SeriesAnimations);
  QBarCategoryAxis *axisX = new QBarCategoryAxis();
  QStringList categories;
  for (int i = 0; i < numClasses; ++i)
    categories << QString::number(i);
  axisX->append(categories);
  chart->addAxis(axisX, Qt::AlignBottom);
  series->attachAxis(axisX);
  QValueAxis *axisY = new QValueAxis();
  axisY->setTitleText("Count");
  chart->addAxis(axisY, Qt::AlignLeft);
  series->attachAxis(axisY);
  QChartView *chartView = new QChartView(chart);
  chartView->setRenderHint(QPainter::Antialiasing);
  return chartView;
}

QChartView *
Metrics::plotRocCurves(const std::vector<std::vector<double>> &probabilities,
                       const std::vector<int> &trueLabels, int numClasses) {
  QChart *chart = new QChart();
  chart->setTitle("ROC Curves (One-vs-All)");
  for (int c = 0; c < numClasses; ++c) {
    std::vector<std::pair<double, double>> points;
    for (double thresh = 0.0; thresh <= 1.01; thresh += 0.01) {
      int tp = 0, fp = 0, tn = 0, fn = 0;
      for (size_t i = 0; i < probabilities.size(); ++i) {
        bool predPos = (probabilities[i][c] >= thresh);
        bool truePos = (trueLabels[i] == c);
        if (predPos && truePos)
          tp++;
        else if (predPos && !truePos)
          fp++;
        else if (!predPos && truePos)
          fn++;
        else
          tn++;
      }
      double tpr = (tp + fn == 0) ? 0 : (double)tp / (tp + fn);
      double fpr = (fp + tn == 0) ? 0 : (double)fp / (fp + tn);
      points.push_back({fpr, tpr});
    }
    std::sort(points.begin(), points.end());
    QLineSeries *series = new QLineSeries();
    for (auto &p : points)
      series->append(p.first, p.second);
    series->setName(QString("Class %1").arg(c));
    chart->addSeries(series);
  }
  QValueAxis *axisX = new QValueAxis();
  axisX->setTitleText("False Positive Rate");
  axisX->setRange(0, 1);
  QValueAxis *axisY = new QValueAxis();
  axisY->setTitleText("True Positive Rate");
  axisY->setRange(0, 1);
  chart->addAxis(axisX, Qt::AlignBottom);
  chart->addAxis(axisY, Qt::AlignLeft);
  for (auto *series : chart->series()) {
    series->attachAxis(axisX);
    series->attachAxis(axisY);
  }
  QChartView *chartView = new QChartView(chart);
  chartView->setRenderHint(QPainter::Antialiasing);
  return chartView;
}

double Metrics::computeAUC(const std::vector<double> &scores,
                           const std::vector<int> &trueBinary) {
  std::vector<std::pair<double, int>> pairs;
  for (size_t i = 0; i < scores.size(); ++i)
    pairs.push_back({scores[i], trueBinary[i]});
  std::sort(pairs.begin(), pairs.end(),
            [](const auto &a, const auto &b) { return a.first > b.first; });
  int nPos = std::count(trueBinary.begin(), trueBinary.end(), 1);
  int nNeg = trueBinary.size() - nPos;
  if (nPos == 0 || nNeg == 0)
    return 0.5;
  double rankSum = 0.0;
  for (size_t i = 0; i < pairs.size(); ++i) {
    if (pairs[i].second == 1)
      rankSum += (i + 1);
  }
  double auc = (rankSum - nPos * (nPos + 1.0) / 2.0) / (nPos * nNeg);
  return auc;
}

std::vector<double>
Metrics::computeAllAUC(const std::vector<std::vector<double>> &probabilities,
                       const std::vector<int> &trueLabels, int numClasses) {
  std::vector<double> aucs(numClasses, 0.0);
  for (int c = 0; c < numClasses; ++c) {
    std::vector<double> scores;
    std::vector<int> binary;
    for (size_t i = 0; i < probabilities.size(); ++i) {
      scores.push_back(probabilities[i][c]);
      binary.push_back(trueLabels[i] == c ? 1 : 0);
    }
    aucs[c] = computeAUC(scores, binary);
  }
  return aucs;
}