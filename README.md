# RiscEncoder

Инструмент на C++ для автоматической генерации битовых масок инструкций RISC  подобных архитектур на основе JSON-описания.

## Сборка и запуск
Требуется: C++17, CMake, nlohmann_json, GTest.

```bash
mkdir build && cd build
cmake ..
make
./risc_encoder ../input.json ../output.json