#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils/complex_plot.h"
#include "utils/experiment_logger.h"
#include "utils/feature_importance.h"
#include "utils/fixed_point_export.h"
#include "utils/hyperparameter_tuner.h"
#include "utils/metrics.h"
#include "utils/model_serializer.h"
#include "utils/pca.h"
#include "utils/sensitivity_analysis.h"
#include "utils/whitening.h"

#include <QBarSeries>
#include <QBarSet>
#include <QCategoryAxis>
#include <QChartView>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QScatterSeries>
#include <QTableWidget> // Добавлено для таблицы журнала экспериментов
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QValueAxis>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), generator_(calculator_),
      classifier_(&lr_), currentClassifierName_("lr") {
  ui->setupUi(this);

  ui->width_conductor_double_spin_box->setMinimum(1e-6);
  ui->thickness_conductor_double_spin_box->setMinimum(1e-6);
  ui->thickness_substrate_double_spin_box->setMinimum(1e-6);
  ui->dielectric_permittion_substrate_double_spin_box->setMinimum(1.0);
  ui->tangence_ougla_electric_loss_double_spin_box->setMinimum(0.0);
  ui->conductive_conductor_double_spin_box->setMinimum(1e3);
  ui->width_conductor_double_spin_box_2->setMinimum(0.0);
  ui->thickness_conductor_double_spin_box_2->setMinimum(0.0);
  ui->thickness_substrate_double_spin_box_2->setMinimum(0.0);

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
    else if (cmd.startsWith("set classifier "))
      onSetClassifier(cmd.mid(15).trimmed());
    else if (cmd == "plot hodograph")
      updateHodographs();
    else if (cmd.startsWith("plot quadrature "))
      updateQuadrature(cmd.mid(16).trimmed().toInt());
    else if (!cmd.isEmpty())
      appendToTerminal("Unknown command");
    ui->input_line_edit->clear();
  });

  onParametersChanged();
  onToleranceChanged();
  setupCharts();
  setupAdditionalUi();
  ExperimentLogger::initDatabase("experiments.db");

  lr_.setHyperparameters(0.01, 500, 0.01, 32);
  generator_.setNoiseStd(0.05);
  bayes_.setNoiseStd(0.05);
  bayes_.setParameterGrid();

  updateFreqsFromUi();
  updateHodographs();
  updateQuadrature(0);
  ui->total_channel_radio_button->setChecked(true);

  appendToTerminal("Program started with frequency sweep, AUC, progress bar, "
                   "loss matrix and PCA.");
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupAdditionalUi() {
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

  // Группа генерации данных
  QGroupBox *dataGroup = new QGroupBox("Генерация данных", this);
  QFormLayout *dataLayout = new QFormLayout(dataGroup);
  noiseStdSpin_ = new QDoubleSpinBox(this);
  noiseStdSpin_->setRange(0.0, 0.5);
  noiseStdSpin_->setSingleStep(0.01);
  noiseStdSpin_->setDecimals(3);
  noiseStdSpin_->setValue(0.05);
  dataLayout->addRow("Шум (σ):", noiseStdSpin_);
  samplesPerClassSpin_ = new QSpinBox(this);
  samplesPerClassSpin_->setRange(50, 1000);
  samplesPerClassSpin_->setSingleStep(50);
  samplesPerClassSpin_->setValue(200);
  dataLayout->addRow("Образцов на класс:", samplesPerClassSpin_);

  quantizeCheckBox_ = new QCheckBox("Квантование АЦП", this);
  dataLayout->addRow(quantizeCheckBox_);
  adcBitsSpinBox_ = new QSpinBox(this);
  adcBitsSpinBox_->setRange(8, 24);
  adcBitsSpinBox_->setValue(12);
  adcBitsSpinBox_->setEnabled(false);
  dataLayout->addRow("Разрядность (бит):", adcBitsSpinBox_);
  rawAdcCheckBox_ = new QCheckBox("Сырые коды АЦП", this);
  rawAdcCheckBox_->setEnabled(false);
  dataLayout->addRow(rawAdcCheckBox_);
  whitenCheckBox_ = new QCheckBox("Отбеливание признаков", this);
  dataLayout->addRow(whitenCheckBox_);
  augmentCheckBox_ = new QCheckBox("Аугментация данных (допуски)", this);
  dataLayout->addRow(augmentCheckBox_);

  connect(quantizeCheckBox_, &QCheckBox::toggled, adcBitsSpinBox_,
          &QSpinBox::setEnabled);
  connect(quantizeCheckBox_, &QCheckBox::toggled, rawAdcCheckBox_,
          &QCheckBox::setEnabled);

  generateBtn_ = new QPushButton("Сгенерировать данные");
  dataLayout->addRow(generateBtn_);
  loadCsvBtn_ = new QPushButton("Загрузить из CSV");
  dataLayout->addRow(loadCsvBtn_);
  frameLayout->addWidget(dataGroup, 0, 0, 1, 1);

  // Частоты зондирования
  QGroupBox *freqGroup = new QGroupBox("Частоты зондирования", this);
  QFormLayout *freqLayout = new QFormLayout(freqGroup);
  freqStartSpin_ = new QDoubleSpinBox(this);
  freqStartSpin_->setRange(0.1, 100.0);
  freqStartSpin_->setValue(1.0);
  freqStartSpin_->setSuffix(" ГГц");
  freqLayout->addRow("Начальная:", freqStartSpin_);
  freqStopSpin_ = new QDoubleSpinBox(this);
  freqStopSpin_->setRange(0.1, 100.0);
  freqStopSpin_->setValue(10.0);
  freqStopSpin_->setSuffix(" ГГц");
  freqLayout->addRow("Конечная:", freqStopSpin_);
  freqPointsSpin_ = new QSpinBox(this);
  freqPointsSpin_->setRange(2, 50);
  freqPointsSpin_->setValue(10);
  freqLayout->addRow("Количество точек:", freqPointsSpin_);
  frameLayout->addWidget(freqGroup, 0, 1, 1, 1);

  // Дефект для визуализации годографов
  QGroupBox *defectGroup = new QGroupBox("Дефект для годографов", this);
  QFormLayout *defectLayout = new QFormLayout(defectGroup);
  QComboBox *defectClassCombo = new QComboBox(this);
  defectClassCombo->addItems({"Нет дефекта", "Утонение высоты",
                              "Утонение ширины", "Утонение подложки",
                              "Изменение εr"});
  defectClassCombo->setCurrentIndex(currentDefectClass_);
  defectLayout->addRow("Тип:", defectClassCombo);
  QDoubleSpinBox *defectMagSpin = new QDoubleSpinBox(this);
  defectMagSpin->setRange(0.0, 1.0);
  defectMagSpin->setSingleStep(0.05);
  defectMagSpin->setValue(currentDefectMagnitude_);
  defectLayout->addRow("Величина:", defectMagSpin);
  frameLayout->addWidget(defectGroup, 0, 2, 1, 1);
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

  // Классификатор
  QGroupBox *classifierGroup = new QGroupBox("Классификатор", this);
  QFormLayout *classifierLayout = new QFormLayout(classifierGroup);
  classifierCombo_ = new QComboBox(this);
  classifierCombo_->addItems(
      {"Логистическая регрессия", "LDA", "Наивный Байес", "Байесовский"});
  classifierLayout->addRow("Тип:", classifierCombo_);
  learningRateSpin_ = new QDoubleSpinBox(this);
  learningRateSpin_->setRange(1e-6, 1.0);
  learningRateSpin_->setDecimals(6);
  learningRateSpin_->setValue(0.01);
  classifierLayout->addRow("Скорость обучения:", learningRateSpin_);
  epochsSpin_ = new QSpinBox(this);
  epochsSpin_->setRange(10, 2000);
  epochsSpin_->setValue(500);
  classifierLayout->addRow("Эпохи:", epochsSpin_);
  lambdaSpin_ = new QDoubleSpinBox(this);
  lambdaSpin_->setRange(0.0, 1.0);
  lambdaSpin_->setValue(0.01);
  classifierLayout->addRow("Регуляризация λ:", lambdaSpin_);
  frameLayout->addWidget(classifierGroup, 1, 0, 1, 2);

  // Кнопки действий
  QHBoxLayout *buttonLayout = new QHBoxLayout;
  trainBtn_ = new QPushButton("Обучить");
  classifyBtn_ = new QPushButton("Классифицировать");
  classifyLossBtn_ = new QPushButton("Классифицировать с матрицей потерь");
  exportMetricsBtn_ = new QPushButton("Экспорт метрик");
  buttonLayout->addWidget(trainBtn_);
  buttonLayout->addWidget(classifyBtn_);
  buttonLayout->addWidget(classifyLossBtn_);
  buttonLayout->addWidget(exportMetricsBtn_);
  frameLayout->addLayout(buttonLayout, 2, 0, 1, 3);
  QPushButton *saveModelBtn = new QPushButton("Сохранить модель");
  QPushButton *loadModelBtn = new QPushButton("Загрузить модель");
  QPushButton *tuneBtn = new QPushButton("Подбор гиперпараметров");
  QPushButton *featureImportanceBtn = new QPushButton("Важность признаков");
  QPushButton *exportReportBtn = new QPushButton("Экспорт отчёта");
  QPushButton *exportQ15Btn = new QPushButton("Экспорт Q15");
  QPushButton *viewLogBtn = new QPushButton("Журнал экспериментов");
  buttonLayout->addWidget(saveModelBtn);
  buttonLayout->addWidget(loadModelBtn);
  buttonLayout->addWidget(tuneBtn);
  buttonLayout->addWidget(featureImportanceBtn);
  buttonLayout->addWidget(exportReportBtn);
  buttonLayout->addWidget(exportQ15Btn);
  buttonLayout->addWidget(viewLogBtn);

  connect(saveModelBtn, &QPushButton::clicked, this, &MainWindow::onSaveModel);
  connect(loadModelBtn, &QPushButton::clicked, this, &MainWindow::onLoadModel);
  connect(tuneBtn, &QPushButton::clicked, this, &MainWindow::onTuneHyperparams);
  connect(featureImportanceBtn, &QPushButton::clicked, this,
          &MainWindow::onPlotFeatureImportance);
  connect(exportReportBtn, &QPushButton::clicked, this,
          &MainWindow::onExportReport);
  connect(exportQ15Btn, &QPushButton::clicked, this, &MainWindow::onExportQ15);
  connect(viewLogBtn, &QPushButton::clicked, this,
          &MainWindow::onViewExperiments);

  // Визуализация
  QHBoxLayout *vizLayout = new QHBoxLayout;
  plotConfusionBtn_ = new QPushButton("Матрица ошибок");
  plotRocBtn_ = new QPushButton("ROC-кривые");
  sensitivityBtn_ = new QPushButton("Анализ чувствительности");
  pcaBtn_ = new QPushButton("PCA проекция");
  vizLayout->addWidget(plotConfusionBtn_);
  vizLayout->addWidget(plotRocBtn_);
  vizLayout->addWidget(sensitivityBtn_);
  vizLayout->addWidget(pcaBtn_);
  frameLayout->addLayout(vizLayout, 3, 0, 1, 3);

  // Метрики качества
  QGroupBox *metricsGroup = new QGroupBox("Метрики качества", this);
  QVBoxLayout *metricsLayout = new QVBoxLayout(metricsGroup);
  metricsTextEdit_ = new QTextEdit(this);
  metricsTextEdit_->setReadOnly(true);
  metricsTextEdit_->setMinimumHeight(200);
  metricsLayout->addWidget(metricsTextEdit_);
  frameLayout->addWidget(metricsGroup, 4, 0, 1, 3);

  // Соединения
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
          });
  connect(epochsSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
          [this](int val) {
            if (currentClassifierName_ == "lr")
              lr_.setHyperparameters(learningRateSpin_->value(), val,
                                     lambdaSpin_->value(), 32);
          });
  connect(lambdaSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [this](double val) {
            if (currentClassifierName_ == "lr")
              lr_.setHyperparameters(learningRateSpin_->value(),
                                     epochsSpin_->value(), val, 32);
          });
  connect(noiseStdSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [this](double val) {
            generator_.setNoiseStd(val);
            bayes_.setNoiseStd(val);
          });
  connect(freqStartSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onFreqParamsChanged);
  connect(freqStopSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onFreqParamsChanged);
  connect(freqPointsSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &MainWindow::onFreqParamsChanged);
  connect(pcaBtn_, &QPushButton::clicked, this, &MainWindow::onPlotPCA);
  connect(classifyLossBtn_, &QPushButton::clicked, this,
          &MainWindow::onClassifyWithLoss);

  onClassifierSelectionChanged(0);
}

