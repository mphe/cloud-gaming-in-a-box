#ifndef INPUT_HPP
#define INPUT_HPP

// Include platform specific headers
#ifdef __linux__ 
#include "xorg.hpp"
// Use XTest implementation by default because it will work 99% of the time.
typedef InputSenderXTest InputSender;
#elif _WIN32
#error "Not yet supported"
#else
#error "Not yet supported"
#endif

#endif
