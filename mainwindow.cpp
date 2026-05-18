#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils/complex_plot.h"
#include "utils/metrics.h"
#include "utils/sensitivity_analysis.h"
#include <QBarSeries>
#include <QBarSet>
#include <QCategoryAxis>
#include <QChartView>
#include <QDateTime>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTableWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QValueAxis>


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

  // Консольные команды
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

  // Настройка графиков и дополнительного UI
  setupCharts();
  setupAdditionalUi();

  // Настройка генератора и классификатора по умолчанию
  lr_.setHyperparameters(0.01, 500, 0.01, 32);
  generator_.setNoiseStd(0.05);
  bayes_.setNoiseStd(0.05);
  bayes_.setParameterGrid();

  // Автоматическое обновление годографов при изменении параметров дефекта через
  // UI (виджеты созданы в setupAdditionalUi, нужно сохранить указатели) Для
  // этого доработаем setupAdditionalUi, чтобы сохранить указатели на combobox и
  // spinbox

  // Теперь инициализируем отображение
  updateHodographs();
  updateQuadrature(0);
  ui->total_channel_radio_button->setChecked(true);

  appendToTerminal(
      "Program started. Commands: generate, train, classify, set "
      "classifier lr/lda/nb/bayes, plot hodograph, plot quadrature "
      "0/1/2, set defect class 0..4, set defect mag 0..1");
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupAdditionalUi() {
  // Очищаем frame и создаём новый UI
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
  noiseStdSpin_->setToolTip(
      "Стандартное отклонение аддитивного гауссовского шума");
  dataLayout->addRow("Стандартное отклонение шума:", noiseStdSpin_);

  samplesPerClassSpin_ = new QSpinBox(this);
  samplesPerClassSpin_->setRange(50, 1000);
  samplesPerClassSpin_->setSingleStep(50);
  samplesPerClassSpin_->setValue(200);
  samplesPerClassSpin_->setToolTip(
      "Количество синтетических образцов на каждый класс");
  dataLayout->addRow("Образцов на класс:", samplesPerClassSpin_);

  generateBtn_ = new QPushButton("Сгенерировать данные", this);
  generateBtn_->setToolTip("Сгенерировать новую синтетическую выборку");
  dataLayout->addRow(generateBtn_);

  loadCsvBtn_ = new QPushButton("Загрузить из CSV", this);
  loadCsvBtn_->setToolTip("Загрузить реальные данные из CSV файла");
  dataLayout->addRow(loadCsvBtn_);
  frameLayout->addWidget(dataGroup, 0, 0, 1, 1);

  // Группа настройки дефекта для годографов
  QGroupBox *defectGroup = new QGroupBox("Дефект для годографов", this);
  QFormLayout *defectLayout = new QFormLayout(defectGroup);

  QComboBox *defectClassCombo = new QComboBox(this);
  defectClassCombo->addItems({"Нет дефекта", "Утонение высоты проводника",
                              "Утонение ширины проводника", "Утонение подложки",
                              "Изменение диэлектрической проницаемости"});
  defectClassCombo->setCurrentIndex(currentDefectClass_);
  defectLayout->addRow("Тип дефекта:", defectClassCombo);

  QDoubleSpinBox *defectMagSpin = new QDoubleSpinBox(this);
  defectMagSpin->setRange(0.0, 1.0);
  defectMagSpin->setSingleStep(0.05);
  defectMagSpin->setValue(currentDefectMagnitude_);
  defectMagSpin->setToolTip("Величина дефекта (0..1)");
  defectLayout->addRow("Величина дефекта:", defectMagSpin);

  frameLayout->addWidget(defectGroup, 0, 1, 1, 1);

  // Подключение сигналов обновления годографов
  connect(defectClassCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          [this](int idx) {
            currentDefectClass_ = idx;
            updateHodographs();
          });
  connect(defectMagSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [this](double val) {
            currentDefectMagnitude_ = val;
            updateHodographs();
          });

  // Группа настройки классификатора
  QGroupBox *classifierGroup = new QGroupBox("Классификатор", this);
  QFormLayout *classifierLayout = new QFormLayout(classifierGroup);
  classifierCombo_ = new QComboBox(this);
  classifierCombo_->addItems({"Логистическая регрессия (Softmax)",
                              "Линейный дискриминантный анализ (LDA)",
                              "Наивный Байес (GaussianNB)",
                              "Байесовский с маргинализацией"});
  classifierLayout->addRow("Тип:", classifierCombo_);

  learningRateSpin_ = new QDoubleSpinBox(this);
  learningRateSpin_->setRange(1e-6, 1.0);
  learningRateSpin_->setDecimals(6);
  learningRateSpin_->setSingleStep(0.001);
  learningRateSpin_->setValue(0.01);
  learningRateSpin_->setToolTip(
      "Скорость обучения для логистической регрессии");
  classifierLayout->addRow("Скорость обучения (LR):", learningRateSpin_);

  epochsSpin_ = new QSpinBox(this);
  epochsSpin_->setRange(10, 2000);
  epochsSpin_->setSingleStep(50);
  epochsSpin_->setValue(500);
  epochsSpin_->setToolTip("Количество эпох обучения");
  classifierLayout->addRow("Количество эпох:", epochsSpin_);

  lambdaSpin_ = new QDoubleSpinBox(this);
  lambdaSpin_->setRange(0.0, 1.0);
  lambdaSpin_->setDecimals(6);
  lambdaSpin_->setSingleStep(0.01);
  lambdaSpin_->setValue(0.01);
  lambdaSpin_->setToolTip("Коэффициент L2-регуляризации");
  classifierLayout->addRow("Сила регуляризации (λ):", lambdaSpin_);

  frameLayout->addWidget(classifierGroup, 1, 0, 1, 2);

  // Кнопки управления
  QHBoxLayout *buttonLayout = new QHBoxLayout;
  trainBtn_ = new QPushButton("Обучить", this);
  trainBtn_->setToolTip("Обучить выбранный классификатор на текущих данных");
  classifyBtn_ = new QPushButton("Классифицировать", this);
  classifyBtn_->setToolTip("Выполнить классификацию и вывести метрики");
  exportMetricsBtn_ = new QPushButton("Экспорт метрик", this);
  exportMetricsBtn_->setToolTip("Сохранить метрики в текстовый файл");
  buttonLayout->addWidget(trainBtn_);
  buttonLayout->addWidget(classifyBtn_);
  buttonLayout->addWidget(exportMetricsBtn_);
  frameLayout->addLayout(buttonLayout, 2, 0, 1, 2);

  // Кнопки визуализации
  QHBoxLayout *vizLayout = new QHBoxLayout;
  plotConfusionBtn_ = new QPushButton("Матрица ошибок", this);
  plotConfusionBtn_->setToolTip("Показать тепловую карту матрицы ошибок");
  plotRocBtn_ = new QPushButton("ROC-кривые", this);
  plotRocBtn_->setToolTip("Показать ROC-кривые для каждого класса");
  sensitivityBtn_ = new QPushButton("Анализ чувствительности", this);
  sensitivityBtn_->setToolTip(
      "Вычислить и показать частные производные волнового сопротивления");
  vizLayout->addWidget(plotConfusionBtn_);
  vizLayout->addWidget(plotRocBtn_);
  vizLayout->addWidget(sensitivityBtn_);
  frameLayout->addLayout(vizLayout, 3, 0, 1, 2);

  // Вывод метрик
  QGroupBox *metricsGroup = new QGroupBox("Метрики качества", this);
  QVBoxLayout *metricsLayout = new QVBoxLayout(metricsGroup);
  metricsTextEdit_ = new QTextEdit(this);
  metricsTextEdit_->setReadOnly(true);
  metricsTextEdit_->setMinimumHeight(200);
  metricsLayout->addWidget(metricsTextEdit_);
  frameLayout->addWidget(metricsGroup, 4, 0, 1, 2);

  // Подключение сигналов
  connect(generateBtn_, &QPushButton::clicked, this,
          &MainWindow::onGenerateDataClicked);
  connect(loadCsvBtn_, &QPushButton::clicked, this,
          &MainWindow::onLoadCsvClicked);
  connect(trainBtn_, &QPushButton::clicked, this, &MainWindow::onTrainClicked);
  connect(classifyBtn_, &QPushButton::clicked, this,
          &MainWindow::onClassifyClicked);
  connect(exportMetricsBtn_, &QPushButton::clicked, this,
          &MainWindow::onExportMetricsClicked);
  connect(plotConfusionBtn_, &QPushButton::clicked, this,
          &MainWindow::onPlotConfusionMatrix);
  connect(plotRocBtn_, &QPushButton::clicked, this,
          &MainWindow::onPlotRocCurves);
  connect(sensitivityBtn_, &QPushButton::clicked, this,
          &MainWindow::onSensitivityAnalysis);
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
            bayes_.setNoiseStd(val);
            appendToTerminal(QString("Уровень шума установлен: %1").arg(val));
          });

  onClassifierSelectionChanged(0);
}

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
  case 3:
    name = "bayes";
    break;
  default:
    name = "lr";
  }
  onSetClassifier(name);
  bool isLR = (index == 0);
  learningRateSpin_->setEnabled(isLR);
  epochsSpin_->setEnabled(isLR);
  lambdaSpin_->setEnabled(isLR);
  if (!isLR) {
    appendToTerminal(
        "Для выбранного классификатора гиперпараметры не используются.");
  } else {
    lr_.setHyperparameters(learningRateSpin_->value(), epochsSpin_->value(),
                           lambdaSpin_->value(), 32);
  }
}

