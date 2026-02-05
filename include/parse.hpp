#pragma once

#include <charconv>  // для подключения std::from_chars
#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "types.hpp"

namespace stdx::details {

// === Семейство шаблонных функций parse_value, конвертирующих подстроки входных данных в конкретные типы

// Базовая шаблонная функция parse_value (не определена - вызовет ошибку компиляции для неподдерживаемых типов)
template <typename T>
std::expected<T, scan_error> parse_value(std::string_view input);

// Специализация шаблонной функции parse_value для типов std::string и std::string_view
template <typename T>
    requires StringType<T>
std::expected<T, scan_error> parse_value(std::string_view input) {
    return T(input);  // автоматически работает для string и string_view
}

// Специализация шаблонной функции parse_value для всех целочисленных типов
template <typename T>
    requires(IntType<T> || UIntType<T>)
std::expected<T, scan_error> parse_value(std::string_view input) {
    // Создаем переменную типа T и инициализируем ее нулем
    T value{};

    // Пытаемся распарсить строку в число
    // (ptr - указатель на первый необработанный символ, input.data() + input.size() - указатель на конец строки)
    auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), value, 10);

    // Лямбда для создания ошибок (принцип DRY)
    auto make_error = [](const char *msg) { return std::unexpected(scan_error{msg}); };

    // Обрабатываем код результата парсинга
    switch (ec) {
    // Успешный парсинг (ошибок нет)
    case std::errc{}: {
        // Проверяем, обработан ли весь ввод (если есть лишние символы после цифр, то ошибка)
        return (ptr == input.data() + input.size()) ? std::expected<T, scan_error>(value)
                                                    : make_error("Extra characters after number");
    }
    // Строка не является числом (ошибка)
    case std::errc::invalid_argument: {
        return make_error("Invalid integer argument");
    }
    // Число не помещается в тип (ошибка)
    case std::errc::result_out_of_range: {
        return make_error("Integer out of range");
    }
    // Нераспознанный код (ошибка)
    default: {
        return make_error("Failed to parse integer");
    }
    }
}

// Функция для парсинга значения с учетом спецификатора формата
template <typename T>
std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) {
    // Проверяем валидность типа T (на этапе компиляции)
    if constexpr (!ValidType<T>) {
        return std::unexpected(scan_error{"Invalid type"});  // ошибка, если тип T не валиден
    }

    // Проверяем спецификаторы форматов для валидных типов
    // Этот блок попадает в бинарник только для строкового типа
    if constexpr (StringType<T>) {
        if (fmt != "" && fmt != "%s") {  // если спецификатор формата не "" и не "%s", то возвращаем ошибку
            return std::unexpected(scan_error{"Invalid format specifier for string type"});
        }
    }
    // Этот блок попадает в бинарник только для целочисленного типа
    else if constexpr (IntType<T>) {
        if (fmt != "" && fmt != "%d") {  // если спецификатор формата не "" и не "%d", то возвращаем ошибку
            return std::unexpected(scan_error{"Invalid format specifier for integer type"});
        }
    }
    // Этот блок попадает в бинарник только для беззнакового целочисленного типа
    else if constexpr (UIntType<T>) {
        if (fmt != "" && fmt != "%u") {  // если спецификатор формата не "" и не "%u", то возвращаем ошибку
            return std::unexpected(scan_error{"Invalid format specifier for unsigned integer type"});
        }
    }
    // Этот блок попадает в бинарник только для вещественного типа
    else if constexpr (RealType<T>) {
        if (fmt != "" && fmt != "%f") {  // если спецификатор формата не "" и не "%f", то возвращаем ошибку
            return std::unexpected(scan_error{"Invalid format specifier for floating type"});
        }
    }

    // Вызываем соответствующую специализацию шаблонной функции parse_value
    return parse_value<T>(input);
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