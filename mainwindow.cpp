#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <cmath>
#include <random>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // Ограничения ввода
  ui->width_conductor_double_spin_box->setMinimum(1e-9);
  ui->thickness_conductor_double_spin_box->setMinimum(1e-9);
  ui->thickness_substrate_double_spin_box->setMinimum(1e-9);
  ui->dielectric_permittion_substrate_double_spin_box->setMinimum(1.0);
  ui->tangence_ougla_electric_loss_double_spin_box->setMinimum(0.0);
  ui->conductive_conductor_double_spin_box->setMinimum(1e3);
  ui->field_strength_conductor_breakdown_double_spin_box->setMinimum(0.0);

  ui->width_conductor_double_spin_box_2->setMinimum(0.0);
  ui->thickness_conductor_double_spin_box_2->setMinimum(0.0);
  ui->thickness_substrate_double_spin_box_2->setMinimum(0.0);

  // Начальные значения
  ui->width_conductor_double_spin_box->setValue(0.0002);
  ui->thickness_conductor_double_spin_box->setValue(35e-6);
  ui->thickness_substrate_double_spin_box->setValue(0.001);
  ui->dielectric_permittion_substrate_double_spin_box->setValue(4.6);
  ui->tangence_ougla_electric_loss_double_spin_box->setValue(0.02);
  ui->conductive_conductor_double_spin_box->setValue(5.8e7);
  ui->field_strength_conductor_breakdown_double_spin_box->setValue(20.0);

  ui->width_conductor_double_spin_box_2->setValue(0.0002);
  ui->thickness_conductor_double_spin_box_2->setValue(35e-6);
  ui->thickness_substrate_double_spin_box_2->setValue(0.001);

  // Кнопки конфигурации
  connect(ui->saving_reconfiguration_push_button, &QPushButton::clicked, this,
          &MainWindow::onSaveConfiguration);
  connect(ui->booting_configuration_push_utton, &QPushButton::clicked, this,
          &MainWindow::onLoadConfiguration);
  connect(ui->saving_reconfiguration_push_button_2, &QPushButton::clicked, this,
          &MainWindow::onSaveConfiguration);
  connect(ui->booting_configuration_push_utton_2, &QPushButton::clicked, this,
          &MainWindow::onLoadConfiguration);
  connect(ui->saving_reconfiguration_push_button_3, &QPushButton::clicked, this,
          &MainWindow::onSaveConfiguration);
  connect(ui->booting_configuration_push_utton_3, &QPushButton::clicked, this,
          &MainWindow::onLoadConfiguration);
  connect(ui->reset_configuration_push_button, &QPushButton::clicked, this,
          &MainWindow::onResetConfiguration);

  // Параметры
  connect(ui->width_conductor_double_spin_box,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onParametersChanged);
  connect(ui->thickness_conductor_double_spin_box,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onParametersChanged);
  connect(ui->thickness_substrate_double_spin_box,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onParametersChanged);
  connect(ui->dielectric_permittion_substrate_double_spin_box,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onParametersChanged);
  connect(ui->tangence_ougla_electric_loss_double_spin_box,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onParametersChanged);
  connect(ui->conductive_conductor_double_spin_box,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onParametersChanged);
  connect(ui->field_strength_conductor_breakdown_double_spin_box,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onParametersChanged);

  // Допуски
  connect(ui->width_conductor_double_spin_box_2,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onToleranceChanged);
  connect(ui->thickness_conductor_double_spin_box_2,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onToleranceChanged);
  connect(ui->thickness_substrate_double_spin_box_2,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onToleranceChanged);

  // Консольные команды
  connect(ui->input_line_edit, &QLineEdit::returnPressed, this, [this]() {
    QString cmd = ui->input_line_edit->text().trimmed();
    if (cmd == "generate")
      onGenerateData();
    else if (cmd == "train")
      onTrainClassifier();
    else if (cmd == "classify")
      onClassify();
    else if (!cmd.isEmpty())
      appendToTerminal(
          "Неизвестная команда. Доступны: generate, train, classify");
    ui->input_line_edit->clear();
  });

  // Первоначальный расчёт
  onParametersChanged();
  onToleranceChanged();

  // Заглушки для виджетов графиков (чтобы не было пустых фреймов)
  if (ui->total_channel_frame->layout())
    delete ui->total_channel_frame->layout();
  ui->total_channel_frame->setLayout(new QVBoxLayout);
  ui->total_channel_frame->layout()->addWidget(
      new QLabel("Годограф (заглушка)"));
  if (ui->verticy_channel_frame->layout())
    delete ui->verticy_channel_frame->layout();
  ui->verticy_channel_frame->setLayout(new QVBoxLayout);
  ui->verticy_channel_frame->layout()->addWidget(
      new QLabel("Годограф (заглушка)"));
  if (ui->horizontal_channel_frame->layout())
    delete ui->horizontal_channel_frame->layout();
  ui->horizontal_channel_frame->setLayout(new QVBoxLayout);
  ui->horizontal_channel_frame->layout()->addWidget(
      new QLabel("Годограф (заглушка)"));
  if (ui->i_component_frame->layout())
    delete ui->i_component_frame->layout();
  ui->i_component_frame->setLayout(new QVBoxLayout);
  ui->i_component_frame->layout()->addWidget(
      new QLabel("I-составляющая (заглушка)"));
  if (ui->q_component_frame->layout())
    delete ui->q_component_frame->layout();
  ui->q_component_frame->setLayout(new QVBoxLayout);
  ui->q_component_frame->layout()->addWidget(
      new QLabel("Q-составляющая (заглушка)"));

  appendToTerminal("Программа запущена (минимальная версия). Команды: "
                   "generate, train, classify");
}

