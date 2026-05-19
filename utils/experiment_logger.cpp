#include "experiment_logger.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>


static QSqlDatabase getConnection(const QString &dbPath) {
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(dbPath);
  return db;
}

bool ExperimentLogger::initDatabase(const QString &dbPath) {
  QSqlDatabase db = getConnection(dbPath);
  if (!db.open()) {
    qDebug() << "Cannot open database:" << db.lastError().text();
    return false;
  }
  QSqlQuery query(db);
  bool ok = query.exec("CREATE TABLE IF NOT EXISTS experiments ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "timestamp TEXT,"
                       "classifier_type TEXT,"
                       "accuracy REAL,"
                       "macro_f1 REAL,"
                       "macro_auc REAL,"
                       "params TEXT,"
                       "per_class_f1 TEXT"
                       ")");
  db.close();
  return ok;
}

void ExperimentLogger::logExperiment(const QString &classifierType,
                                     double accuracy, double macroF1,
                                     double macroAUC, const QString &params) {
  logExperimentWithDetails(classifierType, accuracy, macroF1, macroAUC, params,
                           {});
}

void ExperimentLogger::logExperimentWithDetails(
    const QString &classifierType, double accuracy, double macroF1,
    double macroAUC, const QString &params,
    const std::vector<double> &perClassF1) {
  // Используем соединение по умолчанию, открываем временно
  QSqlDatabase db = QSqlDatabase::database("experiment_log", false);
  if (!db.isValid()) {
    db = QSqlDatabase::addDatabase("QSQLITE", "experiment_log");
    db.setDatabaseName("experiments.db");
  }
  if (!db.isOpen()) {
    if (!db.open()) {
      qDebug() << "Failed to open experiment log DB:" << db.lastError().text();
      return;
    }
  }
  QSqlQuery query(db);
  query.prepare("INSERT INTO experiments (timestamp, classifier_type, "
                "accuracy, macro_f1, macro_auc, params, per_class_f1) "
                "VALUES (?, ?, ?, ?, ?, ?, ?)");
  query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
  query.addBindValue(classifierType);
  query.addBindValue(accuracy);
  query.addBindValue(macroF1);
  query.addBindValue(macroAUC);
  query.addBindValue(params);
  QString f1str;
  for (double v : perClassF1)
    f1str += QString::number(v, 'f', 2) + ";";
  query.addBindValue(f1str);
  if (!query.exec()) {
    qDebug() << "Insert failed:" << query.lastError().text();
  }
}

QVector<QStringList> ExperimentLogger::fetchAllExperiments() {
  QVector<QStringList> results;
  QSqlDatabase db = QSqlDatabase::database("experiment_log", false);
  if (!db.isValid())
    return results;
  if (!db.isOpen())
    return results;
  QSqlQuery query("SELECT timestamp, classifier_type, accuracy, macro_f1, "
                  "macro_auc, params FROM experiments ORDER BY id DESC",
                  db);
  while (query.next()) {
    QStringList row;
    row << query.value(0).toString() << query.value(1).toString()
        << QString::number(query.value(2).toDouble(), 'f', 2)
        << QString::number(query.value(3).toDouble(), 'f', 2)
        << QString::number(query.value(4).toDouble(), 'f', 3)
        << query.value(5).toString();
    results.append(row);
  }
  return results;
}