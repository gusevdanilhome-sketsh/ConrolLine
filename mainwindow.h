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
  void onGenerateData();
  void onTrain();
  void onClassify();
  void onSaveConfig();
  void onLoadConfig();
  void onResetConfig();
  void onParametersChanged();

private:
  Ui::MainWindow *ui;
  LineCalculator calculator_;
  DataGenerator generator_;
  LogisticRegression classifier_;
  std::vector<std::vector<double>> features_;
  std::vector<int> labels_;

  void appendToTerminal(const QString &txt);
  void updateLineParams();
  void loadSettings(const QJsonObject &obj);
  QJsonObject saveSettings() const;
};

#endif