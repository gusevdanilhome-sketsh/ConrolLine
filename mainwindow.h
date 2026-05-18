#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "classifiers/logistic_regression.h"
#include "data/data_generator.h"
#include "models/line_calculator.h"
#include <QJsonObject>
#include <QMainWindow>
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
  void onToleranceChanged(); // <-- добавить
  void onGenerateData();
  void onTrainClassifier();
  void onClassify();

private:
  Ui::MainWindow *ui;
  LineCalculator calculator_;
  DataGenerator generator_;
  LogisticRegression classifier_;
  std::vector<std::vector<double>> features_;
  std::vector<int> labels_;

  void updateParametersOutput();
  void updateToleranceOutput();
  void appendToTerminal(const QString &text);
  void appendToOutput(const QString &text);
  void loadSettingsFromJson(const QJsonObject &json);
  QJsonObject saveSettingsToJson() const;
  void setupCharts();                 // заглушка визуализации
  void updateHodographs();            // заглушка
  void updateQuadrature(int channel); // заглушка
};

#endif