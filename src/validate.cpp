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
    bool isLengthUniform, areCountsCorrect;
    char firstFailingBird;
    std::string filename;
};


// Функция для форматированного вывода результатов валидации
void logResults(std::ostream& out, const ValidationResult& result) {
    out << "==================================================\n\n"
        << "    Проверка файла: " << result.filename << "\n"
        << "    Количество веток: " << result.totalBranches << "\n"
        << "    Количество заполненных веток: " << result.filledBranches << "\n"
        << "    Количество пустых веток: " << result.emptyBranches << "\n\n";

    if (!result.birdCounts.empty()) {
        out << "------------------------------\n  Птица  Количество\n";
        int i = 0;
        for (const auto& [bird, count] : result.birdCounts) {
            out << std::left << std::setw(2) << i++ << "     " 
                << std::setw(6) << bird << std::setw(10) << count << "\n";
        }
        out << "------------------------------\n\n";
    }

    out << "    Длина заполненных веток: [" << (result.isLengthUniform ? "True" : "False") << ", "
        << (result.isLengthUniform ? std::to_string(result.branchLength) : "None") << "]\n"
        << "    Кратность птиц одного типа: [" << (result.areCountsCorrect ? "True" : "False");
    
    if (!result.areCountsCorrect && result.firstFailingBird != '\0')
        out << ", '" << result.firstFailingBird << "'";
    else if (!result.areCountsCorrect)
        out << ", None";
    
    out << "]\n\n"
        << "ФАЙЛ " << result.filename << " " 
        << (result.isLengthUniform && result.areCountsCorrect ? "КОРРЕКТНЫЙ" : "НЕКОРРЕКТНЫЙ") << "\n"
        << "==================================================\n";
}


// Основная функция обработки и валидации файла
ValidationResult processFile(const std::string& filename) {
    ValidationResult result = {0, 0, 0, -1, {}, true, true, '\0', filename};
    
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
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

    // Подсчет количества птиц каждого вида
    for (const auto& branch : filledBranches)
        for (char bird : branch)
            result.birdCounts[bird]++;

    // Проверка кратности количества птиц длине ветки
    if (result.isLengthUniform && result.branchLength > 0) {
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