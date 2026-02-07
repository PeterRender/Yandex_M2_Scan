#include <gtest/gtest.h>
#include <limits>  // для получения граничных значений с помощью std::numeric_limits

#include "scan.hpp"

// Тест сканирования корректных строковых аргументов
TEST(ScanTest, ValidStringArguments) {
    // Создаем шаблонную лямбду для тестирования строкового аргумента
    auto test_str_scan = []<typename T>(const std::string &input, const std::string &format, const T &expected) {
        // Макрос GTest, указывающий, на каких входных данных падает тест (если он падает)
        SCOPED_TRACE("Testing type " + std::string(typeid(T).name()) + " with input '" + input + "', format '" +
                     format + "'");
        auto result = stdx::scan<T>(input, format);
        // Должен быть создан результат сканирования
        ASSERT_TRUE(result.has_value());
        // Результат сканирования должен совпадать с ожидаемым значением
        EXPECT_EQ(std::get<0>(result.value().values()), expected);
    };

    // Пустой спецификатор для строковых типов в контексте
    test_str_scan.operator()<std::string>("Company: Yandex", "Company: {}", "Yandex");
    test_str_scan.operator()<std::string_view>("Company: Yandex", "Company: {}", "Yandex");
    // Cпецификатор %s для строковых типов в контексте
    test_str_scan.operator()<std::string>("Company: Yandex", "Company: {%s}", "Yandex");
    test_str_scan.operator()<std::string_view>("Company: Yandex", "Company: {%s}", "Yandex");
}

// Тест сканирования граничных значений целых чисел (со знаком и без)
TEST(ScanTest, ValidIntegerLimits) {
    // Создаем шаблонную лямбду для тестирования целого числа
    auto test_int_scan = []<typename T>(const std::string &input_min, const std::string &input_max,
                                        const std::string &format) {
        // Проверяем 4 варианта: (min/max) × (спецификатор формата/пустой)
        for (const auto &[value_str, expected] : {std::pair{input_min, std::numeric_limits<T>::min()},
                                                  std::pair{input_max, std::numeric_limits<T>::max()}}) {
            for (const auto &fmt : {format, std::string("{}")}) {
                // Макрос GTest, указывающий, на каких входных данных падает тест (если он падает)
                SCOPED_TRACE("Testing type " + std::string(typeid(T).name()) + " with input '" + value_str +
                             "', format '" + fmt + "'");
                auto result = stdx::scan<T>(value_str, fmt);
                // Должен быть создан результат сканирования
                ASSERT_TRUE(result.has_value());
                // Результат сканирования должен совпадать с ожидаемым значением
                EXPECT_EQ(std::get<0>(result.value().values()), expected);
            }
        }
    };

    // Тесты сканирования граничных значений типов int8_t, int16_t, int32_t и int64_t
    test_int_scan.operator()<int8_t>("-128", "127", "{%d}");
    test_int_scan.operator()<int16_t>("-32768", "32767", "{%d}");
    test_int_scan.operator()<int32_t>("-2147483648", "2147483647", "{%d}");
    test_int_scan.operator()<int64_t>("-9223372036854775808", "9223372036854775807", "{%d}");

    // Тесты сканирования граничных значений типов uint8_t, uint16_t, uint32_t и uint64_t
    test_int_scan.operator()<uint8_t>("0", "255", "{%u}");
    test_int_scan.operator()<uint16_t>("0", "65535", "{%u}");
    test_int_scan.operator()<uint32_t>("0", "4294967295", "{%u}");
    test_int_scan.operator()<uint64_t>("0", "18446744073709551615", "{%u}");
}