void MainWindow::onGenerateDataClicked() { onGenerateData(); }

void MainWindow::onLoadCsvClicked() {
  QString fn = QFileDialog::getOpenFileName(this, "Загрузить CSV", "", "*.csv");
  if (fn.isEmpty())
    return;
  try {
    dataLoader_.loadCSV(fn.toStdString(), features_, labels_);
    appendToTerminal(
        QString("Загружено %1 образцов из %2").arg(features_.size()).arg(fn));
    logDetailedDataInfo();
    appendToOutput("Данные загружены из CSV. Можно обучать классификатор.");
  } catch (const std::exception &e) {
    appendToTerminal(QString("Ошибка загрузки CSV: %1").arg(e.what()));
  }
}

void MainWindow::onTrainClicked() { onTrainClassifier(); }

void MainWindow::onClassifyClicked() { onClassify(); }

void MainWindow::onExportMetricsClicked() {
  if (features_.empty() || labels_.empty()) {
    appendToTerminal("Нет данных. Сначала сгенерируйте или загрузите выборку.");
    return;
  }
  auto pred = classifier_->predict(features_);
  QString metricsStr = Metrics::formatMetrics(pred, labels_);
  QString fn =
      QFileDialog::getSaveFileName(this, "Сохранить метрики", "", "*.txt");
  if (!fn.isEmpty()) {
    QFile file(fn);
    if (file.open(QIODevice::WriteOnly)) {
      file.write(metricsStr.toUtf8());
      appendToTerminal("Метрики сохранены в " + fn);
    } else {
      appendToTerminal("Ошибка сохранения файла");
    }
  }
}

