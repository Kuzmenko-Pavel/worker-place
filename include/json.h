#ifndef JSON_H_INCLUDED
#define JSON_H_INCLUDED

#include <string>

namespace Json
{
class Utils
{
public:
    static std::string Escape(const std::string &str);
    static std::string Escape(const bool &str);
};
}
#endif // JSON_H_INCLUDED
