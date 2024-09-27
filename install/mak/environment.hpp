// environment.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_ENVIRONMENT_HPP
#define INCLUDED_ENVIRONMENT_HPP

#include <unistd.h>

#include <vector>
#include <string>

namespace vb::mak {

struct environment {
private:
    std::string definitions;
    std::vector<std::size_t> separators;
    std::vector<std::size_t> environ;
public:

    explicit environment(const char **environment_)
    {
        while(environment_ != nullptr) {
            environ.push_back(std::size(definitions));
            definitions += std::string(*environment_);
            separators.push_back(definitions.find('=', environ.back()));
        }
    }
    
    environment(char** environment_ = ::environ) :
        environment(const_cast<const char**>(environment_))
    {}

};

} // namespace BloombergLP

#endif
