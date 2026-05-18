#ifndef COMPLEX_PLOT_H
#define COMPLEX_PLOT_H

#include <QChartView>
#include <complex>
#include <vector>


QT_BEGIN_NAMESPACE
class QChart;
class QValueAxis;
QT_END_NAMESPACE

class ComplexPlot : public QChartView {
  Q_OBJECT
public:
  explicit ComplexPlot(QWidget *parent = nullptr);
  void setData(const std::vector<std::complex<double>> &points,
               bool asScatter = false);
  void clear();
  void setTitle(const QString &title);
  void setAxesLabels(const QString &xLabel, const QString &yLabel);

private:
  QChart *chart_;
  QValueAxis *axisX_;
  QValueAxis *axisY_;
};

#endif