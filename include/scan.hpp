#pragma once

#include "parse.hpp"
#include "types.hpp"
#include <optional>

namespace stdx {

// Делаем концепты видимыми в пространстве имен stdx
using details::AllValidTypes;
using details::DoubleType;
using details::FloatType;
using details::IntType;
using details::StringType;
using details::UIntType;
using details::ValidType;

// Функция извлечения типобезопасных значений из строки по формату
template <typename... Ts>
    requires AllValidTypes<Ts...>  // проверяем на этапе компиляции валидность всех типов в пакете
std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input, std::string_view format) {
    // 1. Разбиваем входную строку и форматную строку на части, соответствующие плейсхолдерам
    auto parse_src_result = details::parse_sources(input, format);
    if (!parse_src_result) {
        return std::unexpected(parse_src_result.error());  // возвращаем ошибку парсинга формата
    }

    // 2. Распаковываем результат: два вектора подстрок
    // format_parts - подстроки форматной строки, содержащие спецификаторы форматов (значения внутри {})
    // input_parts - подстроки входной строки, соответствующие плейсхолдерам {}
    auto [format_parts, input_parts] = *parse_src_result;

    // 3. Проверяем соответствие количества плейсхолдеров и типов
    constexpr size_t expected_parts = sizeof...(Ts);  // количество типов в пакете Ts...
    if (format_parts.size() != expected_parts) {
        return std::unexpected(details::scan_error{"Numbers of format specifiers and types are not equal"});
    }

    // 4. Создаем последовательность индексов (0, 1, ..., N-1) для доступа к подстрокам
    constexpr auto part_indices = std::make_index_sequence<expected_parts>{};

    // 5. Немедленно вызываемая шаблонная лямбда (идиома IIFE) для парсинга входной строки
    // (лямбда принимает на вход последовательность индексов подстрок, созданную выше)
    return [&]<size_t... Idseq>(
               std::index_sequence<Idseq...>) -> std::expected<details::scan_result<Ts...>, details::scan_error> {
        // Создаем кортеж для хранения результатов парсинга (например, std::tuple<std::string, float, int>)
        std::tuple<Ts...> parsed_values;

        // Опциональная переменная для хранения первой ошибки парсинга (пустая, если ошибки нет)
        std::optional<details::scan_error> first_error;

        // Шаблонная лямбда для парсинга I-ой подстроки входной строки (I - целочисленный шаблонный параметр)
        auto parse_part = [&]<size_t I> -> bool {
            // Получаем тип для текущего индекса из пакета
            using ScanType = std::tuple_element_t<I, std::tuple<Ts...>>;

            // Парсим подстроку с учетом значения специфиактора формата
            auto result = details::parse_value_with_format<ScanType>(input_parts[I], format_parts[I]);

            if (result.has_value()) {
                std::get<I>(parsed_values) = std::move(*result);
                return true;  // успех
            } else {
                first_error = result.error();  // сохраняем первую ошибку
                return false;                  // ошибка
            }
        };

        // Парсим все подстроки (fold expression + короткосхемное вычисление):
        // parse_part<0>() && parse_part<1>() && ... && parse_part<N-1>()
        // Вычисление прекращается при первом false (ошибке)
        if (!(parse_part.template operator()<Idseq>() && ...)) {
            return std::unexpected(*first_error);  // достаем ошибку (гарантированно есть)
        }

        // Все успешно - возвращаем результат
        return details::scan_result<Ts...>(std::move(parsed_values));
    }(part_indices);  // вызываем лямбду с последовательностью индексов
}
}  // namespace stdx