// Тест сканирования корректных вещественных чисел
TEST(ScanTest, ValidRealArguments) {
    // Создаем шаблонную лямбду для тестирования вещественного числа
    auto test_real_scan = []<typename T>(const std::string &input, const std::string &format, T expected) {
        // Проверяем 2 варианта: спецификатор формата/пустой
        for (const auto &fmt : {format, std::string("{}")}) {
            // Макрос GTest, указывающий, на каких входных данных падает тест (если он падает)
            SCOPED_TRACE("Testing type " + std::string(typeid(T).name()) + " with input '" + input + "', format '" +
                         format + "'");
            auto result = stdx::scan<T>(input, fmt);
            // Должен быть создан результат сканирования
            ASSERT_TRUE(result.has_value());
            // Результат сканирования должен совпадать с ожидаемым значением
            if constexpr (stdx::details::FloatType<T>) {  // этот блок попадает в бинарник только для float
                EXPECT_FLOAT_EQ(std::get<0>(result.value().values()), expected);
            } else if constexpr (stdx::details::DoubleType<T>) {  // этот блок попадает в бинарник только для double
                EXPECT_DOUBLE_EQ(std::get<0>(result.value().values()), expected);
            }
        }
    };

    // Тесты сканирования float-числа в десятичной и научной нотации
    test_real_scan.operator()<float>("1.234", "{%f}", 1.234f);
    test_real_scan.operator()<float>("-1.234", "{%f}", -1.234f);
    test_real_scan.operator()<float>("1.23e-3", "{%f}", 1.23e-3f);
    test_real_scan.operator()<float>("-1.23e-3", "{%f}", -1.23e-3f);

    // Тесты сканирования double-числа в десятичной и научной нотации
    test_real_scan.operator()<double>("1.23456789", "{%f}", 1.23456789);
    test_real_scan.operator()<double>("-1.23456789", "{%f}", -1.23456789);
    test_real_scan.operator()<double>("1.23e-8", "{%f}", 1.23e-8);
    test_real_scan.operator()<double>("-1.23e-8", "{%f}", -1.23e-8);
}

// Тест сканирования некорректных числовых аргументов (целочисленных и вещественных)
TEST(ScanTest, InvalidNumberArguments) {
    // Создаем шаблонную лямбду для тестирования ошибки сканирования вещественного числа
    auto test_err = []<typename T>(const std::string &input, const std::string &format, const std::string &err_str) {
        // Проверяем 2 варианта: спецификатор формата/пустой
        for (const auto &fmt : {format, std::string("{}")}) {
            // Макрос GTest, указывающий, на каких входных данных падает тест (если он падает)
            SCOPED_TRACE("Testing type " + std::string(typeid(T).name()) + " with input '" + input + "', format '" +
                         format + "'");
            auto result = stdx::scan<T>(input, fmt);
            // Сканирование не должно дать результат
            ASSERT_FALSE(result.has_value());
            // Текст ошибки должен совпадать с ожидаемым
            EXPECT_EQ(result.error().message_, err_str);
        }
    };

    // Тесты случая "строка вместо числа"
    const std::string expected_err1("Invalid numeric argument");  // ожидаемая ошибка
    test_err.operator()<int8_t>("abc", "{%d}", expected_err1);
    test_err.operator()<int16_t>("abc", "{%d}", expected_err1);
    test_err.operator()<int32_t>("abc", "{%d}", expected_err1);
    test_err.operator()<int64_t>("abc", "{%d}", expected_err1);
    test_err.operator()<uint8_t>("abc", "{%u}", expected_err1);
    test_err.operator()<uint16_t>("abc", "{%u}", expected_err1);
    test_err.operator()<uint32_t>("abc", "{%u}", expected_err1);
    test_err.operator()<uint64_t>("abc", "{%u}", expected_err1);
    test_err.operator()<float>("abc", "{%f}", expected_err1);
    test_err.operator()<double>("abc", "{%f}", expected_err1);

    // Тесты случая "лишние символы после числа"
    const std::string expected_err2("Extra characters after number");  // ожидаемая ошибка
    test_err.operator()<int8_t>("123abc", "{%d}", expected_err2);
    test_err.operator()<int16_t>("123abc", "{%d}", expected_err2);
    test_err.operator()<int32_t>("123abc", "{%d}", expected_err2);
    test_err.operator()<int64_t>("123abc", "{%d}", expected_err2);
    test_err.operator()<uint8_t>("123abc", "{%u}", expected_err2);
    test_err.operator()<uint16_t>("123abc", "{%u}", expected_err2);
    test_err.operator()<uint32_t>("123abc", "{%u}", expected_err2);
    test_err.operator()<uint64_t>("123abc", "{%u}", expected_err2);
    test_err.operator()<float>("1.23abc", "{%f}", expected_err2);
    test_err.operator()<double>("1.23abc", "{%f}", expected_err2);

    // Тесты случая "число выходит за границы диапазона типа" (для целых чисел)
    const std::string expected_err3("Numeric value out of range");  // ожидаемая ошибка
    test_err.operator()<int8_t>("128", "{%d}", expected_err3);
    test_err.operator()<int16_t>("32768", "{%d}", expected_err3);
    test_err.operator()<int32_t>("2147483648", "{%d}", expected_err3);
    test_err.operator()<int64_t>("9223372036854775808", "{%d}", expected_err3);
    test_err.operator()<uint8_t>("256", "{%u}", expected_err3);
    test_err.operator()<uint8_t>("65536", "{%u}", expected_err3);
    test_err.operator()<uint8_t>("4294967296", "{%u}", expected_err3);
    test_err.operator()<uint8_t>("18446744073709551616", "{%u}", expected_err3);
}