void MainWindow::onPlotConfusionMatrix() {
  if (features_.empty() || labels_.empty()) {
    appendToTerminal("Нет данных для построения матрицы ошибок.");
    return;
  }
  auto pred = classifier_->predict(features_);
  auto cm = Metrics::confusionMatrix(pred, labels_, 5);
  QDialog *dialog = new QDialog(this);
  dialog->setWindowTitle("Матрица ошибок");
  QChartView *chartView = Metrics::plotConfusionMatrix(cm, 5);
  QVBoxLayout *layout = new QVBoxLayout(dialog);
  layout->addWidget(chartView);
  dialog->resize(600, 500);
  dialog->exec();
}

void MainWindow::onPlotRocCurves() {
  if (features_.empty() || labels_.empty()) {
    appendToTerminal("Нет данных для построения ROC-кривых.");
    return;
  }
  std::vector<std::vector<double>> probs;
  if (currentClassifierName_ == "lr") {
    probs = lr_.predictProbabilities(features_);
  } else if (currentClassifierName_ == "bayes") {
    probs = bayes_.predictProbabilities(features_);
  } else {
    appendToTerminal("ROC-кривые доступны только для логистической регрессии и "
                     "байесовского классификатора.");
    return;
  }
  QDialog *dialog = new QDialog(this);
  dialog->setWindowTitle("ROC-кривые");
  QChartView *chartView = Metrics::plotRocCurves(probs, labels_, 5);
  QVBoxLayout *layout = new QVBoxLayout(dialog);
  layout->addWidget(chartView);
  dialog->resize(800, 600);
  dialog->exec();
}

