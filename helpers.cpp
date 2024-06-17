//
// Created by vbmsi on 4/17/2024.
//

#include "helpers.h"

#define DEBUG

/*
* Prints out a debugging statement
 */
void print_debug(const std::string &header, const std::string &message)
{
    #ifdef DEBUG
    std::cout << "============================ " << header << " ============================\n";
    std::cout << message;
    std::cout << "\n============================ " << header << " ============================" << std::endl;
    #endif
}

void print_header(const std::string &message)
{
    #ifdef DEBUG

    std::cout <<"==============================" << message << "==============================" << std::endl;
    #endif
}
void print_line()
{
    #ifdef DEBUG

    std::cout <<"===================================" << "===================================" << std::endl;
    #endif
}

void print_message(const std::string &message)
{
    #ifdef DEBUG

    std::cout << message << std::endl;
    #endif
}

void print_debug_message(const std::string &message)
{
    #ifdef DEBUG

    std::cout << "\t[DEBUG] " << message << std::endl;
    #endif
}


/*
* Helper function
*
*   Splits a string into substrings based on specified delimiter
*/
std::vector<std::string> split(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}