void MainWindow::onFreqParamsChanged() {
  updateFreqsFromUi();
  updateHodographs();
}

void MainWindow::updateFreqsFromUi() {
  double start = freqStartSpin_->value();
  double stop = freqStopSpin_->value();
  int points = freqPointsSpin_->value();
  freqs_.clear();
  for (int i = 0; i < points; ++i) {
    double f = start + (stop - start) * i / (points - 1);
    freqs_.push_back(f * 1e9);
  }
  appendToTerminal(QString("Частоты обновлены: %1 точек от %2 до %3 ГГц")
                       .arg(points)
                       .arg(start)
                       .arg(stop));
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
}

void MainWindow::onGenerateDataClicked() {
  bool quantize = quantizeCheckBox_->isChecked();
  int bits = adcBitsSpinBox_->value();
  bool rawMode = rawAdcCheckBox_->isChecked();
  generator_.setQuantization(quantize ? bits : -1, rawMode);
  generator_.setVariationEnabled(augmentCheckBox_->isChecked());
  double w_tol = ui->width_conductor_double_spin_box_2->value();
  double t_tol = ui->thickness_conductor_double_spin_box_2->value();
  double h_tol = ui->thickness_substrate_double_spin_box_2->value();
  generator_.setTolerances(w_tol, t_tol, h_tol);
  onGenerateData();
}

