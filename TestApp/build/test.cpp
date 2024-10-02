#include <iostream>
#include <fstream>
#include <thread>     // Для std::this_thread::sleep_for
#include <chrono>     // Для std::chrono::seconds
#include <cstdlib>    // Для rand() и srand()
#include <ctime>      // Для time()

int main() {
    // Устанавливаем начальное значение для генератора случайных чисел
    std::srand(std::time(nullptr));  

    // Открываем файл для записи
    std::ofstream outfile("/home/pexa/Projects/MyApp/TestApp_C++/tmp/indata.txt");
    if (!outfile.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }
    int i = 0;
    // Бесконечный цикл записи 0 или 1 раз в секунду
    while (i != 40) {
        int random_value = std::rand() % 2;  // Генерация 0 или 1

        // Записываем значение в файл
        outfile << random_value << std::endl;

        // Очищаем буфер, чтобы данные записались сразу
        outfile.flush();

        // Спим 1 секунду
        std::this_thread::sleep_for(std::chrono::seconds(1));
        i++;
    }

    return 0;
}
