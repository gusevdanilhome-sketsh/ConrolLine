#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextBrowser>
#include <QVBoxLayout>

void MainWindow::onSetClassifier(const QString &name) {
  if (name == "lr") {
    classifier_ = &lr_;
    currentClassifierName_ = "lr";
    appendToTerminal("Активен классификатор: логистическая регрессия");
  } else if (name == "lda") {
    classifier_ = &lda_;
    currentClassifierName_ = "lda";
    appendToTerminal("Активен классификатор: LDA (заглушка)");
  } else if (name == "nb") {
    classifier_ = &nb_;
    currentClassifierName_ = "nb";
    appendToTerminal("Активен классификатор: наивный Байес (заглушка)");
  } else {
    appendToTerminal("Неизвестный классификатор. Используйте: lr, lda, nb");
  }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), generator_(calculator_),
      classifier_(&lr_) // по умолчанию логистическая регрессия
      ,
      currentClassifierName_("lr") {
  ui->setupUi(this);
  qDebug() << "MainWindow constructor started";

  // Установка минимальных значений для спинбоксов
  ui->width_conductor_double_spin_box->setMinimum(1e-6);
  ui->thickness_conductor_double_spin_box->setMinimum(1e-6);
  ui->thickness_substrate_double_spin_box->setMinimum(1e-6);
  ui->dielectric_permittion_substrate_double_spin_box->setMinimum(1.0);
  ui->tangence_ougla_electric_loss_double_spin_box->setMinimum(0.0);
  ui->conductive_conductor_double_spin_box->setMinimum(1e3);
  ui->width_conductor_double_spin_box_2->setMinimum(0.0);
  ui->thickness_conductor_double_spin_box_2->setMinimum(0.0);
  ui->thickness_substrate_double_spin_box_2->setMinimum(0.0);

  // Установка начальных значений
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

  // Подключение сигналов
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

  connect(ui->width_conductor_double_spin_box_2,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onToleranceChanged);
  connect(ui->thickness_conductor_double_spin_box_2,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onToleranceChanged);
  connect(ui->thickness_substrate_double_spin_box_2,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onToleranceChanged);

  connect(ui->input_line_edit, &QLineEdit::returnPressed, this, [this]() {
    QString cmd = ui->input_line_edit->text().trimmed();
    if (cmd == "generate")
      onGenerateData();
    else if (cmd == "train")
      onTrainClassifier();
    else if (cmd == "classify")
      onClassify();
    else if (cmd.startsWith("set classifier ")) {
      QString name = cmd.mid(15).trimmed();
      onSetClassifier(name);
    } else if (!cmd.isEmpty())
      appendToTerminal("Неизвестная команда. Доступны: generate, train, "
                       "classify, set classifier lr/lda/nb");
    ui->input_line_edit->clear();
  });

  lr_.setHyperparameters(0.01, 500, 0.01, 32);

  // Первоначальный расчёт
  onParametersChanged();
  onToleranceChanged();
  setupCharts();

  generator_.setNoiseStd(0.05);
  appendToTerminal("Программа запущена. Команды: generate, train, classify");
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

  if (w <= 0.0 || t <= 0.0 || h <= 0.0 || er < 1.0) {
    ui->output_parameters_text_browser->setText(
        "Ошибка: некорректные параметры (размеры > 0, εr ≥ 1)");
    return;
  }
  calculator_.setParams(w, t, h, er, tand, sigma);
  updateParametersOutput();
}

void MainWindow::updateParametersOutput() {
  double f = 1.0e9;
  double Z0 = calculator_.calcZ0(f);
  double er_eff = calculator_.calcEffPermittivity(f);
  double alpha = calculator_.calcAttenuation(f);
  ui->output_parameters_text_browser->setText(
      QString(
          "Параметры линии при f=1 ГГц:\nZ0 = %1 Ом\nε_eff = %2\nα = %3 Нп/м")
          .arg(Z0, 0, 'f', 2)
          .arg(er_eff, 0, 'f', 4)
          .arg(alpha, 0, 'f', 4));
}

