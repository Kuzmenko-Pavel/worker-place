#include <iostream>
#include "json.h"

std::string Json::Utils::Escape(const std::string &str)
{
    std::string result;
    std::string temps = str;
    size_t f = temps.find("http:");
    if (f!=std::string::npos)
    {
        temps.replace(f, std::string("http:").length(), "https:");
    }
    for (auto it = temps.begin(); it != temps.end(); it++)
    {
        switch (*it)
        {
        case '\t':
            result.append("\\t");
            break;
        case '"':
            result.append("\\\"");
            break;
        case '\\':
            result.append(" ");
            break;
        case '\'':
            result.append("\\'");
            break;
        case '/':
            result.append("\\/");
            break;
        case '\b':
            result.append("\\b");
            break;
        case '\r':
            result.append("\\r");
            break;
        case '\n':
            result.append("\\n");
            break;
        default:
            result.append(it, it + 1);
            break;
        }
    }
    return result;
}
std::string Json::Utils::Escape(const bool &str)
{
    std::string result;
    result = str ? "true" : "false";
    return result;
}
