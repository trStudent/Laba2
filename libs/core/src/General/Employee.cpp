/**
 * @file Employee.cpp
 * @brief Implementation of the Employee class methods.
 * @author Timofei Romanchuck
 * @date 2026-01-03
 * 
 * Provides logic for object initialization, data access, and 
 * fixed-size binary serialization.
 */

#include <core/General/Employee.h>
#include <cstring>

namespace core::General
{
    Employee::Employee() noexcept 
        : id_(ID_MIN), name_(""), hours_(0)
    {
    }

    Employee::Employee(ID_TYPE id, const char* name, double hours) 
        : id_(id), name_(""), hours_(hours)
    {
        // Guard against null pointers and ensure fixed-size buffer bounds
        if(nullptr != name)
        {
            for(size_t i = 0; '\0' != name[i] && i < BUFF_SIZE; i++)
            {
                name_[i] = name[i];
            }
        }
    }

    // --- Getters (Const) ---

    Employee::ID_TYPE Employee::id() const noexcept
    { return id_; }

    const char* Employee::name() const noexcept
    { return name_; }

    double Employee::hours() const noexcept
    { return hours_; }

    // --- Setters / Mutable Accessors ---

    Employee::ID_TYPE& Employee::id() noexcept
    { return id_; }

    char* Employee::name() noexcept
    { return name_; }

    double& Employee::hours() noexcept
    { return hours_; }

    std::array<char, Employee::SERIALIZED_SIZE> Employee::serialize() const noexcept
    {
        std::array<char, SERIALIZED_SIZE> m;
        size_t i = 0;

        // Build binary layout: [ID][Hours][NameBuffer]
        memcpy(&m[i], &id_, sizeof(ID_TYPE)); 
        i += sizeof(ID_TYPE);

        memcpy(&m[i], &hours_, sizeof(double)); 
        i += sizeof(double);

        // Name buffer is copied as a raw block of BUFF_SIZE
        memcpy(&m[i], name_, BUFF_SIZE);

        return m;
    }

    Employee Employee::deserialize(const char* m)
    {
        Employee output;
        size_t i = 0;

        // Extract fields in the exact same sequence as serialize()
        memcpy(&output.id_, &m[i], sizeof(ID_TYPE)); 
        i += sizeof(ID_TYPE);

        memcpy(&output.hours_, &m[i], sizeof(double)); 
        i += sizeof(double);

        memcpy(output.name_, &m[i], BUFF_SIZE);

        return output;
    }

} // namespace core::General