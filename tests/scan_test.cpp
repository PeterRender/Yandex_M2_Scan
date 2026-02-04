#include <gtest/gtest.h>
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