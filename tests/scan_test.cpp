#include <gtest/gtest.h>
#include <limits>  // для получения граничных значений с помощью std::numeric_limits
#include <print>

#include "scan.hpp"

// Тест сканирования корректного случая "одна строка, один пустой спецификатор"
TEST(ScanTest, ValidSingleString) {
    auto result = stdx::scan<std::string>("Hello", "{}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().values()), "Hello");
}

// Тест сканирования корректного случая "одна строка, один спецификатор %s"
TEST(ScanTest, ValidSingleStringAndSpecifier) {
    auto result = stdx::scan<std::string>("Hello", "{%s}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().values()), "Hello");
}

// Тест сканирования корректного случая "одна строка в контексте, один пустой спецификатор"
TEST(ScanTest, ValidStringWithContext) {
    auto result = stdx::scan<std::string>("Phrase: Hello", "Phrase: {}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().values()), "Hello");
}

// Тест сканирования некорректного случая "одна строка, неправильный спецификатор"
TEST(ScanTest, InvalidStringSpecifier) {
    auto result = stdx::scan<std::string>("Hello", "{%d}");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message_, "Invalid format specifier for string type");
}

// Тест сканирования некорректного случая "2 плейсхолдера, 1 тип"
TEST(ScanTest, InvalidTypesNumber) {
    auto result = stdx::scan<std::string>("Hello Goodbye", "{} {}");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message_, "Numbers of format specifiers and types are not equal");
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
    test_err.operator()<int8_t>("abc", "{%d}", "Invalid numeric argument");
    test_err.operator()<int16_t>("abc", "{%d}", "Invalid numeric argument");
    test_err.operator()<int32_t>("abc", "{%d}", "Invalid numeric argument");
    test_err.operator()<int64_t>("abc", "{%d}", "Invalid numeric argument");
    test_err.operator()<uint8_t>("abc", "{%u}", "Invalid numeric argument");
    test_err.operator()<uint16_t>("abc", "{%u}", "Invalid numeric argument");
    test_err.operator()<uint32_t>("abc", "{%u}", "Invalid numeric argument");
    test_err.operator()<uint64_t>("abc", "{%u}", "Invalid numeric argument");
    test_err.operator()<float>("abc", "{%f}", "Invalid numeric argument");
    test_err.operator()<double>("abc", "{%f}", "Invalid numeric argument");

    // Тесты случая "лишние символы после числа"
    test_err.operator()<int8_t>("123abc", "{%d}", "Extra characters after number");
    test_err.operator()<int16_t>("123abc", "{%d}", "Extra characters after number");
    test_err.operator()<int32_t>("123abc", "{%d}", "Extra characters after number");
    test_err.operator()<int64_t>("123abc", "{%d}", "Extra characters after number");
    test_err.operator()<uint8_t>("123abc", "{%u}", "Extra characters after number");
    test_err.operator()<uint16_t>("123abc", "{%u}", "Extra characters after number");
    test_err.operator()<uint32_t>("123abc", "{%u}", "Extra characters after number");
    test_err.operator()<uint64_t>("123abc", "{%u}", "Extra characters after number");
    test_err.operator()<float>("1.234abc", "{%f}", "Extra characters after number");
    test_err.operator()<double>("1.234abc", "{%f}", "Extra characters after number");

    // Тесты случая "число выходит за границы диапазона типа" (для целых чисел)
    test_err.operator()<int8_t>("128", "{%d}", "Numeric value out of range");
    test_err.operator()<int16_t>("32768", "{%d}", "Numeric value out of range");
    test_err.operator()<int32_t>("2147483648", "{%d}", "Numeric value out of range");
    test_err.operator()<int64_t>("9223372036854775808", "{%d}", "Numeric value out of range");
    test_err.operator()<uint8_t>("256", "{%u}", "Numeric value out of range");
    test_err.operator()<uint8_t>("65536", "{%u}", "Numeric value out of range");
    test_err.operator()<uint8_t>("4294967296", "{%u}", "Numeric value out of range");
    test_err.operator()<uint8_t>("18446744073709551616", "{%u}", "Numeric value out of range");
}