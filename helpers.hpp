#ifndef HELPERS_HPP_INCLUDED
#define HELPERS_HPP_INCLUDED

#include "logger.hpp"
#include <ctime>
#include <iostream>
using namespace std;

namespace helpers {

string date2string(time_t date) {
char mbstr[100];
if (std::strftime(mbstr, sizeof(mbstr), "%Y%m%d%H%M%S", std::localtime(&date)))
    return string(mbstr);
else THROW_EXCEPTION("cannot convert "<<date);
}
std::string TrimLeft(const std::string& s)
{
    const std::string WHITESPACE = " \n\r\t";
    size_t startpos = s.find_first_not_of(WHITESPACE);
    return (startpos == std::string::npos) ? "" : s.substr(startpos);
}

std::string TrimRight(const std::string& s)
{
    const std::string WHITESPACE = " \n\r\t";
    size_t endpos = s.find_last_not_of(WHITESPACE);
    return (endpos == std::string::npos) ? "" : s.substr(0, endpos+1);
}


std::string Trim(const std::string& s)
{
    return TrimRight(TrimLeft(s));
}
}


#endif // HELPERS_HPP_INCLUDED
