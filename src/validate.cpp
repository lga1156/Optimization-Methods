#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>


struct ValidationResult {
    int totalBranches, filledBranches, emptyBranches, branchLength;
    std::map<char, int> birdCounts;
    bool isLengthUniform, areCountsCorrect, hasStructure, exceedsMaxBranches, exceedsMaxBranchLength;
    char firstFailingBird;
    std::string filename;
    std::string errorMessage;
};


// Функция для форматированного вывода результатов валидации
void logResults(std::ostream& out, const ValidationResult& result) {
    out << "============================================================\n";
    out << "Проверка файла: " << result.filename << "\n";
    out << "→ Анализ структуры файла...\n";
    
    if (result.hasStructure) {
        out << "✅ Структура файла корректна (блоки найдены).\n";
    } else {
        out << "❌ Ошибка структуры файла.\n";
    }
    
    // Проверка количества веток
    if (result.exceedsMaxBranches) {
        out << "❌ Количество веток: " << result.totalBranches << " (превышает норму ≤ 1000)\n";
    } else {
        out << "✅ Количество веток: " << result.totalBranches << " (норма ≤ 1000)\n";
    }
    
    out << "→ Проверка содержимого блока DATA...\n";
    
    // Проверка длины веток
    if (result.exceedsMaxBranchLength) {
        out << "❌ Ошибка: на ветке больше 26 птиц -> " << result.branchLength << "\n";
    } else if (!result.isLengthUniform) {
        out << "❌ Ошибка: ветки имеют разную длину\n";
    } else {
        out << "✅ Все ветки одинаковой длины: N = " << result.branchLength << "\n";
        
        // Проверка кратности только если длина корректна
        if (result.areCountsCorrect) {
            out << "✅ Количество птиц каждого типа кратно N.\n";
            out << "✅ Все проверки пройдены успешно.\n";
            out << "Результат: OK\n";
        } else if (result.firstFailingBird != '\0') {
            int count = result.birdCounts.at(result.firstFailingBird);
            out << "❌ Ошибка: количество птиц '" << result.firstFailingBird 
                << "' (" << count << ") не кратно N=" << result.branchLength << "\n";
        }
    }
    
    out << "============================================================\n";
}


// Основная функция обработки и валидации файла
ValidationResult processFile(const std::string& filename) {
    ValidationResult result = {0, 0, 0, -1, {}, true, true, true, false, false, '\0', filename, ""};
    
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        result.hasStructure = false;
        return result;
    }

    // Парсинг данных из файла
    std::vector<std::vector<char>> filledBranches;
    std::string line;
    bool inDataSection = false;

    while (std::getline(inputFile, line)) {
        if (!line.empty() && line.back() == '\r') 
            line.pop_back();

        if (!inDataSection) {
            if (line == "DATA") inDataSection = true;
            continue;
        }
        if (line == "/") break;

        if (line == "==") {
            result.emptyBranches++;
        } else {
            std::vector<char> currentBranch;
            std::istringstream ss(line);
            char bird;
            while (ss >> bird) 
                currentBranch.push_back(bird);
            if (!currentBranch.empty()) 
                filledBranches.push_back(std::move(currentBranch));
        }
    }
    inputFile.close();
    
    // Подсчет общего количества веток
    result.filledBranches = filledBranches.size();
    result.totalBranches = result.filledBranches + result.emptyBranches;

    // Проверка единообразия длины веток
    if (result.filledBranches > 0) {
        result.branchLength = filledBranches[0].size();
        for (size_t i = 1; i < filledBranches.size(); ++i) {
            if (filledBranches[i].size() != static_cast<size_t>(result.branchLength)) {
                result.isLengthUniform = false;
                break;
            }
        }
    } else {
        result.branchLength = 0;
    }

    // Проверка начальных условий задачи
    // 1. Количество веток не должно превышать 1000
    if (result.totalBranches > 1000) {
        result.areCountsCorrect = false;
        result.exceedsMaxBranches = true;
    }
    
    // 2. Длина ветки (количество птиц на одной ветке) не должна превышать 26
    if (result.branchLength > 26) {
        result.areCountsCorrect = false;
        result.isLengthUniform = false;
        result.exceedsMaxBranchLength = true;
    }

    // Подсчет количества птиц каждого вида
    for (const auto& branch : filledBranches)
        for (char bird : branch)
            result.birdCounts[bird]++;

    // Проверка кратности количества птиц длине ветки
    if (result.isLengthUniform && result.branchLength > 0 && result.branchLength <= 26) {
        for (const auto& [bird, count] : result.birdCounts) {
            if (count % result.branchLength != 0) {
                result.areCountsCorrect = false;
                result.firstFailingBird = bird;
                break;
            }
        }
    } else {
        result.areCountsCorrect = false;
    }

    return result;
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::vector<std::string> files;
    const std::string dataDirectory = "../data";
    
   for (const auto& entry : std::filesystem::directory_iterator(dataDirectory)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
        std::sort(files.begin(), files.end());

    const std::string logDirectory = "../logs";
    try {
        std::filesystem::create_directory(logDirectory);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Ошибка при создании директории '" << logDirectory << "': " << e.what() << std::endl;
        return 1;
    }

    const std::string logFilePath = logDirectory + "/validation.log";
    std::ofstream logFile(logFilePath);

    if (!logFile.is_open()) {
        std::cerr << "Не удалось создать файл лога: " << logFilePath << std::endl;
        return 1;
    }

    for (const auto& file : files) {
        auto result = processFile(file);
        logResults(logFile, result);
    }
    
    logFile.close();
    std::cout << "Анализ файлов завершен. Результаты сохранены в " << logFilePath << "\n";

    return 0;
}