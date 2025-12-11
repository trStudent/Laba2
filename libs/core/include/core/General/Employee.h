#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <cstdint>
#include <array>

namespace core::General
{
    class Employee
    {
    public:
        typedef uint16_t ID_TYPE;
        static const ID_TYPE ID_MAX = UINT16_MAX;
        static const ID_TYPE ID_MIN = 0;
        static const size_t  BUFF_SIZE = 15;
        static constexpr size_t  SERIALIZED_SIZE = sizeof(ID_TYPE) + sizeof(double) + BUFF_SIZE;

    private:
        ID_TYPE id_;
        char name_[BUFF_SIZE];
        double hours_;

    public:
        Employee() noexcept;
        Employee(ID_TYPE id, const char* name, double hours);
        Employee(const Employee& _other) noexcept = default;
        Employee(Employee&& _other) noexcept = default;
        ~Employee() noexcept = default;
        std::array<char, SERIALIZED_SIZE> serialize() const noexcept;
        static Employee deserialize(const char* s);

        Employee& operator=(const Employee& _other) noexcept = default;
        Employee& operator=(Employee&& _other) noexcept = default;

        ID_TYPE id()       const noexcept;
        const char* name() const noexcept;
        double hours()     const noexcept;

        ID_TYPE& id()   noexcept;
        char* name()    noexcept;
        double& hours() noexcept;
    };
} // namespace core::General

#endif // EMPLOYEE_H