void MainWindow::updateToleranceOutput() {
  double w_nom = ui->width_conductor_double_spin_box->value();
  double t_nom = ui->thickness_conductor_double_spin_box->value();
  double h_nom = ui->thickness_substrate_double_spin_box->value();
  double w_tol = ui->width_conductor_double_spin_box_2->value();
  double t_tol = ui->thickness_conductor_double_spin_box_2->value();
  double h_tol = ui->thickness_substrate_double_spin_box_2->value();

  QString msg = QString("Отклонения:\nШирина: %1 м (%2%)\nТолщина: %3 м "
                        "(%4%)\nПодложка: %5 м (%6%)")
                    .arg(w_tol - w_nom, 0, 'e', 2)
                    .arg((w_tol - w_nom) / w_nom * 100, 0, 'f', 2)
                    .arg(t_tol - t_nom, 0, 'e', 2)
                    .arg((t_tol - t_nom) / t_nom * 100, 0, 'f', 2)
                    .arg(h_tol - h_nom, 0, 'e', 2)
                    .arg((h_tol - h_nom) / h_nom * 100, 0, 'f', 2);
  ui->output_parameters_text_browser_2->setText(msg);
}

void MainWindow::onToleranceChanged() { updateToleranceOutput(); }

void MainWindow::onGenerateData() {
  appendToTerminal("Генерация синтетических данных...");
  std::vector<double> freqs;
  for (int f = 1; f <= 10; ++f)
    freqs.push_back(f * 1e9);
  generator_.generate(200, freqs, features_, labels_);
  appendToTerminal(QString("Сгенерировано %1 образцов, каждый с %2 признаками")
                       .arg(features_.size())
                       .arg(features_.empty() ? 0 : (int)features_[0].size()));
  appendToOutput("Данные сгенерированы. Теперь можно выполнить train.");
}

void MainWindow::onTrainClassifier() {
  if (features_.empty()) {
    appendToTerminal("Ошибка: нет данных. Сначала выполните generate.");
    return;
  }
  appendToTerminal("Обучение классификатора (логистическая регрессия)...");
  classifier_->train(features_, labels_);
  appendToTerminal("Обучение завершено.");
  appendToOutput("Классификатор обучен.");
}

void MainWindow::onClassify() {
  if (features_.empty()) {
    appendToTerminal("Ошибка: нет данных. Сначала выполните generate и train.");
    return;
  }
  appendToTerminal("Классификация...");
  std::vector<int> pred = classifier_->predict(features_);
  int correct = 0;
  for (size_t i = 0; i < pred.size(); ++i)
    if (pred[i] == labels_[i])
      ++correct;
  double acc = 100.0 * correct / pred.size();
  appendToTerminal(
      QString("Точность на обучающей выборке: %1% (случайное угадывание ~20%)")
          .arg(acc, 0, 'f', 2));
  appendToOutput("Классификация выполнена.");
}

void MainWindow::onSaveConfiguration() {
  QString fileName = QFileDialog::getSaveFileName(
      this, "Сохранить конфигурацию", "", "JSON (*.json)");
  if (fileName.isEmpty())
    return;
  QJsonObject obj = saveSettingsToJson();
  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(QJsonDocument(obj).toJson());
    appendToTerminal("Конфигурация сохранена в " + fileName);
  } else {
    appendToTerminal("Ошибка сохранения");
  }
}

void MainWindow::onLoadConfiguration() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Загрузить конфигурацию", "", "JSON (*.json)");
  if (fileName.isEmpty())
    return;
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly)) {
    appendToTerminal("Не удалось открыть файл");
    return;
  }
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  if (doc.isNull()) {
    appendToTerminal("Некорректный JSON");
    return;
  }
  loadSettingsFromJson(doc.object());
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
  ui->width_conductor_double_spin_box_2->setValue(0.0002);
  ui->thickness_conductor_double_spin_box_2->setValue(35e-6);
  ui->thickness_substrate_double_spin_box_2->setValue(0.001);
  onParametersChanged();
  onToleranceChanged();
  appendToTerminal("Конфигурация сброшена к значениям по умолчанию");
}