// Тест для проверки некорректных спецификаторов формата
TEST(ScanTest, InvalidFormatSpecifiers) {
    // Создаем шаблонную лямбду для тестирования некорректного спецификатора формата
    auto test_err = []<typename T>(const std::string &input, const std::string &format, const std::string &err_str) {
        // Макрос GTest, указывающий, на каких входных данных падает тест (если он падает)
        SCOPED_TRACE("Testing type " + std::string(typeid(T).name()) + " with input '" + input + "', format '" +
                     format + "'");
        auto result = stdx::scan<T>(input, format);
        // Сканирование не должно дать результат
        ASSERT_FALSE(result.has_value());
        // Текст ошибки должен совпадать с ожидаемым
        EXPECT_EQ(result.error().message_, err_str);
    };

    // Спецификатор %d для строковых типов
    const std::string expected_err("Invalid format specifier for string type");  // ожидаемая ошибка
    test_err.operator()<std::string>("Yandex", "{%d}", expected_err);
    test_err.operator()<std::string_view>("Yandex", "{%d}", expected_err);
    // Спецификатор %u для знаковых типов
    const std::string expected_err1("Invalid unsigned format specifier for signed type");  // ожидаемая ошибка
    test_err.operator()<int8_t>("-123", "{%u}", expected_err1);
    test_err.operator()<int16_t>("-123", "{%u}", expected_err1);
    test_err.operator()<int32_t>("-123", "{%u}", expected_err1);
    test_err.operator()<int64_t>("-123", "{%u}", expected_err1);
    // Спецификатор %f для знаковых типов
    const std::string expected_err2("Invalid format specifier for signed type");  // ожидаемая ошибка
    test_err.operator()<int8_t>("-123", "{%f}", expected_err2);
    test_err.operator()<int16_t>("-123", "{%f}", expected_err2);
    test_err.operator()<int32_t>("-123", "{%f}", expected_err2);
    test_err.operator()<int64_t>("-123", "{%f}", expected_err2);
    // Спецификатор %d для беззнаковых типов
    const std::string expected_err3("Invalid signed format specifier for unsigned type");  // ожидаемая ошибка
    test_err.operator()<uint8_t>("123", "{%d}", expected_err3);
    test_err.operator()<uint16_t>("123", "{%d}", expected_err3);
    test_err.operator()<uint32_t>("123", "{%d}", expected_err3);
    test_err.operator()<uint64_t>("123", "{%d}", expected_err3);
    // Спецификатор %f для беззнаковых типов
    const std::string expected_err4("Invalid format specifier for unsigned type");  // ожидаемая ошибка
    test_err.operator()<uint8_t>("123", "{%f}", expected_err4);
    test_err.operator()<uint16_t>("123", "{%f}", expected_err4);
    test_err.operator()<uint32_t>("123", "{%f}", expected_err4);
    test_err.operator()<uint64_t>("123", "{%f}", expected_err4);
    // Спецификатор %s для вещественных типов
    const std::string expected_err5("Invalid format specifier for floating type");  // ожидаемая ошибка
    test_err.operator()<float>("3.14", "{%s}", expected_err5);
    test_err.operator()<double>("3.14", "{%s}", expected_err5);
}

// Тест сканирования нескольких валидных аргументов различных типов
TEST(ScanTest, MultipleValidArguments) {
    // Тест 1: целое число и float в контексте (со спецификатором и без)
    auto result1 =
        stdx::scan<int8_t, float>("I want to sum 42 and 3.14 numbers.", "I want to sum {} and {%f} numbers.");
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(std::get<0>(result1.value().values()), 42);
    EXPECT_FLOAT_EQ(std::get<1>(result1.value().values()), 3.14f);

    // Тест 2: строка и целое число в контексте (без спецификаторов)
    auto result2 = stdx::scan<std::string, int32_t>("Company: Yandex, Year: 2026", "Company: {}, Year: {}");
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(std::get<0>(result2.value().values()), "Yandex");
    EXPECT_EQ(std::get<1>(result2.value().values()), 2026);

    // Тест 3: строка, беззнаковое целое число и double в контексте (без спецификаторов)
    auto result3 = stdx::scan<std::string, uint32_t, double>("Company: Yandex, Year: 2026, Pi: 3.1415926535",
                                                             "Company: {}, Year: {}, Pi: {}");
    ASSERT_TRUE(result3.has_value());
    EXPECT_EQ(std::get<0>(result3.value().values()), "Yandex");
    EXPECT_EQ(std::get<1>(result3.value().values()), 2026);
    EXPECT_DOUBLE_EQ(std::get<2>(result3.value().values()), 3.1415926535);

    // Тест 4: строка, беззнаковое целое число и double в контексте (со спецификаторами)
    auto result4 = stdx::scan<std::string, uint32_t, double>("Company: Yandex, Year: 2026, Pi: 3.1415926535",
                                                             "Company: {%s}, Year: {%u}, Pi: {%f}");
    ASSERT_TRUE(result4.has_value());
    EXPECT_EQ(std::get<0>(result4.value().values()), "Yandex");
    EXPECT_EQ(std::get<1>(result4.value().values()), 2026);
    EXPECT_DOUBLE_EQ(std::get<2>(result4.value().values()), 3.1415926535);
}