void MainWindow::onLoadCsvClicked() {
  QString fn = QFileDialog::getOpenFileName(this, "Загрузить CSV", "", "*.csv");
  if (fn.isEmpty())
    return;
  try {
    dataLoader_.loadCSV(fn.toStdString(), features_, labels_);
    appendToTerminal(
        QString("Загружено %1 образцов из %2").arg(features_.size()).arg(fn));
    logDetailedDataInfo();
    appendToOutput("Данные загружены.");
  } catch (const std::exception &e) {
    appendToTerminal(QString("Ошибка: %1").arg(e.what()));
  }
}

void MainWindow::onTrainClicked() {
  if (features_.empty()) {
    appendToTerminal("Нет данных");
    return;
  }
  bool doWhiten = whitenCheckBox_->isChecked();
  std::vector<std::vector<double>> trainX = features_;
  std::vector<int> trainY = labels_;
  if (doWhiten) {
    currentWhitening_.fit(trainX);
    trainX = currentWhitening_.transform(trainX);
    appendToTerminal("Применено отбеливание признаков");
  }
  appendToTerminal("Обучение...");
  ui->process_progress_bar->setRange(0, 100);
  ui->process_label->setText("Обучение...");
  // Имитация прогресса
  for (int i = 0; i <= 100; i += 10) {
    ui->process_progress_bar->setValue(i);
    QCoreApplication::processEvents();
  }
  classifier_->train(trainX, trainY);
  ui->process_progress_bar->setValue(100);
  QCoreApplication::processEvents();
  ui->process_progress_bar->setValue(0);
  ui->process_label->setText("Процесс");
  appendToTerminal("Обучение завершено.");
}