QJsonObject MainWindow::saveSettingsToJson() const {
  QJsonObject obj;
  obj["width"] = ui->width_conductor_double_spin_box->value();
  obj["thickness"] = ui->thickness_conductor_double_spin_box->value();
  obj["height"] = ui->thickness_substrate_double_spin_box->value();
  obj["er"] = ui->dielectric_permittion_substrate_double_spin_box->value();
  obj["tand"] = ui->tangence_ougla_electric_loss_double_spin_box->value();
  obj["sigma"] = ui->conductive_conductor_double_spin_box->value();
  obj["width_tol"] = ui->width_conductor_double_spin_box_2->value();
  obj["thickness_tol"] = ui->thickness_conductor_double_spin_box_2->value();
  obj["height_tol"] = ui->thickness_substrate_double_spin_box_2->value();
  return obj;
}

void MainWindow::loadSettingsFromJson(const QJsonObject &obj) {
  if (obj.contains("width"))
    ui->width_conductor_double_spin_box->setValue(obj["width"].toDouble());
  if (obj.contains("thickness"))
    ui->thickness_conductor_double_spin_box->setValue(
        obj["thickness"].toDouble());
  if (obj.contains("height"))
    ui->thickness_substrate_double_spin_box->setValue(obj["height"].toDouble());
  if (obj.contains("er"))
    ui->dielectric_permittion_substrate_double_spin_box->setValue(
        obj["er"].toDouble());
  if (obj.contains("tand"))
    ui->tangence_ougla_electric_loss_double_spin_box->setValue(
        obj["tand"].toDouble());
  if (obj.contains("sigma"))
    ui->conductive_conductor_double_spin_box->setValue(obj["sigma"].toDouble());
  if (obj.contains("width_tol"))
    ui->width_conductor_double_spin_box_2->setValue(
        obj["width_tol"].toDouble());
  if (obj.contains("thickness_tol"))
    ui->thickness_conductor_double_spin_box_2->setValue(
        obj["thickness_tol"].toDouble());
  if (obj.contains("height_tol"))
    ui->thickness_substrate_double_spin_box_2->setValue(
        obj["height_tol"].toDouble());
}

void MainWindow::appendToTerminal(const QString &text) {
  ui->terminal_text_browser->append(
      QDateTime::currentDateTime().toString("[hh:mm:ss] ") + text);
}

void MainWindow::appendToOutput(const QString &text) {
  ui->output_text_browser->append(
      QDateTime::currentDateTime().toString("[hh:mm:ss] ") + text);
}

// Заглушки визуализации (без реальных графиков, но без падений)
void MainWindow::setupCharts() {
  if (ui->total_channel_frame->layout())
    delete ui->total_channel_frame->layout();
  ui->total_channel_frame->setLayout(new QVBoxLayout);
  ui->total_channel_frame->layout()->addWidget(
      new QLabel("Годограф суммарного канала (заглушка)"));

  if (ui->verticy_channel_frame->layout())
    delete ui->verticy_channel_frame->layout();
  ui->verticy_channel_frame->setLayout(new QVBoxLayout);
  ui->verticy_channel_frame->layout()->addWidget(
      new QLabel("Годограф вертикального канала (заглушка)"));

  if (ui->horizontal_channel_frame->layout())
    delete ui->horizontal_channel_frame->layout();
  ui->horizontal_channel_frame->setLayout(new QVBoxLayout);
  ui->horizontal_channel_frame->layout()->addWidget(
      new QLabel("Годограф горизонтального канала (заглушка)"));

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

  appendToTerminal("Визуализация графиков временно отключена (заглушка)");
}

void MainWindow::updateHodographs() {
  appendToTerminal("Обновление годографов (заглушка)");
}

void MainWindow::updateQuadrature(int channel) {
  appendToTerminal(
      QString("Обновление квадратурных составляющих для канала %1 (заглушка)")
          .arg(channel));
}