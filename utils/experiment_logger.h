#ifndef EXPERIMENT_LOGGER_H
#define EXPERIMENT_LOGGER_H

#include <QDateTime>
#include <QString>
#include <vector>


class ExperimentLogger {
public:
  static bool initDatabase(const QString &dbPath);
  static void logExperiment(const QString &classifierType, double accuracy,
                            double macroF1, double macroAUC,
                            const QString &params);
  static void logExperimentWithDetails(const QString &classifierType,
                                       double accuracy, double macroF1,
                                       double macroAUC, const QString &params,
                                       const std::vector<double> &perClassF1);
  static QVector<QStringList> fetchAllExperiments();
};

#endif