void MainWindow::onClassifyClicked() {
  if (features_.empty()) {
    appendToTerminal("Нет данных");
    return;
  }
  std::vector<std::vector<double>> testX = features_;
  if (currentWhitening_.isFitted()) {
    testX = currentWhitening_.transform(testX);
  }
  auto pred = classifier_->predict(testX);
  printMetricsWithAUC(pred, labels_);
  updateMetricsDisplay();
}

void MainWindow::printMetricsWithAUC(const std::vector<int> &pred,
                                     const std::vector<int> &trueLabels) {
  auto metrics = Metrics::computeAll(pred, trueLabels, 5);
  std::vector<std::vector<double>> probs;
  if (currentClassifierName_ == "lr")
    probs = lr_.predictProbabilities(features_);
  else if (currentClassifierName_ == "bayes")
    probs = bayes_.predictProbabilities(features_);
  if (!probs.empty()) {
    metrics.auc = Metrics::computeAllAUC(probs, trueLabels, 5);
  } else {
    metrics.auc.assign(5, 0.5);
  }
  appendToTerminal(QString("Accuracy: %1%").arg(metrics.accuracy, 0, 'f', 2));
  for (int c = 0; c < 5; ++c) {
    appendToTerminal(QString("Class %1: P=%.1f%% R=%.1f%% F1=%.1f%% AUC=%.3f")
                         .arg(c)
                         .arg(metrics.precision[c])
                         .arg(metrics.recall[c])
                         .arg(metrics.f1[c])
                         .arg(metrics.auc[c]));
  }
  appendToTerminal(QString("Macro F1 = %1%").arg(metrics.macroF1, 0, 'f', 2));
  double macroAUC =
      std::accumulate(metrics.auc.begin(), metrics.auc.end(), 0.0) /
      metrics.auc.size();
  appendToTerminal(QString("Macro AUC = %1").arg(macroAUC, 0, 'f', 3));
}

