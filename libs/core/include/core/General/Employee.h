/**
 * @file Employee.h
 * @brief Definition of the Employee class for binary serialization.
 * @author Timofei Romanchuck
 * @date 2026-01-03
 */

#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <cstdint>
#include <array>

/**
 * @namespace core::General
 * @brief Main namespace for general-purpose core utilities.
 */
namespace core::General
{
    /**
     * @class Employee
     * @brief Represents an employee entity capable of binary serialization.
     * 
     * This class follows RAII principles and providing fixed-size serialization 
     * to satisfy system-level laboratory requirements.
     */
    class Employee
    {
    public:
        /** @brief Type definition for Employee ID for type-safety. */
        typedef uint16_t ID_TYPE;

        /** @name Constants
         *  Defined to avoid magic numbers.
         *  @{ */
        static const ID_TYPE ID_MAX = UINT16_MAX;      /**< Maximum allowed ID value. */
        static const ID_TYPE ID_MIN = 0;               /**< Minimum allowed ID value. */
        static const size_t  BUFF_SIZE = 15;           /**< Fixed size for the name buffer. */
        /** @brief Total size of the object when serialized to binary. */
        static constexpr size_t SERIALIZED_SIZE = sizeof(ID_TYPE) + sizeof(double) + BUFF_SIZE;
        /** @} */

    private:
        ID_TYPE id_;               /**< Unique identifier for the employee. */
        char name_[BUFF_SIZE];     /**< Fixed-size character array for the name. */
        double hours_;             /**< Number of hours worked. */

    public:
        /**
         * @brief Default constructor. 
         * Initializes employee with ID_MIN, empty name, and zero hours.
         */
        Employee() noexcept;

        /**
         * @brief Parameterized constructor.
         * @param id Employee identifier.
         * @param name C-style string for the name.
         * @param hours Number of hours worked.
         */
        Employee(ID_TYPE id, const char* name, double hours);

        /** @brief Default copy constructor. */
        Employee(const Employee& other) noexcept = default;
        
        /** @brief Default move constructor. */
        Employee(Employee&& other) noexcept = default;
        
        /** @brief Default destructor. */
        ~Employee() noexcept = default;

        /**
         * @brief Serializes the current object state into a binary array.
         * @return std::array of bytes representing the object.
         * @note Useful for file persistence.
         */
        std::array<char, SERIALIZED_SIZE> serialize() const noexcept;

        /**
         * @brief Reconstructs an Employee object from a binary buffer.
         * @param s Pointer to a source buffer of at least SERIALIZED_SIZE.
         * @return A new Employee instance populated from the buffer.
         * @warning Ensure the pointer is not null.
         */
        static Employee deserialize(const char* s);

        /** @brief Default copy assignment operator. */
        Employee& operator=(const Employee& other) noexcept = default;
        
        /** @brief Default move assignment operator. */
        Employee& operator=(Employee&& other) noexcept = default;

        /** @name Getters (Const Accessors)
         *  @{ */
        ID_TYPE id()       const noexcept; /**< @return Current ID. */
        const char* name() const noexcept; /**< @return Pointer to name buffer. */
        double hours()     const noexcept; /**< @return Hours worked. */
        /** @} */

        /** @name Setters (Mutable References)
         *  @{ */
        ID_TYPE& id()   noexcept;          /**< @return Reference to ID. */
        char* name()    noexcept;          /**< @return Pointer to name buffer. */
        double& hours() noexcept;          /**< @return Reference to hours. */
        /** @} */
    };
} // namespace core::General

#endif // EMPLOYEE_H