void MainWindow::onSensitivityAnalysis() {
  double w = ui->width_conductor_double_spin_box->value();
  double t = ui->thickness_conductor_double_spin_box->value();
  double h = ui->thickness_substrate_double_spin_box->value();
  double er = ui->dielectric_permittion_substrate_double_spin_box->value();
  auto sens = SensitivityAnalysis::computeSensitivities(w, t, h, er);
  QString msg = QString("Частные производные волнового сопротивления:\n"
                        "∂Z0/∂W = %1 Ом/м\n∂Z0/∂h = %2 Ом/м\n∂Z0/∂εr = %3 Ом")
                    .arg(sens.dZdW, 0, 'e', 4)
                    .arg(sens.dZdh, 0, 'e', 4)
                    .arg(sens.dZder, 0, 'e', 4);
  appendToTerminal(msg);
  QMessageBox::information(this, "Анализ чувствительности", msg);
}

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
  updateHodographs(); // автообновление годографов при изменении параметров
                      // линии
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
  logDetailedDataInfo();
  appendToOutput("Data generated. Now you can train a classifier.");
}

void MainWindow::logDetailedDataInfo() {
  if (features_.empty())
    return;
  int nClasses = 5;
  std::vector<int> classCounts(nClasses, 0);
  for (int l : labels_)
    classCounts[l]++;
  appendToTerminal("=== Detailed data info ===");
  appendToTerminal(QString("Total samples: %1").arg(features_.size()));
  appendToTerminal(QString("Feature dimension: %1").arg(features_[0].size()));
  for (int c = 0; c < nClasses; ++c) {
    appendToTerminal(
        QString("Class %1: %2 samples").arg(c).arg(classCounts[c]));
  }
  double sum = 0.0, sum2 = 0.0;
  for (const auto &feat : features_) {
    sum += feat[0];
    sum2 += feat[0] * feat[0];
  }
  double mean = sum / features_.size();
  double var = sum2 / features_.size() - mean * mean;
  appendToTerminal(QString("Feature0 mean = %1, variance = %2")
                       .arg(mean, 0, 'e', 4)
                       .arg(var, 0, 'e', 4));
  appendToTerminal("===========================");
}

void MainWindow::onTrainClassifier() {
  if (features_.empty()) {
    appendToTerminal("No data. Run 'generate' or load CSV first.");
    return;
  }
  appendToTerminal(QString("Training classifier: %1...")
                       .arg(QString::fromStdString(currentClassifierName_)));
  classifier_->train(features_, labels_);
  appendToTerminal("Training finished.");
}

void MainWindow::onClassify() {
  if (features_.empty()) {
    appendToTerminal("No data. Run 'generate' or load CSV first.");
    return;
  }
  appendToTerminal("Classifying...");
  auto pred = classifier_->predict(features_);
  printMetrics(pred, labels_);
  updateMetricsDisplay();
}