void MainWindow::updateMetricsDisplay() {
  if (features_.empty())
    return;
  auto pred = classifier_->predict(features_);
  auto metrics = Metrics::computeAll(pred, labels_, 5);
  std::vector<std::vector<double>> probs;
  if (currentClassifierName_ == "lr")
    probs = lr_.predictProbabilities(features_);
  else if (currentClassifierName_ == "bayes")
    probs = bayes_.predictProbabilities(features_);
  if (!probs.empty())
    metrics.auc = Metrics::computeAllAUC(probs, labels_, 5);
  else
    metrics.auc.assign(5, 0.5);
  QString html = "<b>Общая точность:</b> " +
                 QString::number(metrics.accuracy, 'f', 2) + "%<br><br>";
  html += "<table border=1>";
  html += "<tr><th>Класс</th><th>Precision (%)</th><th>Recall (%)</th><th>F1 "
          "(%)</th><th>AUC</th></tr>";
  for (int c = 0; c < 5; ++c) {
    html +=
        QString(
            "<td><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
            .arg(c)
            .arg(metrics.precision[c], 0, 'f', 1)
            .arg(metrics.recall[c], 0, 'f', 1)
            .arg(metrics.f1[c], 0, 'f', 1)
            .arg(metrics.auc[c], 0, 'f', 3);
  }
  html += "<tr><br>";
  double macroAUC =
      std::accumulate(metrics.auc.begin(), metrics.auc.end(), 0.0) /
      metrics.auc.size();
  html += QString("<b>Macro F1 = %1%</b><br><b>Macro AUC = %2</b>")
              .arg(metrics.macroF1, 0, 'f', 2)
              .arg(macroAUC, 0, 'f', 3);
  metricsTextEdit_->setHtml(html);
}

void MainWindow::onSetClassifier(const QString &name) {
  if (name == "lr") {
    classifier_ = &lr_;
    currentClassifierName_ = "lr";
  } else if (name == "lda") {
    classifier_ = &lda_;
    currentClassifierName_ = "lda";
  } else if (name == "nb") {
    classifier_ = &nb_;
    currentClassifierName_ = "nb";
  } else if (name == "bayes") {
    classifier_ = &bayes_;
    currentClassifierName_ = "bayes";
  } else
    return;
  appendToTerminal("Classifier set to " + name);
}

void MainWindow::onSaveConfiguration() {
  QString fn = QFileDialog::getSaveFileName(this, "Save config", "", "*.json");
  if (fn.isEmpty())
    return;
  QFile f(fn);
  if (!f.open(QIODevice::WriteOnly)) {
    appendToTerminal("Cannot save");
    return;
  }
  f.write(QJsonDocument(saveSettingsToJson()).toJson());
  appendToTerminal("Saved");
}

void MainWindow::onLoadConfiguration() {
  QString fn = QFileDialog::getOpenFileName(this, "Load config", "", "*.json");
  if (fn.isEmpty())
    return;
  QFile f(fn);
  if (!f.open(QIODevice::ReadOnly)) {
    appendToTerminal("Cannot open");
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
  appendToTerminal("Loaded");
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
  obj["freq_start"] = freqStartSpin_->value();
  obj["freq_stop"] = freqStopSpin_->value();
  obj["freq_points"] = freqPointsSpin_->value();
  obj["noise_std"] = noiseStdSpin_->value();
  obj["samples_per_class"] = samplesPerClassSpin_->value();
  obj["classifier"] = classifierCombo_->currentIndex();
  obj["quantize"] = quantizeCheckBox_->isChecked();
  obj["adc_bits"] = adcBitsSpinBox_->value();
  obj["raw_adc"] = rawAdcCheckBox_->isChecked();
  obj["whiten"] = whitenCheckBox_->isChecked();
  obj["augment"] = augmentCheckBox_->isChecked();
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
  if (obj.contains("freq_start"))
    freqStartSpin_->setValue(obj["freq_start"].toDouble());
  if (obj.contains("freq_stop"))
    freqStopSpin_->setValue(obj["freq_stop"].toDouble());
  if (obj.contains("freq_points"))
    freqPointsSpin_->setValue(obj["freq_points"].toInt());
  if (obj.contains("noise_std"))
    noiseStdSpin_->setValue(obj["noise_std"].toDouble());
  if (obj.contains("samples_per_class"))
    samplesPerClassSpin_->setValue(obj["samples_per_class"].toInt());
  if (obj.contains("classifier"))
    classifierCombo_->setCurrentIndex(obj["classifier"].toInt());
  if (obj.contains("quantize"))
    quantizeCheckBox_->setChecked(obj["quantize"].toBool());
  if (obj.contains("adc_bits"))
    adcBitsSpinBox_->setValue(obj["adc_bits"].toInt());
  if (obj.contains("raw_adc"))
    rawAdcCheckBox_->setChecked(obj["raw_adc"].toBool());
  if (obj.contains("whiten"))
    whitenCheckBox_->setChecked(obj["whiten"].toBool());
  if (obj.contains("augment"))
    augmentCheckBox_->setChecked(obj["augment"].toBool());
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
  if (!totalPlot_ || !vertPlot_ || !horizPlot_)
    return;
  if (freqs_.empty())
    return;
  hodographTotal_.clear();
  hodographVert_.clear();
  hodographHoriz_.clear();
  double a = 0.001;
  for (double f : freqs_) {
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
    std::complex<double> Gamma =
        (currentDefectClass_ == 0) ? 0.0 : (Zdef - Z0) / (Zdef + Z0);
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
  if (ui->total_channel_radio_button->isChecked())
    updateQuadrature(0);
  else if (ui->verticy_channel_radio_button->isChecked())
    updateQuadrature(1);
  else if (ui->horizontal_channel_radio_button->isChecked())
    updateQuadrature(2);
}

void MainWindow::updateQuadrature(int channel) {
  if (!iPlot_ || !qPlot_)
    return;
  const std::vector<std::complex<double>> *data = nullptr;
  if (channel == 0)
    data = &hodographTotal_;
  else if (channel == 1)
    data = &hodographVert_;
  else
    data = &hodographHoriz_;
  if (!data || data->empty() || freqs_.empty())
    return;
  std::vector<std::complex<double>> iPoints, qPoints;
  for (size_t idx = 0; idx < freqs_.size(); ++idx) {
    double freqGHz = freqs_[idx] / 1e9;
    iPoints.push_back({freqGHz, (*data)[idx].real()});
    qPoints.push_back({freqGHz, (*data)[idx].imag()});
  }
  iPlot_->setData(iPoints, true);
  qPlot_->setData(qPoints, true);
}

// ========== Остальные методы из оригинального кода ==========

void MainWindow::onParametersChanged() {
  double w = ui->width_conductor_double_spin_box->value();
  double t = ui->thickness_conductor_double_spin_box->value();
  double h = ui->thickness_substrate_double_spin_box->value();
  double er = ui->dielectric_permittion_substrate_double_spin_box->value();
  double tand = ui->tangence_ougla_electric_loss_double_spin_box->value();
  double sigma = ui->conductive_conductor_double_spin_box->value();
  if (w <= 0 || t <= 0 || h <= 0 || er < 1) {
    ui->output_parameters_text_browser->setText("Ошибка");
    return;
  }
  calculator_.setParams(w, t, h, er, tand, sigma);
  fullModel_.setParams(w, t, h, er, tand, sigma);
  updateParametersOutput();
  updateHodographs();
}

void MainWindow::updateParametersOutput() {
  double f = 1e9;
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
  appendToTerminal("Generating data...");
  int total = samplesPerClassSpin_->value() * 5;
  ui->process_progress_bar->setRange(0, total);
  ui->process_label->setText("Генерация...");
  generator_.generate(samplesPerClassSpin_->value(), freqs_, features_,
                      labels_);
  ui->process_progress_bar->setValue(total);
  QCoreApplication::processEvents();
  ui->process_progress_bar->setValue(0);
  ui->process_label->setText("Процесс");
  logDetailedDataInfo();
}

void MainWindow::logDetailedDataInfo() {
  if (features_.empty())
    return;
  int nClasses = 5;
  std::vector<int> counts(nClasses, 0);
  for (int l : labels_)
    counts[l]++;
  appendToTerminal("=== Data info ===");
  appendToTerminal(QString("Samples: %1, Features: %2")
                       .arg(features_.size())
                       .arg(features_[0].size()));
  for (int c = 0; c < nClasses; ++c)
    appendToTerminal(QString("Class %1: %2").arg(c).arg(counts[c]));
}

void MainWindow::onTrainClassifier() {
  // Этот метод вызывается из консольной команды "train", а также из
  // onTrainClicked Для простоты вызываем onTrainClicked
  onTrainClicked();
}

void MainWindow::onClassify() { onClassifyClicked(); }

void MainWindow::onExportMetricsClicked() {
  if (features_.empty()) {
    appendToTerminal("Нет данных");
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
      appendToTerminal("Метрики сохранены");
    }
  }
}

void MainWindow::onPlotConfusionMatrix() {
  if (features_.empty())
    return;
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
  if (features_.empty())
    return;
  std::vector<std::vector<double>> probs;
  if (currentClassifierName_ == "lr")
    probs = lr_.predictProbabilities(features_);
  else if (currentClassifierName_ == "bayes")
    probs = bayes_.predictProbabilities(features_);
  else {
    appendToTerminal("ROC доступны только для LR и Bayes");
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
  QString msg = QString("Частные производные:\n∂Z0/∂W = %1 Ом/м\n∂Z0/∂h = %2 "
                        "Ом/м\n∂Z0/∂εr = %3 Ом")
                    .arg(sens.dZdW, 0, 'e', 4)
                    .arg(sens.dZdh, 0, 'e', 4)
                    .arg(sens.dZder, 0, 'e', 4);
  appendToTerminal(msg);
  QMessageBox::information(this, "Анализ чувствительности", msg);
}

void MainWindow::onPlotPCA() {
  if (features_.empty()) {
    appendToTerminal("Нет данных для PCA");
    return;
  }
  PCA pca;
  pca.fit(features_);
  auto proj = pca.transform(features_, 2);
  QDialog *dialog = new QDialog(this);
  dialog->setWindowTitle("PCA проекция");
  QChart *chart = new QChart();
  chart->setTitle("Первые две главные компоненты");
  for (int c = 0; c < 5; ++c) {
    QScatterSeries *series = new QScatterSeries();
    series->setName(QString("Класс %1").arg(c));
    series->setMarkerSize(8);
    for (size_t i = 0; i < proj.size(); ++i)
      if (labels_[i] == c)
        series->append(proj[i][0], proj[i][1]);
    chart->addSeries(series);
  }
  chart->createDefaultAxes();
  QChartView *view = new QChartView(chart);
  view->setRenderHint(QPainter::Antialiasing);
  QVBoxLayout *layout = new QVBoxLayout(dialog);
  layout->addWidget(view);
  dialog->resize(800, 600);
  dialog->exec();
}

void MainWindow::onClassifyWithLoss() {
  if (features_.empty()) {
    appendToTerminal("Нет данных");
    return;
  }
  // Диалог для ввода матрицы потерь 5x5
  QDialog dialog(this);
  dialog.setWindowTitle("Матрица потерь");
  QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
  QTableWidget *table = new QTableWidget(5, 5);
  table->setHorizontalHeaderLabels({"Класс 0", "1", "2", "3", "4"});
  table->setVerticalHeaderLabels({"Пред. 0", "1", "2", "3", "4"});
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      table->setItem(i, j, new QTableWidgetItem((i == j) ? "0" : "1"));
  mainLayout->addWidget(table);
  QDialogButtonBox *buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  mainLayout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted)
    return;
  std::vector<std::vector<double>> lossMatrix(5, std::vector<double>(5));
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      lossMatrix[i][j] = table->item(i, j)->text().toDouble();
  std::vector<std::vector<double>> testX = features_;
  if (currentWhitening_.isFitted()) {
    testX = currentWhitening_.transform(testX);
  }
  auto pred = classifier_->predictWithLoss(testX, lossMatrix);
  printMetricsWithAUC(pred, labels_);
  updateMetricsDisplay();
}

void MainWindow::onSaveModel() {
  if (features_.empty() || !classifier_) {
    appendToTerminal("Нет обученной модели");
    return;
  }
  QString fn =
      QFileDialog::getSaveFileName(this, "Сохранить модель", "", "*.crmodel");
  if (fn.isEmpty())
    return;
  bool ok = false;
  if (currentClassifierName_ == "lr")
    ok = ModelSerializer::saveLogisticRegression(lr_, fn.toStdString());
  else if (currentClassifierName_ == "lda")
    ok = ModelSerializer::saveLDA(lda_, fn.toStdString());
  else if (currentClassifierName_ == "nb")
    ok = ModelSerializer::saveNaiveBayes(nb_, fn.toStdString());
  else {
    appendToTerminal(
        "Сохранение для байесовского классификатора не реализовано");
    return;
  }
  appendToTerminal(ok ? "Модель сохранена" : "Ошибка сохранения");
}

void MainWindow::onLoadModel() {
  QString fn =
      QFileDialog::getOpenFileName(this, "Загрузить модель", "", "*.crmodel");
  if (fn.isEmpty())
    return;
  bool ok = false;
  if (currentClassifierName_ == "lr")
    ok = ModelSerializer::loadLogisticRegression(lr_, fn.toStdString());
  else if (currentClassifierName_ == "lda")
    ok = ModelSerializer::loadLDA(lda_, fn.toStdString());
  else if (currentClassifierName_ == "nb")
    ok = ModelSerializer::loadNaiveBayes(nb_, fn.toStdString());
  else {
    appendToTerminal("Загрузка для байесовского классификатора не реализована");
    return;
  }
  if (ok) {
    appendToTerminal("Модель загружена");
    classifier_ = (currentClassifierName_ == "lr")
                      ? static_cast<ClassifierBase *>(&lr_)
                  : (currentClassifierName_ == "lda")
                      ? static_cast<ClassifierBase *>(&lda_)
                      : static_cast<ClassifierBase *>(&nb_);
  } else {
    appendToTerminal("Ошибка загрузки");
  }
}

void MainWindow::onTuneHyperparams() {
  if (features_.empty()) {
    appendToTerminal("Нет данных для подбора гиперпараметров");
    return;
  }
  if (currentClassifierName_ != "lr") {
    appendToTerminal(
        "Подбор гиперпараметров доступен только для логистической регрессии");
    return;
  }
  HyperparameterTuner::Config cfg; // создаём объект с параметрами по умолчанию
  auto best =
      HyperparameterTuner::tuneLogisticRegression(features_, labels_, cfg);
  lr_.setHyperparameters(best.learningRate, best.epochs, best.lambda, 32);
  appendToTerminal(QString("Лучшие параметры: LR=%1 λ=%2 эпохи=%3 macroF1=%4%")
                       .arg(best.learningRate)
                       .arg(best.lambda)
                       .arg(best.epochs)
                       .arg(best.bestScore));
  lr_.train(features_, labels_);
  appendToTerminal("Модель переобучена с лучшими параметрами");
}

void MainWindow::onPlotFeatureImportance() {
  if (currentClassifierName_ != "lr") {
    appendToTerminal(
        "Важность признаков доступна только для логистической регрессии");
    return;
  }
  if (features_.empty())
    return;
  int nFeat = features_[0].size();
  auto importance =
      FeatureImportance::computeLogisticRegressionImportance(lr_, nFeat);
  // Отобразить топ-20 в диалоге
  QDialog dlg(this);
  dlg.setWindowTitle("Важность признаков");
  QVBoxLayout *layout = new QVBoxLayout(&dlg);
  QTextEdit *text = new QTextEdit(&dlg);
  text->setReadOnly(true);
  QString html = "<b>Топ-20 наиболее информативных признаков:</b><br><br><ol>";
  int limit = std::min(20, (int)importance.size());
  for (int i = 0; i < limit; ++i) {
    html += QString("<li>%1: важность = %2</li>")
                .arg(importance[i].description)
                .arg(importance[i].importance, 0, 'e', 4);
  }
  html += "</ol>";
  text->setHtml(html);
  layout->addWidget(text);
  dlg.resize(500, 400);
  dlg.exec();
}

void MainWindow::onExportReport() {
  if (features_.empty() || labels_.empty()) {
    appendToTerminal("Нет данных для отчёта");
    return;
  }
  // Здесь можно использовать QTextDocument и QPdfWriter
  QString fn =
      QFileDialog::getSaveFileName(this, "Сохранить отчёт", "", "*.html");
  if (fn.isEmpty())
    return;
  std::vector<int> pred = classifier_->predict(features_);
  auto metrics = Metrics::computeAll(pred, labels_, 5);
  // Дополнительно AUC (если есть вероятности)
  // Формируем HTML
  QString html =
      "<html><head><meta charset=\"UTF-8\"><title>Отчёт</title></head><body>";
  html += "<h1>Отчёт об эксперименте</h1>";
  html += "<p>Дата: " + QDateTime::currentDateTime().toString() + "</p>";
  html += "<p><b>Общая точность:</b> " +
          QString::number(metrics.accuracy, 'f', 2) + "%</p>";
  html += "<p><b>Macro F1:</b> " + QString::number(metrics.macroF1, 'f', 2) +
          "%</p>";
  html += "<h2>Метрики по классам</h2><table "
          "border=1><tr><th>Класс</th><th>Precision</th><th>Recall</th><th>F1</"
          "th></tr>";
  for (int c = 0; c < 5; ++c) {
    html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                .arg(c)
                .arg(metrics.precision[c], 0, 'f', 1)
                .arg(metrics.recall[c], 0, 'f', 1)
                .arg(metrics.f1[c], 0, 'f', 1);
  }
  html += "</table></body></html>";
  QFile file(fn);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(html.toUtf8());
    appendToTerminal("Отчёт сохранён");
  } else {
    appendToTerminal("Ошибка сохранения отчёта");
  }
}

void MainWindow::onExportQ15() {
  if (currentClassifierName_ != "lr") {
    appendToTerminal("Экспорт Q15 доступен только для логистической регрессии");
    return;
  }
  QString fn =
      QFileDialog::getSaveFileName(this, "Экспорт весов Q15", "", "*.h");
  if (fn.isEmpty())
    return;
  bool ok = FixedPointExport::exportLogisticRegressionQ15(
      lr_.getWeights(), lr_.getBiases(), fn.toStdString());
  appendToTerminal(ok ? "Экспорт Q15 выполнен" : "Ошибка экспорта Q15");
}

void MainWindow::onViewExperiments() {
  QDialog dlg(this);
  dlg.setWindowTitle("Журнал экспериментов");
  QVBoxLayout *layout = new QVBoxLayout(&dlg);
  QTableWidget *table = new QTableWidget(&dlg);
  auto rows = ExperimentLogger::fetchAllExperiments();
  table->setRowCount(rows.size());
  table->setColumnCount(6);
  table->setHorizontalHeaderLabels(
      {"Дата", "Классификатор", "Accuracy", "Macro F1", "AUC", "Параметры"});
  for (int i = 0; i < rows.size(); ++i) {
    for (int j = 0; j < 6; ++j)
      table->setItem(i, j, new QTableWidgetItem(rows[i][j]));
  }
  layout->addWidget(table);
  dlg.resize(800, 400);
  dlg.exec();
}