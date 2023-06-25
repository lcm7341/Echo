#ifndef CONVERTIBLE_H
#define CONVERTIBLE_H

#include <string>

class Convertible {
public:
    virtual ~Convertible() {}

    virtual void import(const std::string& filename) = 0;
    virtual void export_to(const std::string& filename) = 0;
    virtual std::string get_type_filter() const = 0;
    virtual std::string get_directory() const = 0;
};

#endif // CONVERTIBLE.H
