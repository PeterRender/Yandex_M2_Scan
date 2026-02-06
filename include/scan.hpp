#pragma once

#include "parse.hpp"
#include "types.hpp"

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
    // 1. Извлекаем из форматной и входной строки массив спецификаторов форматов и массив строковых значений,
    // соответствующие плейсхолдерам
    auto parse_src_result = details::parse_sources(input, format);
    if (!parse_src_result) {
        return std::unexpected(parse_src_result.error());
    }
    // Кортеж с извлеченными массивами спецификаторов форматов и строковых значений
    auto [format_parts, input_parts] = *parse_src_result;

    // 2. Проверяем количество спецификаторов форматов (ожидается, что оно равно числу типов в пакете)
    constexpr size_t expected_parts = sizeof...(Ts);  // число типов в пакете
    // Если число спецификаторов форматов не равно ожидаемому числу, то возвращаем ошибку
    if (format_parts.size() != expected_parts) {
        return std::unexpected(details::scan_error{"Numbers of format specifiers and types are not equal"});
    }

    // 3. Преобразуем строковые значения в типобезопасные значения, соответствующие спецификаторам форматов
    // == Тестовая реализация (для одного параметра в пакете) ==

    // Этот блок добавляется в бинарник, только если в пакете один параметр
    if constexpr (expected_parts == 1) {
        using FirstType = std::tuple_element_t<0, std::tuple<Ts...>>;  // псевдоним типа первого элемента в кортеже
        // Парсим первую пару (спецификатор формата, входные данные)
        auto parse_val_result = details::parse_value_with_format<FirstType>(input_parts[0], format_parts[0]);
        // Если ошибка парсинга, то возвращаем ее значение
        if (!parse_val_result) {
            return std::unexpected(parse_val_result.error());
        }
        // Успешный парсинг - создаем результат.
        return details::scan_result<Ts...>(std::make_tuple(*parse_val_result));
    }
    // Для остальных случаев временно возвращаем ошибку "Не реализовано"
    else {
        return std::unexpected(details::scan_error{"Not implemented"});
    }
}
}  // namespace stdx
