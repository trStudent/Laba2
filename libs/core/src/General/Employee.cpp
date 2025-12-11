#include <core/General/Employee.h>
#include <string.h>

using namespace core::General;

Employee::Employee() noexcept 
    : id_(ID_MIN), name_(""), hours_(0)
{
}

Employee::Employee(ID_TYPE id, const char* name, double hours) 
    : id_(id), hours_(hours)
{
    for(size_t i = 0; name[i] != '\0' && i < BUFF_SIZE; i++)
        name_[i] = name[i];
}

Employee::ID_TYPE Employee::id() const noexcept
{ return id_; }
const char* Employee::name() const noexcept
{ return name_; }
double Employee::hours() const noexcept
{ return hours_; }

Employee::ID_TYPE& Employee::id() noexcept
{ return id_; }
char* Employee::name() noexcept
{ return name_; }
double& Employee::hours() noexcept
{ return hours_; }

std::array<char, Employee::SERIALIZED_SIZE> Employee::serialize() const noexcept
{
    std::array<char, SERIALIZED_SIZE> m;
    int i = 0;
    memcpy(&m[i], &id_, sizeof(ID_TYPE)); i += sizeof(ID_TYPE);
    memcpy(&m[i], &hours_, sizeof(double)); i += sizeof(double);
    memcpy(&m[i], name_, BUFF_SIZE);
    return m;
}

Employee Employee::deserialize(const char* m)
{
    Employee output;
    int i = 0;
    memcpy(&output.id_, &m[i], sizeof(ID_TYPE)); i += sizeof(ID_TYPE);
    memcpy(&output.hours_, &m[i], sizeof(double)); i += sizeof(double);
    memcpy(output.name_, &m[i], BUFF_SIZE);
    return output;
}