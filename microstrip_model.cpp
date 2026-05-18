#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDoubleSpinBox>
#include <QTextBrowser>
#include <QDateTime>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , dataGen_(nullptr)
    , classifier_(nullptr)
{
    ui->setupUi(this);

    // --- Ограничения ввода для спинбоксов ---
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

    // --- Установка начальных значений ---
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

    // --- Инициализация моделей ---
    lineModel_.setParams(
        ui->width_conductor_double_spin_box->value(),
        ui->thickness_conductor_double_spin_box->value(),
        ui->thickness_substrate_double_spin_box->value(),
        ui->dielectric_permittion_substrate_double_spin_box->value(),
        ui->tangence_ougla_electric_loss_double_spin_box->value(),
        ui->conductive_conductor_double_spin_box->value()
        );
    dataGen_ = new MicrowaveNDT::DataGenerator(lineModel_);
    classifier_ = new MicrowaveNDT::LogisticRegression();

    // --- Подключение сигналов кнопок конфигурации ---
    connect(ui->saving_reconfiguration_push_button, &QPushButton::clicked, this, &MainWindow::onSaveConfiguration);
    connect(ui->booting_configuration_push_utton, &QPushButton::clicked, this, &MainWindow::onLoadConfiguration);
    connect(ui->saving_reconfiguration_push_button_2, &QPushButton::clicked, this, &MainWindow::onSaveConfiguration);
    connect(ui->booting_configuration_push_utton_2, &QPushButton::clicked, this, &MainWindow::onLoadConfiguration);
    connect(ui->saving_reconfiguration_push_button_3, &QPushButton::clicked, this, &MainWindow::onSaveConfiguration);
    connect(ui->booting_configuration_push_utton_3, &QPushButton::clicked, this, &MainWindow::onLoadConfiguration);
    connect(ui->reset_configuration_push_button, &QPushButton::clicked, this, &MainWindow::onResetConfiguration);

    // --- Изменение параметров линии (вкладка Параметры) ---
    connect(ui->width_conductor_double_spin_box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onParametersChanged);
    connect(ui->thickness_conductor_double_spin_box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onParametersChanged);
    connect(ui->thickness_substrate_double_spin_box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onParametersChanged);
    connect(ui->dielectric_permittion_substrate_double_spin_box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onParametersChanged);
    connect(ui->tangence_ougla_electric_loss_double_spin_box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onParametersChanged);
    connect(ui->conductive_conductor_double_spin_box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onParametersChanged);
    connect(ui->field_strength_conductor_breakdown_double_spin_box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onParametersChanged);

    // --- Изменение допусков (вкладка Допуски) ---
    connect(ui->width_conductor_double_spin_box_2, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onToleranceChanged);
    connect(ui->thickness_conductor_double_spin_box_2, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onToleranceChanged);
    connect(ui->thickness_substrate_double_spin_box_2, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onToleranceChanged);

    // --- Консольные команды ---
    connect(ui->input_line_edit, &QLineEdit::returnPressed, this, [this]() {
        QString cmd = ui->input_line_edit->text().trimmed();
        if (cmd == "generate") {
            onGenerateData();
        } else if (cmd == "train") {
            onTrainClassifier();
        } else if (cmd == "classify") {
            onClassify();
        } else if (!cmd.isEmpty()) {
            appendToTerminal("Неизвестная команда. Доступны: generate, train, classify");
        }
        ui->input_line_edit->clear();
    });

    // --- Первоначальный расчёт параметров и допусков ---
    onParametersChanged();
    onToleranceChanged();

    // --- Инициализация визуализации (заглушка) ---
    setupCharts();

    appendToTerminal("Программа запущена. Команды: generate, train, classify");
}

MainWindow::~MainWindow()
{
    delete dataGen_;
    delete classifier_;
    delete ui;
}

// ------------------- Параметры линии -------------------
void MainWindow::onParametersChanged()
{
    double w = ui->width_conductor_double_spin_box->value();
    double t = ui->thickness_conductor_double_spin_box->value();
    double h = ui->thickness_substrate_double_spin_box->value();
    double er = ui->dielectric_permittion_substrate_double_spin_box->value();
    double tand = ui->tangence_ougla_electric_loss_double_spin_box->value();
    double sigma = ui->conductive_conductor_double_spin_box->value();

    if (w <= 0.0 || t <= 0.0 || h <= 0.0 || er < 1.0) {
        ui->output_parameters_text_browser->setText("Ошибка: некорректные параметры (размеры > 0, εr ≥ 1)");
        return;
    }

    lineModel_.setParams(w, t, h, er, tand, sigma);
    updateParametersOutput();
}

