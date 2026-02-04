#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "types.hpp"

namespace stdx::details {

// здесь ваш код

// Функция для парсинга значения с учетом спецификатора формата
template <typename T>
std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) {
    // Проверяем на этапе компиляции валидность типа T
    if constexpr (!ValidType<T>) {
        // Возвращаем ошибку, если типа T нет в списке разрешенных типов
        return std::unexpected(scan_error{"Invalid type"});
    }

    // Обработка строки
    if constexpr (StringType<T>) {
        // Проверяем спецификатор
        if (fmt != "" && fmt != "%s") {
            return std::unexpected(scan_error{"Invalid format specifier for string type"});
        }
        return T(input);  // создаем строку из входных данных
    }

    // Для остальных типов (int, float и т.д.) парсинг пока не реализован - возвращаем временную ошибку
    return std::unexpected(scan_error{"Parsing for this type is not implemented"});
}

// Функция для проверки корректности входных данных и выделения из обеих строк интересующих данных для парсинга
template <typename... Ts>  // !пакет шаблонных параметров в этой ф-ции не используется, вероятно ошибка шаблона проекта
std::expected<std::pair<std::vector<std::string_view>, std::vector<std::string_view>>,
              scan_error>  // <тип успешного результата, тип ошибки>
parse_sources(std::string_view input, std::string_view format) {
    std::vector<std::string_view> format_parts;  // спецификаторы формата внутри {}
    std::vector<std::string_view> input_parts;   // куски входной строки, которые подставляются в {}
    size_t start = 0;
    while (true) {
        // Ищем открывающую {
        size_t open = format.find('{', start);
        if (open == std::string_view::npos) {
            break;  // больше нет плейсхолдеров
        }
        // Ищем закрывающую }
        size_t close = format.find('}', open);
        if (close == std::string_view::npos) {
            break;  // если нет закрывающей скобки, то ошибка формата
        }

        // Проверяем текст между плейсхолдерами (между start и open, между предыдущей } и текущей {)
        if (open > start) {
            // Получим кусок эталонного текста из форматной строки
            std::string_view between = format.substr(start, open - start);
            // Ищем этот кусок текста во входной строке
            auto pos = input.find(between);
            // Если кусок эталонного текста длиннее входной строки или не найден в ней, то прерываем парсинг с ошибкой.
            if (input.size() < between.size() || pos == std::string_view::npos) {
                return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
            }
            // Если этот кусок текста between не первый (до первой {), то запоминаем часть входной строки "до" куска
            // between. Это будет значение во входной строке, подставленное на место плейсхолдера.
            if (start != 0) {
                input_parts.emplace_back(input.substr(0, pos));
            }

            // Отрезаем найденный кусок эталонного текста от входной строки.
            input = input.substr(pos + between.size());
        }

        // Сохраняем спецификатор формата (то, что между {}: если {}, то сохраняем "", а если {%d}, то "%d")
        format_parts.push_back(format.substr(open + 1, close - open - 1));
        start = close + 1;
    }

    // Проверяем оставшийся текст после последней } (когда цикл выше выходит по первому условию open)
    if (start < format.size()) {
        // Получим оставшийся кусок эталонного текста из форматной строки
        std::string_view remaining_format = format.substr(start);
        // Ищем оставшийся кусок эталонного текста во входной строке
        auto pos = input.find(remaining_format);
        // Если кусок эталонного текста длиннее входной строки или не найден в ней, то прерываем парсинг с ошибкой.
        if (input.size() < remaining_format.size() || pos == std::string_view::npos) {
            return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
        }
        // Запоминаем часть входной строки "до" оставшегося куска эталонного текста.
        // Это будет значение во входной строке, подставленное на место последнего плейсхолдера.
        input_parts.emplace_back(input.substr(0, pos));

        // Отрезаем оставшийся кусок эталонного текста от входной строки.
        input = input.substr(pos + remaining_format.size());
    } else {
        // Если после последней } ничего нет
        input_parts.emplace_back(input);
    }
    // Возвращаем пару массивов: со спецификаторами формата внутри {} и со значениями, подставленными вместо {}.
    return std::pair{format_parts, input_parts};
}

}  // namespace stdx::details