// Тест для проверки несоответствия числа типов числу плейсхолдеров
TEST(ScanTest, InvalidTypesNumber) {
    auto result = stdx::scan<std::string>("Yandex Practicum", "{} {}");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message_, "Numbers of format specifiers and types are not equal");
}

// Тест короткосхемности парсинга аргументов
TEST(ScanTest, ShortCircuitParsing) {
    // Если второй аргумент парсится с ошибкой, то третий не должен парситься
    auto result = stdx::scan<int8_t, int8_t, int8_t>("Numbers: 123, abc, 456", "Numbers: {}, {}, {}");
    ASSERT_FALSE(result.has_value());
    // Ошибка должна быть от второго аргумента (неверный тип), а не от третьего (выход за границы диапазона)
    EXPECT_EQ(result.error().message_, "Invalid numeric argument");
}

// Тест сканирования cv-квалифицированных типов
TEST(ScanTest, ValidCVTypes) {
    // Тест с const типами std::string и uint32_t
    auto result1 = stdx::scan<const std::string, const uint32_t>("Yandex 2026", "{} {}");
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(std::get<0>(result1.value().values()), "Yandex");
    EXPECT_EQ(std::get<1>(result1.value().values()), 2026);

    // Тест с std::string и volatile double
    auto result2 =
        stdx::scan<std::string, volatile double>("Constant: Pi, Value: 3.1415926535", "Constant: {}, Value: {}");
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(std::get<0>(result2.value().values()), "Pi");
    EXPECT_DOUBLE_EQ(std::get<1>(result2.value().values()), 3.1415926535);

    // Тест с std::string и const volatile float (со спецификаторами формата)
    auto result3 =
        stdx::scan<std::string, const volatile float>("Constant: Pi, Value: 3.14", "Constant: {%s}, Value: {%f}");
    ASSERT_TRUE(result3.has_value());
    EXPECT_EQ(std::get<0>(result3.value().values()), "Pi");
    EXPECT_FLOAT_EQ(std::get<1>(result3.value().values()), 3.14f);
}

// Тест для проверки несоответствия входной и форматной строк
TEST(ScanTest, DifferentInputFormat) {
    const std::string expected_err("Unformatted text in input and format string are different");  // ожидаемая ошибка

    // Различие текстов между плейсхолдерами
    auto result = stdx::scan<uint32_t, uint32_t>("Yandex Practicum 2019 - 2026", "Yandex Practicum {%u} and {%u}");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message_, expected_err);

    // Недостаток текста во входной строке
    auto result2 = stdx::scan<uint32_t, uint32_t>("2019", "{} {}");
    ASSERT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error().message_, expected_err);
}

// Тесты для некоторых краевых случаев и специальных форматов
TEST(ScanTest, EdgeCases) {
    // Пустая входная строка (корректно)
    auto result1 = stdx::scan<std::string>("", "{}");
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(std::get<0>(result1.value().values()), "");

    // Целые числа с нулями впереди (корректно)
    auto result2 = stdx::scan<int32_t, uint32_t>("002019 0002026", "{} {}");
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(std::get<0>(result2.value().values()), 2019);
    EXPECT_EQ(std::get<1>(result2.value().values()), 2026);

    // Отрицательное число для беззнакового типа (ошибка)
    auto result3 = stdx::scan<uint32_t>("-123", "{%u}");
    ASSERT_FALSE(result3.has_value());
    EXPECT_EQ(result3.error().message_, "Invalid numeric argument");

    // Вещественное число в научной нотации для целого типа (ошибка)
    auto result4 = stdx::scan<int32_t>("3.14e3", "{%d}");
    ASSERT_FALSE(result4.has_value());
    EXPECT_EQ(result4.error().message_, "Extra characters after number");
}