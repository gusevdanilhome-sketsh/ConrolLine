#include "complex_plot.h"
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>


ComplexPlot::ComplexPlot(QWidget *parent) : QChartView(parent) {
  chart_ = new QChart();
  axisX_ = new QValueAxis();
  axisY_ = new QValueAxis();
  axisX_->setTitleText("Re");
  axisY_->setTitleText("Im");
  chart_->addAxis(axisX_, Qt::AlignBottom);
  chart_->addAxis(axisY_, Qt::AlignLeft);
  chart_->setTitle("Годограф");
  setChart(chart_);
  setRenderHint(QPainter::Antialiasing);
}

void ComplexPlot::setData(const std::vector<std::complex<double>> &points,
                          bool asScatter) {
  chart_->removeAllSeries();
  if (points.empty())
    return;
  if (asScatter) {
    QScatterSeries *series = new QScatterSeries();
    series->setMarkerSize(8);
    for (const auto &p : points)
      series->append(p.real(), p.imag());
    chart_->addSeries(series);
    series->attachAxis(axisX_);
    series->attachAxis(axisY_);
  } else {
    QLineSeries *series = new QLineSeries();
    for (const auto &p : points)
      series->append(p.real(), p.imag());
    chart_->addSeries(series);
    series->attachAxis(axisX_);
    series->attachAxis(axisY_);
  }
  chart_->createDefaultAxes();
  // Удаляем созданные по умолчанию оси и подключаем наши
  auto axesX = chart_->axes(Qt::Horizontal);
  if (!axesX.isEmpty())
    chart_->removeAxis(axesX.first());
  auto axesY = chart_->axes(Qt::Vertical);
  if (!axesY.isEmpty())
    chart_->removeAxis(axesY.first());
  chart_->addAxis(axisX_, Qt::AlignBottom);
  chart_->addAxis(axisY_, Qt::AlignLeft);
  for (auto *series : chart_->series()) {
    series->attachAxis(axisX_);
    series->attachAxis(axisY_);
  }
}

void ComplexPlot::clear() { chart_->removeAllSeries(); }

void ComplexPlot::setTitle(const QString &title) { chart_->setTitle(title); }

void ComplexPlot::setAxesLabels(const QString &xLabel, const QString &yLabel) {
  axisX_->setTitleText(xLabel);
  axisY_->setTitleText(yLabel);
}