void MainWindow::printMetrics(const std::vector<int> &pred,
                              const std::vector<int> &trueLabels) {
  auto metrics = Metrics::computeAll(pred, trueLabels, 5);
  appendToTerminal(
      QString("Overall accuracy: %1%").arg(metrics.accuracy, 0, 'f', 2));
  for (int c = 0; c < 5; ++c) {
    appendToTerminal(QString("Class %1: P=%.1f%% R=%.1f%% F1=%.1f%%")
                         .arg(c)
                         .arg(metrics.precision[c], 0, 'f', 1)
                         .arg(metrics.recall[c], 0, 'f', 1)
                         .arg(metrics.f1[c], 0, 'f', 1));
  }
  appendToTerminal(QString("Macro F1 = %1%").arg(metrics.macroF1, 0, 'f', 2));
}

void MainWindow::updateMetricsDisplay() {
  if (features_.empty() || labels_.empty()) {
    metricsTextEdit_->setText(
        "Нет данных. Сначала сгенерируйте или загрузите выборку.");
    return;
  }
  auto pred = classifier_->predict(features_);
  QString html = Metrics::formatHtmlMetrics(pred, labels_);
  metricsTextEdit_->setHtml(html);
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
  } else if (name == "bayes") {
    classifier_ = &bayes_;
    currentClassifierName_ = "bayes";
    appendToTerminal("Classifier set to Bayesian with marginalization");
  } else {
    appendToTerminal("Unknown classifier. Use lr, lda, nb, bayes");
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
  if (totalFrame->layout())
    delete totalFrame->layout();
  totalFrame->setLayout(new QVBoxLayout);
  totalPlot_ = new ComplexPlot(this);
  totalFrame->layout()->addWidget(totalPlot_);
  totalPlot_->setTitle("Годограф суммарного канала");

  auto *vertFrame = ui->verticy_channel_frame;
  if (vertFrame->layout())
    delete vertFrame->layout();
  vertFrame->setLayout(new QVBoxLayout);
  vertPlot_ = new ComplexPlot(this);
  vertFrame->layout()->addWidget(vertPlot_);
  vertPlot_->setTitle("Годограф вертикального канала");

  auto *horizFrame = ui->horizontal_channel_frame;
  if (horizFrame->layout())
    delete horizFrame->layout();
  horizFrame->setLayout(new QVBoxLayout);
  horizPlot_ = new ComplexPlot(this);
  horizFrame->layout()->addWidget(horizPlot_);
  horizPlot_->setTitle("Годограф горизонтального канала");

  auto *iFrame = ui->i_component_frame;
  if (iFrame->layout())
    delete iFrame->layout();
  iFrame->setLayout(new QVBoxLayout);
  iPlot_ = new ComplexPlot(this);
  iFrame->layout()->addWidget(iPlot_);
  iPlot_->setTitle("Синфазная составляющая (I)");
  iPlot_->setAxesLabels("Частота, ГГц", "I, отн. ед.");

  auto *qFrame = ui->q_component_frame;
  if (qFrame->layout())
    delete qFrame->layout();
  qFrame->setLayout(new QVBoxLayout);
  qPlot_ = new ComplexPlot(this);
  qFrame->layout()->addWidget(qPlot_);
  qPlot_->setTitle("Квадратурная составляющая (Q)");
  qPlot_->setAxesLabels("Частота, ГГц", "Q, отн. ед.");
}

void MainWindow::updateHodographs() {
  // Защита от вызова до полной инициализации графиков
  if (!totalPlot_ || !vertPlot_ || !horizPlot_)
    return;

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

  // Автоматически обновить квадратурные составляющие для текущего канала
  if (ui->total_channel_radio_button->isChecked())
    updateQuadrature(0);
  else if (ui->verticy_channel_radio_button->isChecked())
    updateQuadrature(1);
  else if (ui->horizontal_channel_radio_button->isChecked())
    updateQuadrature(2);
}

void MainWindow::updateQuadrature(int channel) {
  // Защита от вызова до инициализации
  if (!iPlot_ || !qPlot_)
    return;

  const std::vector<std::complex<double>> *data = nullptr;
  if (channel == 0)
    data = &hodographTotal_;
  else if (channel == 1)
    data = &hodographVert_;
  else
    data = &hodographHoriz_;
  if (!data || data->empty() || currentFreqs_.empty()) {
    // Не выводим сообщение в терминал при автоматическом обновлении, чтобы не
    // засорять
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
}