void MainWindow::updateParametersOutput()
{
    double f = 1.0e9;
    double Z0 = lineModel_.calcZ0(f);
    double er_eff = lineModel_.calcEffPermittivity(f);
    double alpha = lineModel_.calcAttenuation(f);

    QString text = QString::asprintf(
        "Параметры линии при f = 1 ГГц:\n"
        "Волновое сопротивление Z0 = %.2f Ом\n"
        "Эффективная проницаемость ε_eff = %.4f\n"
        "Погонное затухание α = %.4f Нп/м",
        Z0, er_eff, alpha);
    ui->output_parameters_text_browser->setText(text);
}

// ------------------- Допуски -------------------
void MainWindow::onToleranceChanged()
{
    updateToleranceOutput();
}

void MainWindow::updateToleranceOutput()
{
    double w_nom = ui->width_conductor_double_spin_box->value();
    double t_nom = ui->thickness_conductor_double_spin_box->value();
    double h_nom = ui->thickness_substrate_double_spin_box->value();

    double w_tol = ui->width_conductor_double_spin_box_2->value();
    double t_tol = ui->thickness_conductor_double_spin_box_2->value();
    double h_tol = ui->thickness_substrate_double_spin_box_2->value();

    double w_dev = w_tol - w_nom;
    double t_dev = t_tol - t_nom;
    double h_dev = h_tol - h_nom;

    QString msg;
    msg += QString("Отклонения от номинальных параметров:\n");
    msg += QString("Ширина проводника: %1 м (%2%)\n").arg(w_dev, 0, 'e', 2).arg((w_dev / w_nom) * 100.0, 0, 'f', 4);
    msg += QString("Толщина проводника: %1 м (%2%)\n").arg(t_dev, 0, 'e', 2).arg((t_dev / t_nom) * 100.0, 0, 'f', 4);
    msg += QString("Толщина подложки: %1 м (%2%)\n").arg(h_dev, 0, 'e', 2).arg((h_dev / h_nom) * 100.0, 0, 'f', 4);
    ui->output_parameters_text_browser_2->setText(msg);
}

// ------------------- Генерация данных -------------------
void MainWindow::onGenerateData()
{
    freqs_.clear();
    for (int f = 1; f <= 10; ++f) {
        freqs_.push_back(f * 1e9);
    }
    dataGen_->setNoiseStd(0.05);
    dataGen_->generateDataset(200, freqs_, features_, labels_);
    appendToTerminal(QString("Сгенерировано %1 образцов, признаков %2").arg(features_.rows()).arg(features_.cols()));
    appendToOutput("Данные сгенерированы. Можно обучать классификатор.");
}

// ------------------- Обучение классификатора -------------------
void MainWindow::onTrainClassifier()
{
    if (features_.rows() == 0) {
        appendToTerminal("Ошибка: сначала сгенерируйте данные (команда generate)");
        return;
    }
    classifier_->train(features_, labels_);
    Eigen::VectorXi pred = classifier_->predict(features_);
    int correct = 0;
    for (int i = 0; i < pred.size(); ++i) {
        if (pred(i) == labels_(i)) ++correct;
    }
    double acc = static_cast<double>(correct) / pred.size();
    appendToTerminal(QString("Обучение завершено. Accuracy на обучающей выборке: %1%").arg(acc * 100, 0, 'f', 2));
    appendToOutput("Классификатор обучен.");
}

void MainWindow::onClassify()
{
    onTrainClassifier(); // для демонстрации используем ту же логику
}

// ------------------- Конфигурация (JSON) -------------------
void MainWindow::onSaveConfiguration()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить конфигурацию"), "", tr("JSON files (*.json)"));
    if (fileName.isEmpty()) return;

    QJsonObject json = saveSettingsToJson();
    QJsonDocument doc(json);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить файл."));
        return;
    }
    file.write(doc.toJson());
    file.close();
    appendToTerminal("Конфигурация сохранена в " + fileName);
}

void MainWindow::onLoadConfiguration()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Загрузить конфигурацию"), "", tr("JSON files (*.json)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось открыть файл."));
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Некорректный JSON."));
        return;
    }
    loadSettingsFromJson(doc.object());
    onParametersChanged();
    onToleranceChanged();
    appendToTerminal("Конфигурация загружена из " + fileName);
}

