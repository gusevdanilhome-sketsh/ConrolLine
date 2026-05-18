#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils/complex_plot.h"
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <numeric>
#include <random>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), generator_(calculator_),
      classifier_(&lr_), currentClassifierName_("lr") {
  ui->setupUi(this);

  // Установка минимальных значений
  ui->width_conductor_double_spin_box->setMinimum(1e-6);
  ui->thickness_conductor_double_spin_box->setMinimum(1e-6);
  ui->thickness_substrate_double_spin_box->setMinimum(1e-6);
  ui->dielectric_permittion_substrate_double_spin_box->setMinimum(1.0);
  ui->tangence_ougla_electric_loss_double_spin_box->setMinimum(0.0);
  ui->conductive_conductor_double_spin_box->setMinimum(1e3);
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

  // Сигналы существующих элементов
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

  // Консольные команды (оставляем)
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
    } else if (cmd == "plot hodograph")
      updateHodographs();
    else if (cmd.startsWith("plot quadrature ")) {
      int ch = cmd.mid(16).trimmed().toInt();
      updateQuadrature(ch);
    } else if (cmd.startsWith("set defect class ")) {
      int cls = cmd.mid(17).trimmed().toInt();
      if (cls >= 0 && cls <= 4)
        currentDefectClass_ = cls;
      else
        appendToTerminal("Class must be 0..4");
    } else if (cmd.startsWith("set defect mag ")) {
      double mag = cmd.mid(15).trimmed().toDouble();
      if (mag >= 0 && mag <= 1)
        currentDefectMagnitude_ = mag;
      else
        appendToTerminal("Magnitude must be 0..1");
    } else if (!cmd.isEmpty())
      appendToTerminal("Unknown command");
    ui->input_line_edit->clear();
  });

  onParametersChanged();
  onToleranceChanged();
  setupCharts();

  // Настройка генератора и классификатора по умолчанию
  lr_.setHyperparameters(0.01, 500, 0.01, 32);
  generator_.setNoiseStd(0.05);

  // ========== СОЗДАНИЕ ПОЛЬЗОВАТЕЛЬСКОГО ИНТЕРФЕЙСА В frame ==========
  // Очищаем существующий layout во frame (если что-то было)
  QLayout *oldLayout = ui->frame->layout();
  if (oldLayout) {
    QLayoutItem *item;
    while ((item = oldLayout->takeAt(0)) != nullptr) {
      delete item->widget();
      delete item;
    }
    delete oldLayout;
  }
  QGridLayout *frameLayout = new QGridLayout(ui->frame);
  ui->frame->setLayout(frameLayout);

  // Группа настройки данных
  QGroupBox *dataGroup = new QGroupBox("Генерация данных", this);
  QFormLayout *dataLayout = new QFormLayout(dataGroup);
  noiseStdSpin_ = new QDoubleSpinBox(this);
  noiseStdSpin_->setRange(0.0, 0.5);
  noiseStdSpin_->setSingleStep(0.01);
  noiseStdSpin_->setDecimals(3);
  noiseStdSpin_->setValue(0.05);
  dataLayout->addRow("Стандартное отклонение шума:", noiseStdSpin_);

  samplesPerClassSpin_ = new QSpinBox(this);
  samplesPerClassSpin_->setRange(50, 1000);
  samplesPerClassSpin_->setSingleStep(50);
  samplesPerClassSpin_->setValue(200);
  dataLayout->addRow("Образцов на класс:", samplesPerClassSpin_);

  generateBtn_ = new QPushButton("Сгенерировать данные", this);
  dataLayout->addRow(generateBtn_);
  frameLayout->addWidget(dataGroup, 0, 0, 1, 2);

  // Группа настройки классификатора
  QGroupBox *classifierGroup = new QGroupBox("Классификатор", this);
  QFormLayout *classifierLayout = new QFormLayout(classifierGroup);
  classifierCombo_ = new QComboBox(this);
  classifierCombo_->addItems({"Логистическая регрессия (Softmax)",
                              "Линейный дискриминантный анализ (LDA)",
                              "Наивный Байес (GaussianNB)"});
  classifierLayout->addRow("Тип:", classifierCombo_);

  learningRateSpin_ = new QDoubleSpinBox(this);
  learningRateSpin_->setRange(1e-6, 1.0);
  learningRateSpin_->setDecimals(6);
  learningRateSpin_->setSingleStep(0.001);
  learningRateSpin_->setValue(0.01);
  classifierLayout->addRow("Скорость обучения (LR):", learningRateSpin_);

  epochsSpin_ = new QSpinBox(this);
  epochsSpin_->setRange(10, 2000);
  epochsSpin_->setSingleStep(50);
  epochsSpin_->setValue(500);
  classifierLayout->addRow("Количество эпох:", epochsSpin_);

  lambdaSpin_ = new QDoubleSpinBox(this);
  lambdaSpin_->setRange(0.0, 1.0);
  lambdaSpin_->setDecimals(6);
  lambdaSpin_->setSingleStep(0.01);
  lambdaSpin_->setValue(0.01);
  classifierLayout->addRow("Сила регуляризации (λ):", lambdaSpin_);

  frameLayout->addWidget(classifierGroup, 1, 0, 1, 2);

  // Кнопки управления
  QHBoxLayout *buttonLayout = new QHBoxLayout;
  trainBtn_ = new QPushButton("Обучить", this);
  classifyBtn_ = new QPushButton("Классифицировать", this);
  buttonLayout->addWidget(trainBtn_);
  buttonLayout->addWidget(classifyBtn_);
  frameLayout->addLayout(buttonLayout, 2, 0, 1, 2);

  // Вывод метрик
  QGroupBox *metricsGroup = new QGroupBox("Метрики качества", this);
  QVBoxLayout *metricsLayout = new QVBoxLayout(metricsGroup);
  metricsTextEdit_ = new QTextEdit(this);
  metricsTextEdit_->setReadOnly(true);
  metricsTextEdit_->setMinimumHeight(200);
  metricsLayout->addWidget(metricsTextEdit_);
  frameLayout->addWidget(metricsGroup, 3, 0, 1, 2);

  // Подключение сигналов новых элементов
  connect(generateBtn_, &QPushButton::clicked, this,
          &MainWindow::onGenerateDataClicked);
  connect(trainBtn_, &QPushButton::clicked, this, &MainWindow::onTrainClicked);
  connect(classifyBtn_, &QPushButton::clicked, this,
          &MainWindow::onClassifyClicked);
  connect(classifierCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onClassifierSelectionChanged);
  connect(learningRateSpin_,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [this](double val) {
            if (currentClassifierName_ == "lr")
              lr_.setHyperparameters(val, epochsSpin_->value(),
                                     lambdaSpin_->value(), 32);
            appendToTerminal(QString("Learning rate обновлён: %1").arg(val));
          });
  connect(epochsSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
          [this](int val) {
            if (currentClassifierName_ == "lr")
              lr_.setHyperparameters(learningRateSpin_->value(), val,
                                     lambdaSpin_->value(), 32);
            appendToTerminal(QString("Эпох обновлено: %1").arg(val));
          });
  connect(lambdaSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [this](double val) {
            if (currentClassifierName_ == "lr")
              lr_.setHyperparameters(learningRateSpin_->value(),
                                     epochsSpin_->value(), val, 32);
            appendToTerminal(QString("Регуляризация λ обновлена: %1").arg(val));
          });
  connect(noiseStdSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [this](double val) {
            generator_.setNoiseStd(val);
            appendToTerminal(QString("Уровень шума установлен: %1").arg(val));
          });

  // Установка начального классификатора
  onClassifierSelectionChanged(0);

  appendToTerminal("Program started. Commands: generate, train, classify, set "
                   "classifier lr/lda/nb, plot hodograph, plot quadrature "
                   "0/1/2, set defect class 0..4, set defect mag 0..1");
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
    ui->output_parameters_text_browser->setText("Error: invalid parameters");
    return;
  }
  calculator_.setParams(w, t, h, er, tand, sigma);
  fullModel_.setParams(w, t, h, er, tand, sigma);
  updateParametersOutput();
}