MainWindow::~MainWindow() { delete ui; }

// --- Расчётные методы (упрощённые) ---
double MainWindow::calcImpedance() const {
  double wh = currentWidth / currentHeight;
  double er_eff = (currentEr + 1.0) / 2.0 +
                  (currentEr - 1.0) / (2.0 * std::sqrt(1.0 + 12.0 / wh));
  double A = wh + 1.393 + 0.667 * std::log(wh + 1.444);
  return 120.0 * M_PI / (std::sqrt(er_eff) * A);
}

double MainWindow::calcEffPermittivity() const {
  double wh = currentWidth / currentHeight;
  return (currentEr + 1.0) / 2.0 +
         (currentEr - 1.0) / (2.0 * std::sqrt(1.0 + 12.0 / wh));
}

double MainWindow::calcAttenuation() const {
  // Просто заглушка
  return 0.01;
}

// --- Параметры ---
void MainWindow::onParametersChanged() {
  currentWidth = ui->width_conductor_double_spin_box->value();
  currentThickness = ui->thickness_conductor_double_spin_box->value();
  currentHeight = ui->thickness_substrate_double_spin_box->value();
  currentEr = ui->dielectric_permittion_substrate_double_spin_box->value();
  currentLossTang = ui->tangence_ougla_electric_loss_double_spin_box->value();
  currentSigma = ui->conductive_conductor_double_spin_box->value();

  if (currentWidth <= 0.0 || currentHeight <= 0.0 || currentEr < 1.0) {
    ui->output_parameters_text_browser->setText(
        "Ошибка: некорректные параметры");
    return;
  }
  updateParametersOutput();
}

void MainWindow::updateParametersOutput() {
  double Z0 = calcImpedance();
  double er_eff = calcEffPermittivity();
  double alpha = calcAttenuation();

  QString text = QString("Параметры линии (упрощённый расчёт):\n"
                         "Волновое сопротивление Z0 = %1 Ом\n"
                         "Эффективная проницаемость ε_eff = %2\n"
                         "Погонное затухание α = %3 Нп/м")
                     .arg(Z0, 0, 'f', 2)
                     .arg(er_eff, 0, 'f', 4)
                     .arg(alpha, 0, 'f', 4);
  ui->output_parameters_text_browser->setText(text);
}

// --- Допуски ---
void MainWindow::onToleranceChanged() {
  tolWidth = ui->width_conductor_double_spin_box_2->value();
  tolThickness = ui->thickness_conductor_double_spin_box_2->value();
  tolHeight = ui->thickness_substrate_double_spin_box_2->value();
  updateToleranceOutput();
}

void MainWindow::updateToleranceOutput() {
  double w_dev = tolWidth - currentWidth;
  double t_dev = tolThickness - currentThickness;
  double h_dev = tolHeight - currentHeight;

  QString msg = QString("Отклонения от номиналов:\n"
                        "Ширина: %1 м (%2%)\n"
                        "Толщина: %3 м (%4%)\n"
                        "Высота: %5 м (%6%)")
                    .arg(w_dev, 0, 'e', 2)
                    .arg((w_dev / currentWidth) * 100.0, 0, 'f', 4)
                    .arg(t_dev, 0, 'e', 2)
                    .arg((t_dev / currentThickness) * 100.0, 0, 'f', 4)
                    .arg(h_dev, 0, 'e', 2)
                    .arg((h_dev / currentHeight) * 100.0, 0, 'f', 4);
  ui->output_parameters_text_browser_2->setText(msg);
}

