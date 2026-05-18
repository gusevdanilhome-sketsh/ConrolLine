#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), generator_(calculator_) {
  ui->setupUi(this);
  qDebug() << "MainWindow constructor started";

  // Настройка минимальных значений
  ui->width_conductor_double_spin_box->setMinimum(1e-6);
  ui->thickness_conductor_double_spin_box->setMinimum(1e-6);
  ui->thickness_substrate_double_spin_box->setMinimum(1e-6);
  ui->dielectric_permittion_substrate_double_spin_box->setMinimum(1.0);

  // Сигналы
  connect(ui->saving_reconfiguration_push_button, &QPushButton::clicked, this,
          &MainWindow::onSaveConfig);
  connect(ui->booting_configuration_push_utton, &QPushButton::clicked, this,
          &MainWindow::onLoadConfig);
  connect(ui->reset_configuration_push_button, &QPushButton::clicked, this,
          &MainWindow::onResetConfig);
  connect(ui->width_conductor_double_spin_box, &QDoubleSpinBox::valueChanged,
          this, &MainWindow::onParametersChanged);
  connect(ui->thickness_conductor_double_spin_box,
          &QDoubleSpinBox::valueChanged, this,
          &MainWindow::onParametersChanged);
  connect(ui->thickness_substrate_double_spin_box,
          &QDoubleSpinBox::valueChanged, this,
          &MainWindow::onParametersChanged);
  connect(ui->dielectric_permittion_substrate_double_spin_box,
          &QDoubleSpinBox::valueChanged, this,
          &MainWindow::onParametersChanged);
  connect(ui->input_line_edit, &QLineEdit::returnPressed, this, [this]() {
    QString cmd = ui->input_line_edit->text().trimmed();
    if (cmd == "generate")
      onGenerateData();
    else if (cmd == "train")
      onTrain();
    else if (cmd == "classify")
      onClassify();
    else
      appendToTerminal("Unknown command");
    ui->input_line_edit->clear();
  });

  // Установка значений по умолчанию
  ui->width_conductor_double_spin_box->setValue(0.0002);
  ui->thickness_conductor_double_spin_box->setValue(35e-6);
  ui->thickness_substrate_double_spin_box->setValue(0.001);
  ui->dielectric_permittion_substrate_double_spin_box->setValue(4.6);
  ui->tangence_ougla_electric_loss_double_spin_box->setValue(0.02);
  ui->conductive_conductor_double_spin_box->setValue(5.8e7);

  onParametersChanged();
  generator_.setNoiseStd(0.05);
  appendToTerminal("Program started. Commands: generate, train, classify");
  qDebug() << "MainWindow constructor finished";
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::onParametersChanged() {
  double w = ui->width_conductor_double_spin_box->value();
  double t = ui->thickness_conductor_double_spin_box->value();
  double h = ui->thickness_substrate_double_spin_box->value();
  double er = ui->dielectric_permittion_substrate_double_spin_box->value();
  double tand = ui->tangence_ougla_electric_loss_double_spin_box->value();
  double sigma = ui->conductive_conductor_double_spin_box->value();
  calculator_.setParams(w, t, h, er, tand, sigma);
  updateLineParams();
}

void MainWindow::updateLineParams() {
  double Z0 = calculator_.calcZ0(1e9);
  double er_eff = calculator_.calcEffPermittivity(1e9);
  double alpha = calculator_.calcAttenuation(1e9);
  ui->output_parameters_text_browser->setText(
      QString("Z0 = %1 Ohm, ε_eff = %2, α = %3 Np/m")
          .arg(Z0, 0, 'f', 2)
          .arg(er_eff, 0, 'f', 4)
          .arg(alpha, 0, 'f', 4));
}

void MainWindow::onGenerateData() {
  appendToTerminal("Generating synthetic data...");
  std::vector<double> freqs;
  for (int f = 1; f <= 10; ++f)
    freqs.push_back(f * 1e9);
  generator_.generate(200, freqs, features_, labels_);
  appendToTerminal(QString("Generated %1 samples, %2 features each")
                       .arg(features_.size())
                       .arg(features_.empty() ? 0 : (int)features_[0].size()));
}

