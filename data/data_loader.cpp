#include "data_loader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

void DataLoader::loadCSV(const std::string &filename,
                         std::vector<std::vector<double>> &features,
                         std::vector<int> &labels) {
  std::ifstream file(filename);
  if (!file.is_open())
    throw std::runtime_error("Cannot open file: " + filename);
  features.clear();
  labels.clear();
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty())
      continue;
    std::stringstream ss(line);
    std::string cell;
    std::vector<double> row;
    while (std::getline(ss, cell, ',')) {
      try {
        row.push_back(std::stod(cell));
      } catch (...) {
        throw std::runtime_error("Invalid number in CSV");
      }
    }
    if (row.empty())
      continue;
    int label = static_cast<int>(row.back());
    row.pop_back();
    features.push_back(row);
    labels.push_back(label);
  }
}