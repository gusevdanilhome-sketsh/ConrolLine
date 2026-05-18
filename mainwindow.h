#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
  explicit MainWindow(QWidget *parent = nullptr);
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

private:
  Ui::MainWindow *ui;

  // Простые поля вместо сложных моделей
  double currentWidth, currentThickness, currentHeight, currentEr,
      currentLossTang, currentSigma;
  double tolWidth, tolThickness, tolHeight;

  void updateParametersOutput();
  void updateToleranceOutput();
  void appendToTerminal(const QString &text);
  void appendToOutput(const QString &text);

  // Вспомогательные расчёты (упрощённые)
  double calcImpedance() const;
  double calcEffPermittivity() const;
  double calcAttenuation() const;

  // Для генерации данных (заглушка)
  void generateDummyData();
  void trainDummyClassifier();
};

#endif // MAINWINDOW_H