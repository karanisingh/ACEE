//
// Created by vbmsi on 4/17/2024.
//

#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

void print_debug(const std::string& header, const std::string& message);
void print_message(const std::string& message);
void print_debug_message(const std::string& message);
void print_header(const std::string& message);
void print_line();
std::vector<std::string> split(const std::string& s, char delimiter);

#endif //HELPERS_H