// --- Генерация данных (заглушка) ---
void MainWindow::generateDummyData() {
  // Имитация генерации
  appendToTerminal("Генерация синтетических данных (заглушка): 5 классов, 200 "
                   "образцов, 10 частот");
}

void MainWindow::onGenerateData() {
  generateDummyData();
  appendToOutput("Данные сгенерированы (заглушка).");
}

// --- Обучение классификатора (заглушка) ---
void MainWindow::trainDummyClassifier() {
  appendToTerminal("Обучение классификатора (заглушка): accuracy 0.705");
}

void MainWindow::onTrainClassifier() {
  trainDummyClassifier();
  appendToOutput("Классификатор обучен (заглушка).");
}

void MainWindow::onClassify() {
  appendToTerminal("Классификация выполнена (заглушка).");
  appendToOutput("Результаты классификации выведены в консоль (заглушка).");
}

// --- Сохранение/загрузка конфигурации (упрощённая) ---
void MainWindow::onSaveConfiguration() {
  QString fileName = QFileDialog::getSaveFileName(
      this, "Сохранить конфигурацию", "", "JSON (*.json)");
  if (fileName.isEmpty())
    return;
  QJsonObject json;
  json["width"] = currentWidth;
  json["thickness"] = currentThickness;
  json["height"] = currentHeight;
  json["er"] = currentEr;
  json["loss_tangent"] = currentLossTang;
  json["sigma"] = currentSigma;
  json["tol_width"] = tolWidth;
  json["tol_thickness"] = tolThickness;
  json["tol_height"] = tolHeight;

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл.");
    return;
  }
  file.write(QJsonDocument(json).toJson());
  file.close();
  appendToTerminal("Конфигурация сохранена в " + fileName);
}

void MainWindow::onLoadConfiguration() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Загрузить конфигурацию", "", "JSON (*.json)");
  if (fileName.isEmpty())
    return;
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл.");
    return;
  }
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  file.close();
  if (doc.isNull()) {
    QMessageBox::warning(this, "Ошибка", "Некорректный JSON.");
    return;
  }
  QJsonObject json = doc.object();
  if (json.contains("width"))
    ui->width_conductor_double_spin_box->setValue(json["width"].toDouble());
  if (json.contains("thickness"))
    ui->thickness_conductor_double_spin_box->setValue(
        json["thickness"].toDouble());
  if (json.contains("height"))
    ui->thickness_substrate_double_spin_box->setValue(
        json["height"].toDouble());
  if (json.contains("er"))
    ui->dielectric_permittion_substrate_double_spin_box->setValue(
        json["er"].toDouble());
  if (json.contains("loss_tangent"))
    ui->tangence_ougla_electric_loss_double_spin_box->setValue(
        json["loss_tangent"].toDouble());
  if (json.contains("sigma"))
    ui->conductive_conductor_double_spin_box->setValue(
        json["sigma"].toDouble());
  if (json.contains("tol_width"))
    ui->width_conductor_double_spin_box_2->setValue(
        json["tol_width"].toDouble());
  if (json.contains("tol_thickness"))
    ui->thickness_conductor_double_spin_box_2->setValue(
        json["tol_thickness"].toDouble());
  if (json.contains("tol_height"))
    ui->thickness_substrate_double_spin_box_2->setValue(
        json["tol_height"].toDouble());

  onParametersChanged();
  onToleranceChanged();
  appendToTerminal("Конфигурация загружена из " + fileName);
}

void MainWindow::onResetConfiguration() {
  ui->width_conductor_double_spin_box->setValue(0.0002);
  ui->thickness_conductor_double_spin_box->setValue(35e-6);
  ui->thickness_substrate_double_spin_box->setValue(0.001);
  ui->dielectric_permittion_substrate_double_spin_box->setValue(4.6);
  ui->tangence_ougla_electric_loss_double_spin_box->setValue(0.02);
  ui->conductive_conductor_double_spin_box->setValue(5.8e7);
  ui->field_strength_conductor_breakdown_double_spin_box->setValue(20.0);
  ui->width_conductor_double_spin_box_2->setValue(0.0002);
  ui->thickness_conductor_double_spin_box_2->setValue(35e-6);
  ui->thickness_substrate_double_spin_box_2->setValue(0.001);
  onParametersChanged();
  onToleranceChanged();
  appendToTerminal("Конфигурация сброшена к значениям по умолчанию.");
}

// --- Вспомогательные выводы ---
void MainWindow::appendToTerminal(const QString &text) {
  ui->terminal_text_browser->append(
      QDateTime::currentDateTime().toString("[hh:mm:ss] ") + text);
}

void MainWindow::appendToOutput(const QString &text) {
  ui->output_text_browser->append(
      QDateTime::currentDateTime().toString("[hh:mm:ss] ") + text);
}