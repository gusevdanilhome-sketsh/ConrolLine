#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "classifiers/bayesian_classifier.h"
#include "classifiers/lda.h"
#include "classifiers/logistic_regression.h"
#include "classifiers/naive_bayes.h"
#include "data/data_generator.h"
#include "data/data_loader.h"
#include "models/line_calculator.h"
#include "models/microstrip_full.h"
#include "utils/complex_plot.h"
#include "utils/metrics.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QJsonObject>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <string>
#include <vector>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void onSaveConfiguration();
  void onLoadConfiguration();
  void onResetConfiguration();
  void onParametersChanged();
  void onToleranceChanged();
  void onGenerateData();
  void onTrainClassifier();
  void onClassify();
  void onSetClassifier(const QString &name);
  void updateHodographs();
  void updateQuadrature(int channel);

  void onClassifierSelectionChanged(int index);
  void onGenerateDataClicked();
  void onTrainClicked();
  void onClassifyClicked();
  void updateMetricsDisplay();
  void onLoadCsvClicked();
  void onExportMetricsClicked();
  void onPlotConfusionMatrix();
  void onPlotRocCurves();
  void onSensitivityAnalysis();

private:
  Ui::MainWindow *ui;
  LineCalculator calculator_;
  MicrostripFullModel fullModel_;
  DataGenerator generator_;
  DataLoader dataLoader_;
  LogisticRegression lr_;
  LDA lda_;
  NaiveBayes nb_;
  BayesianClassifier bayes_;
  ClassifierBase *classifier_;
  std::string currentClassifierName_;

  std::vector<std::vector<double>> features_;
  std::vector<int> labels_;
  std::vector<double> currentFreqs_;
  int currentDefectClass_ = 1;
  double currentDefectMagnitude_ = 0.5;
  std::vector<std::complex<double>> hodographTotal_, hodographVert_,
      hodographHoriz_;

  ComplexPlot *totalPlot_, *vertPlot_, *horizPlot_;
  ComplexPlot *iPlot_, *qPlot_;

  QComboBox *classifierCombo_;
  QDoubleSpinBox *learningRateSpin_;
  QSpinBox *epochsSpin_;
  QDoubleSpinBox *lambdaSpin_;
  QDoubleSpinBox *noiseStdSpin_;
  QSpinBox *samplesPerClassSpin_;
  QPushButton *generateBtn_;
  QPushButton *trainBtn_;
  QPushButton *classifyBtn_;
  QPushButton *loadCsvBtn_;
  QPushButton *exportMetricsBtn_;
  QPushButton *plotConfusionBtn_;
  QPushButton *plotRocBtn_;
  QPushButton *sensitivityBtn_;
  QTextEdit *metricsTextEdit_;

  void updateParametersOutput();
  void updateToleranceOutput();
  void printMetrics(const std::vector<int> &pred,
                    const std::vector<int> &trueLabels);
  void appendToTerminal(const QString &text);
  void appendToOutput(const QString &text);
  void loadSettingsFromJson(const QJsonObject &json);
  QJsonObject saveSettingsToJson() const;
  void setupCharts();
  void setupAdditionalUi();
  void logDetailedDataInfo(); // объявлено только один раз
};

#endif