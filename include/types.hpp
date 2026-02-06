#pragma once

#include <cstdint>  // для использования типов int8_t, uint8_t, uint32_t и uint64_t.
#include <string>
#include <tuple>        // для использования кортежей данных
#include <type_traits>  // для использования std::remove_cv_t

namespace stdx::details {

// Класс для хранения ошибки неуспешного сканирования
struct scan_error {
    std::string message_;
    // компилятор сам сгенерирует конструктор по умолчанию.
    // значение message_ будем задавать с помощью агрегатной инициализации.
};

// Шаблонный класс для хранения результатов успешного сканирования
template <typename... Ts>
class scan_result {
private:
    std::tuple<Ts...> vals_;

public:
    // Явный параметризованный конструктор (для создания результатов сканирования)
    explicit scan_result(std::tuple<Ts...> vals) : vals_(std::move(vals)) {}

    // Метод доступа к результатам сканирования
    const std::tuple<Ts...> &values() const { return vals_; }
};

// === Концепты для ограничения набора допустимых типов, с которыми может работать функция scan ===

// Концепт для строковых типов
template <typename T>
concept StringType =
    std::same_as<std::remove_cv_t<T>, std::string> || std::same_as<std::remove_cv_t<T>, std::string_view>;

// Концепт для целочисленных типов
template <typename T>
concept IntType = std::same_as<std::remove_cv_t<T>, int8_t> || std::same_as<std::remove_cv_t<T>, int16_t> ||
                  std::same_as<std::remove_cv_t<T>, int32_t> || std::same_as<std::remove_cv_t<T>, int64_t>;

// Концепт для беззнаковых целочисленных типов (натуральных чисел)
template <typename T>
concept UIntType = std::same_as<std::remove_cv_t<T>, uint8_t> || std::same_as<std::remove_cv_t<T>, uint16_t> ||
                   std::same_as<std::remove_cv_t<T>, uint32_t> || std::same_as<std::remove_cv_t<T>, uint64_t>;

// Концепт для вещественного типа float (с одинарной точностью)
template <typename T>
concept FloatType = std::same_as<std::remove_cv_t<T>, float>;

// Концепт для вещественного типа double (с двойной точностью)
template <typename T>
concept DoubleType = std::same_as<std::remove_cv_t<T>, double>;

// Общий концепт для валидных типов
template <typename T>
concept ValidType = StringType<T> || IntType<T> || UIntType<T> || FloatType<T> || DoubleType<T>;

// Концепт-помощник для проверки всех типов в пакете
template <typename... Ts>
concept AllValidTypes = (ValidType<Ts> && ...);  // раскрываем пакет с помощью унарной правой свертки

}  // namespace stdx::details