void MainWindow::onTrain() {
  if (features_.empty()) {
    appendToTerminal("No data. Run 'generate' first.");
    return;
  }
  appendToTerminal("Training classifier (dummy mode)...");
  classifier_.train(features_, labels_);
  appendToTerminal("Training finished (dummy classifier).");
}

void MainWindow::onClassify() {
  if (features_.empty()) {
    appendToTerminal("No data. Run 'generate' first.");
    return;
  }
  auto pred = classifier_.predict(features_);
  int correct = 0;
  for (size_t i = 0; i < pred.size(); ++i)
    if (pred[i] == labels_[i])
      correct++;
  double acc = (double)correct / pred.size();
  appendToTerminal(
      QString("Dummy classifier accuracy: %1%").arg(acc * 100, 0, 'f', 1));
}

void MainWindow::onSaveConfig() {
  QString fn = QFileDialog::getSaveFileName(this, "Save config", "", "*.json");
  if (fn.isEmpty())
    return;
  QFile f(fn);
  if (!f.open(QIODevice::WriteOnly))
    return;
  f.write(QJsonDocument(saveSettings()).toJson());
  appendToTerminal("Config saved to " + fn);
}

void MainWindow::onLoadConfig() {
  QString fn = QFileDialog::getOpenFileName(this, "Load config", "", "*.json");
  if (fn.isEmpty())
    return;
  QFile f(fn);
  if (!f.open(QIODevice::ReadOnly))
    return;
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  if (doc.isNull())
    return;
  loadSettings(doc.object());
  onParametersChanged();
  appendToTerminal("Config loaded from " + fn);
}

void MainWindow::onResetConfig() {
  ui->width_conductor_double_spin_box->setValue(0.0002);
  ui->thickness_conductor_double_spin_box->setValue(35e-6);
  ui->thickness_substrate_double_spin_box->setValue(0.001);
  ui->dielectric_permittion_substrate_double_spin_box->setValue(4.6);
  ui->tangence_ougla_electric_loss_double_spin_box->setValue(0.02);
  ui->conductive_conductor_double_spin_box->setValue(5.8e7);
  onParametersChanged();
  appendToTerminal("Config reset to default");
}

QJsonObject MainWindow::saveSettings() const {
  QJsonObject obj;
  obj["w"] = ui->width_conductor_double_spin_box->value();
  obj["t"] = ui->thickness_conductor_double_spin_box->value();
  obj["h"] = ui->thickness_substrate_double_spin_box->value();
  obj["er"] = ui->dielectric_permittion_substrate_double_spin_box->value();
  obj["tand"] = ui->tangence_ougla_electric_loss_double_spin_box->value();
  obj["sigma"] = ui->conductive_conductor_double_spin_box->value();
  return obj;
}

void MainWindow::loadSettings(const QJsonObject &obj) {
  if (obj.contains("w"))
    ui->width_conductor_double_spin_box->setValue(obj["w"].toDouble());
  if (obj.contains("t"))
    ui->thickness_conductor_double_spin_box->setValue(obj["t"].toDouble());
  if (obj.contains("h"))
    ui->thickness_substrate_double_spin_box->setValue(obj["h"].toDouble());
  if (obj.contains("er"))
    ui->dielectric_permittion_substrate_double_spin_box->setValue(
        obj["er"].toDouble());
  if (obj.contains("tand"))
    ui->tangence_ougla_electric_loss_double_spin_box->setValue(
        obj["tand"].toDouble());
  if (obj.contains("sigma"))
    ui->conductive_conductor_double_spin_box->setValue(obj["sigma"].toDouble());
}

void MainWindow::appendToTerminal(const QString &txt) {
  ui->terminal_text_browser->append(
      QDateTime::currentDateTime().toString("[hh:mm:ss] ") + txt);
}