void MainWindow::onResetConfiguration()
{
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

QJsonObject MainWindow::saveSettingsToJson() const
{
    QJsonObject json;
    json["width_conductor"] = ui->width_conductor_double_spin_box->value();
    json["thickness_conductor"] = ui->thickness_conductor_double_spin_box->value();
    json["thickness_substrate"] = ui->thickness_substrate_double_spin_box->value();
    json["dielectric_permittivity"] = ui->dielectric_permittion_substrate_double_spin_box->value();
    json["loss_tangent"] = ui->tangence_ougla_electric_loss_double_spin_box->value();
    json["conductivity"] = ui->conductive_conductor_double_spin_box->value();
    json["breakdown_field"] = ui->field_strength_conductor_breakdown_double_spin_box->value();

    json["width_conductor_tolerance"] = ui->width_conductor_double_spin_box_2->value();
    json["thickness_conductor_tolerance"] = ui->thickness_conductor_double_spin_box_2->value();
    json["thickness_substrate_tolerance"] = ui->thickness_substrate_double_spin_box_2->value();
    return json;
}

void MainWindow::loadSettingsFromJson(const QJsonObject &json)
{
    if (json.contains("width_conductor")) ui->width_conductor_double_spin_box->setValue(json["width_conductor"].toDouble());
    if (json.contains("thickness_conductor")) ui->thickness_conductor_double_spin_box->setValue(json["thickness_conductor"].toDouble());
    if (json.contains("thickness_substrate")) ui->thickness_substrate_double_spin_box->setValue(json["thickness_substrate"].toDouble());
    if (json.contains("dielectric_permittivity")) ui->dielectric_permittion_substrate_double_spin_box->setValue(json["dielectric_permittivity"].toDouble());
    if (json.contains("loss_tangent")) ui->tangence_ougla_electric_loss_double_spin_box->setValue(json["loss_tangent"].toDouble());
    if (json.contains("conductivity")) ui->conductive_conductor_double_spin_box->setValue(json["conductivity"].toDouble());
    if (json.contains("breakdown_field")) ui->field_strength_conductor_breakdown_double_spin_box->setValue(json["breakdown_field"].toDouble());

    if (json.contains("width_conductor_tolerance")) ui->width_conductor_double_spin_box_2->setValue(json["width_conductor_tolerance"].toDouble());
    if (json.contains("thickness_conductor_tolerance")) ui->thickness_conductor_double_spin_box_2->setValue(json["thickness_conductor_tolerance"].toDouble());
    if (json.contains("thickness_substrate_tolerance")) ui->thickness_substrate_double_spin_box_2->setValue(json["thickness_substrate_tolerance"].toDouble());
}

// ------------------- Вспомогательные выводы -------------------
void MainWindow::appendToTerminal(const QString &text)
{
    ui->terminal_text_browser->append(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + text);
}

void MainWindow::appendToOutput(const QString &text)
{
    ui->output_text_browser->append(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + text);
}

// ------------------- Заглушки для графиков -------------------
void MainWindow::setupCharts()
{
    if (ui->total_channel_frame->layout()) delete ui->total_channel_frame->layout();
    ui->total_channel_frame->setLayout(new QVBoxLayout);
    ui->total_channel_frame->layout()->addWidget(new QLabel("Годограф суммарного канала (заглушка)"));

    if (ui->verticy_channel_frame->layout()) delete ui->verticy_channel_frame->layout();
    ui->verticy_channel_frame->setLayout(new QVBoxLayout);
    ui->verticy_channel_frame->layout()->addWidget(new QLabel("Годограф вертикального канала (заглушка)"));

    if (ui->horizontal_channel_frame->layout()) delete ui->horizontal_channel_frame->layout();
    ui->horizontal_channel_frame->setLayout(new QVBoxLayout);
    ui->horizontal_channel_frame->layout()->addWidget(new QLabel("Годограф горизонтального канала (заглушка)"));

    if (ui->i_component_frame->layout()) delete ui->i_component_frame->layout();
    ui->i_component_frame->setLayout(new QVBoxLayout);
    ui->i_component_frame->layout()->addWidget(new QLabel("I-составляющая (заглушка)"));

    if (ui->q_component_frame->layout()) delete ui->q_component_frame->layout();
    ui->q_component_frame->setLayout(new QVBoxLayout);
    ui->q_component_frame->layout()->addWidget(new QLabel("Q-составляющая (заглушка)"));

    appendToTerminal("Визуализация графиков временно отключена (заглушка)");
}

void MainWindow::updateHodographs()
{
    appendToTerminal("Обновление годографов (заглушка)");
}

void MainWindow::updateQuadrature(int channel)
{
    appendToTerminal(QString("Обновление квадратурных составляющих для канала %1 (заглушка)").arg(channel));
}