void MainWindow::updateParametersOutput() {
  double f = 1.0e9;
  double Z0 = calculator_.calcZ0(f);
  double er_eff = calculator_.calcEffPermittivity(f);
  double alpha = calculator_.calcAttenuation(f);
  ui->output_parameters_text_browser->setText(
      QString("Line params at 1 GHz:\nZ0 = %1 Ohm\nε_eff = %2\nα = %3 Np/m")
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

  QString msg = QString("Deviations:\nWidth: %1 m (%2%)\nThick: %3 m "
                        "(%4%)\nHeight: %5 m (%6%)")
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
  appendToTerminal("Generating synthetic data...");
  std::vector<double> freqs;
  for (int f = 1; f <= 10; ++f)
    freqs.push_back(f * 1e9);
  generator_.generate(samplesPerClassSpin_->value(), freqs, features_, labels_);
  appendToTerminal(QString("Generated %1 samples, each with %2 features")
                       .arg(features_.size())
                       .arg(features_.empty() ? 0 : (int)features_[0].size()));
  appendToOutput("Data generated. Now you can train a classifier.");
}

void MainWindow::onTrainClassifier() {
  if (features_.empty()) {
    appendToTerminal("No data. Run 'generate' first.");
    return;
  }
  appendToTerminal(QString("Training classifier: %1...")
                       .arg(QString::fromStdString(currentClassifierName_)));
  classifier_->train(features_, labels_);
  appendToTerminal("Training finished.");
}

void MainWindow::onClassify() {
  if (features_.empty()) {
    appendToTerminal("No data. Run 'generate' first.");
    return;
  }
  appendToTerminal("Classifying...");
  auto pred = classifier_->predict(features_);
  printMetrics(pred, labels_);
  updateMetricsDisplay(); // обновляем текстовое поле в frame
}

void MainWindow::printMetrics(const std::vector<int> &pred,
                              const std::vector<int> &trueLabels) {
  int nClasses = 5;
  std::vector<int> tp(nClasses, 0), fp(nClasses, 0), fn(nClasses, 0);
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
  double acc = 100.0 * correct / pred.size();
  appendToTerminal(QString("Overall accuracy: %1%").arg(acc, 0, 'f', 2));
  for (int c = 0; c < nClasses; ++c) {
    double prec = (tp[c] + fp[c] == 0) ? 0 : 100.0 * tp[c] / (tp[c] + fp[c]);
    double rec = (tp[c] + fn[c] == 0) ? 0 : 100.0 * tp[c] / (tp[c] + fn[c]);
    double f1 = (prec + rec == 0) ? 0 : 2 * prec * rec / (prec + rec);
    appendToTerminal(QString("Class %1: P=%.1f%% R=%.1f%% F1=%.1f%%")
                         .arg(c)
                         .arg(prec)
                         .arg(rec)
                         .arg(f1));
  }
}

void MainWindow::onSetClassifier(const QString &name) {
  if (name == "lr") {
    classifier_ = &lr_;
    currentClassifierName_ = "lr";
    appendToTerminal("Classifier set to Logistic Regression");
  } else if (name == "lda") {
    classifier_ = &lda_;
    currentClassifierName_ = "lda";
    appendToTerminal("Classifier set to LDA");
  } else if (name == "nb") {
    classifier_ = &nb_;
    currentClassifierName_ = "nb";
    appendToTerminal("Classifier set to Naive Bayes");
  } else {
    appendToTerminal("Unknown classifier. Use lr, lda, nb");
  }
}

void MainWindow::onSaveConfiguration() {
  QString fn = QFileDialog::getSaveFileName(this, "Save config", "", "*.json");
  if (fn.isEmpty())
    return;
  QFile f(fn);
  if (!f.open(QIODevice::WriteOnly)) {
    appendToTerminal("Cannot save file");
    return;
  }
  f.write(QJsonDocument(saveSettingsToJson()).toJson());
  appendToTerminal("Config saved to " + fn);
}

void MainWindow::onLoadConfiguration() {
  QString fn = QFileDialog::getOpenFileName(this, "Load config", "", "*.json");
  if (fn.isEmpty())
    return;
  QFile f(fn);
  if (!f.open(QIODevice::ReadOnly)) {
    appendToTerminal("Cannot open file");
    return;
  }
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  if (doc.isNull()) {
    appendToTerminal("Invalid JSON");
    return;
  }
  loadSettingsFromJson(doc.object());
  onParametersChanged();
  onToleranceChanged();
  appendToTerminal("Config loaded from " + fn);
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
  appendToTerminal("Config reset to default");
}

QJsonObject MainWindow::saveSettingsToJson() const {
  QJsonObject obj;
  obj["w"] = ui->width_conductor_double_spin_box->value();
  obj["t"] = ui->thickness_conductor_double_spin_box->value();
  obj["h"] = ui->thickness_substrate_double_spin_box->value();
  obj["er"] = ui->dielectric_permittion_substrate_double_spin_box->value();
  obj["tand"] = ui->tangence_ougla_electric_loss_double_spin_box->value();
  obj["sigma"] = ui->conductive_conductor_double_spin_box->value();
  obj["w_tol"] = ui->width_conductor_double_spin_box_2->value();
  obj["t_tol"] = ui->thickness_conductor_double_spin_box_2->value();
  obj["h_tol"] = ui->thickness_substrate_double_spin_box_2->value();
  return obj;
}

void MainWindow::loadSettingsFromJson(const QJsonObject &obj) {
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
  if (obj.contains("w_tol"))
    ui->width_conductor_double_spin_box_2->setValue(obj["w_tol"].toDouble());
  if (obj.contains("t_tol"))
    ui->thickness_conductor_double_spin_box_2->setValue(
        obj["t_tol"].toDouble());
  if (obj.contains("h_tol"))
    ui->thickness_substrate_double_spin_box_2->setValue(
        obj["h_tol"].toDouble());
}

void MainWindow::appendToTerminal(const QString &text) {
  ui->terminal_text_browser->append(
      QDateTime::currentDateTime().toString("[hh:mm:ss] ") + text);
}

void MainWindow::appendToOutput(const QString &text) {
  ui->output_text_browser->append(
      QDateTime::currentDateTime().toString("[hh:mm:ss] ") + text);
}

void MainWindow::setupCharts() {
  auto *totalFrame = ui->total_channel_frame;
  totalFrame->setLayout(new QVBoxLayout);
  totalPlot_ = new ComplexPlot(this);
  totalFrame->layout()->addWidget(totalPlot_);
  totalPlot_->setTitle("Годограф суммарного канала");

  auto *vertFrame = ui->verticy_channel_frame;
  vertFrame->setLayout(new QVBoxLayout);
  vertPlot_ = new ComplexPlot(this);
  vertFrame->layout()->addWidget(vertPlot_);
  vertPlot_->setTitle("Годограф вертикального канала");

  auto *horizFrame = ui->horizontal_channel_frame;
  horizFrame->setLayout(new QVBoxLayout);
  horizPlot_ = new ComplexPlot(this);
  horizFrame->layout()->addWidget(horizPlot_);
  horizPlot_->setTitle("Годограф горизонтального канала");

  auto *iFrame = ui->i_component_frame;
  iFrame->setLayout(new QVBoxLayout);
  iPlot_ = new ComplexPlot(this);
  iFrame->layout()->addWidget(iPlot_);
  iPlot_->setTitle("Синфазная составляющая (I)");
  iPlot_->setAxesLabels("Частота, ГГц", "I, отн. ед.");

  auto *qFrame = ui->q_component_frame;
  qFrame->setLayout(new QVBoxLayout);
  qPlot_ = new ComplexPlot(this);
  qFrame->layout()->addWidget(qPlot_);
  qPlot_->setTitle("Квадратурная составляющая (Q)");
  qPlot_->setAxesLabels("Частота, ГГц", "Q, отн. ед.");
}

void MainWindow::updateHodographs() {
  currentFreqs_.clear();
  for (int f = 1; f <= 10; ++f)
    currentFreqs_.push_back(f * 1e9);
  hodographTotal_.clear();
  hodographVert_.clear();
  hodographHoriz_.clear();

  double a = 0.001;
  for (double f : currentFreqs_) {
    double Z0 = fullModel_.calcZ0(f);
    std::complex<double> Zdef;
    double mag = currentDefectMagnitude_;
    switch (currentDefectClass_) {
    case 1:
      Zdef = {0.01 * mag, 2 * M_PI * f * 0.2e-9 * mag};
      break;
    case 2:
      Zdef = {0.0, -1.0 / (2 * M_PI * f * 0.1e-12 * mag + 1e-15)};
      break;
    case 3:
      Zdef = {0.0, -1.0 / (2 * M_PI * f * 0.15e-12 * mag + 1e-15)};
      break;
    case 4:
      Zdef = {0.0, -1.0 / (2 * M_PI * f * 0.12e-12 * mag + 1e-15)};
      break;
    default:
      Zdef = Z0;
    }
    std::complex<double> Gamma = (Zdef - Z0) / (Zdef + Z0);
    std::complex<double> Uinc(1.0, 0.0);
    double xd = 0.0;
    auto U1 = fullModel_.voltageAt(-a / 2, f, Uinc, Gamma, xd);
    auto U2 = fullModel_.voltageAt(a / 2, f, Uinc, Gamma, xd);
    auto U3 = U2;
    auto U4 = U1;
    std::complex<double> S = (U1 + U2 + U3 + U4) / 4.0;
    std::complex<double> Dx = ((U1 + U2) - (U3 + U4)) / 2.0;
    std::complex<double> Dy = ((U1 - U2) - (U3 - U4)) / 2.0;
    hodographTotal_.push_back(S);
    hodographVert_.push_back(Dx);
    hodographHoriz_.push_back(Dy);
  }
  totalPlot_->setData(hodographTotal_, false);
  vertPlot_->setData(hodographVert_, false);
  horizPlot_->setData(hodographHoriz_, false);
  appendToTerminal("Hodographs updated");
}

void MainWindow::updateQuadrature(int channel) {
  const std::vector<std::complex<double>> *data = nullptr;
  if (channel == 0)
    data = &hodographTotal_;
  else if (channel == 1)
    data = &hodographVert_;
  else
    data = &hodographHoriz_;
  if (!data || data->empty() || currentFreqs_.empty()) {
    appendToTerminal("No hodograph data. Run 'plot hodograph' first.");
    return;
  }
  std::vector<std::complex<double>> iPoints, qPoints;
  for (size_t idx = 0; idx < currentFreqs_.size(); ++idx) {
    double freqGHz = currentFreqs_[idx] / 1e9;
    double I = (*data)[idx].real();
    double Q = (*data)[idx].imag();
    iPoints.push_back({freqGHz, I});
    qPoints.push_back({freqGHz, Q});
  }
  iPlot_->setData(iPoints, true);
  qPlot_->setData(qPoints, true);
  appendToTerminal(QString("Quadrature for channel %1 updated").arg(channel));
}

// ========== Реализация новых слотов для UI ==========
void MainWindow::onClassifierSelectionChanged(int index) {
  QString name;
  switch (index) {
  case 0:
    name = "lr";
    break;
  case 1:
    name = "lda";
    break;
  case 2:
    name = "nb";
    break;
  default:
    name = "lr";
  }
  onSetClassifier(name);
  // В зависимости от выбранного классификатора делаем активными/неактивными
  // настройки
  bool isLR = (index == 0);
  learningRateSpin_->setEnabled(isLR);
  epochsSpin_->setEnabled(isLR);
  lambdaSpin_->setEnabled(isLR);
  if (!isLR) {
    // Для LDA и NB можно при желании установить другие гиперпараметры, но пока
    // просто игнорируем
    appendToTerminal("Для LDA и NB гиперпараметры не используются.");
  } else {
    // Принудительно устанавливаем параметры LR из текущих значений спинбоксов
    lr_.setHyperparameters(learningRateSpin_->value(), epochsSpin_->value(),
                           lambdaSpin_->value(), 32);
  }
}

void MainWindow::onGenerateDataClicked() { onGenerateData(); }

void MainWindow::onTrainClicked() { onTrainClassifier(); }

void MainWindow::onClassifyClicked() { onClassify(); }

void MainWindow::updateMetricsDisplay() {
  if (features_.empty() || labels_.empty()) {
    metricsTextEdit_->setText("Нет данных. Сначала сгенерируйте выборку.");
    return;
  }
  // Получаем предсказания и вычисляем метрики
  auto pred = classifier_->predict(features_);
  int nClasses = 5;
  std::vector<int> tp(nClasses, 0), fp(nClasses, 0), fn(nClasses, 0);
  for (size_t i = 0; i < pred.size(); ++i) {
    int t = labels_[i];
    int p = pred[i];
    if (p == t)
      tp[t]++;
    else {
      fp[p]++;
      fn[t]++;
    }
  }
  int correct = std::accumulate(tp.begin(), tp.end(), 0);
  double acc = 100.0 * correct / pred.size();

  QString html = "<b>Общая точность (Accuracy):</b> " +
                 QString::number(acc, 'f', 2) + "%<br><br>";
  html += "<table border='1' cellpadding='5' cellspacing='0'>";
  html += "<tr><th>Класс</th><th>Precision (%)</th><th>Recall "
          "(%)</th><th>F1-score (%)</th></tr>";
  for (int c = 0; c < nClasses; ++c) {
    double prec = (tp[c] + fp[c] == 0) ? 0 : 100.0 * tp[c] / (tp[c] + fp[c]);
    double rec = (tp[c] + fn[c] == 0) ? 0 : 100.0 * tp[c] / (tp[c] + fn[c]);
    double f1 = (prec + rec == 0) ? 0 : 2 * prec * rec / (prec + rec);
    html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                .arg(c)
                .arg(prec, 0, 'f', 1)
                .arg(rec, 0, 'f', 1)
                .arg(f1, 0, 'f', 1);
  }
  html += "</table>";
  metricsTextEdit_->setHtml(html);
}