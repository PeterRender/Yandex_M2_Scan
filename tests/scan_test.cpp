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
TEST(ScanTest, ScanIntegerLimits) {
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

// Тест сканирования некорректных целочисленных аргументов
TEST(ScanTest, InvalidIntArgument) {
    // Случай "строка вместо числа"
    auto result = stdx::scan<int32_t>("abc", "{}");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message_, "Invalid integer argument");

    // Случай "лишние символы после числа"
    auto result2 = stdx::scan<int32_t>("123abc", "{}");
    ASSERT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error().message_, "Extra characters after number");

    // Случай "число выходит за границы диапазона типа"
    auto result3 = stdx::scan<int8_t>("256", "{}");
    ASSERT_FALSE(result3.has_value());
    EXPECT_EQ(result3.error().message_, "Integer out of range");
}