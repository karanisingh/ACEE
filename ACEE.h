//
// Created by karn on 4/16/24.
//

#ifndef ACEE_H
#define ACEE_H

#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iterator>
#include <stdlib.h>
//#include <pair>

#include <z3++.h>

#include "helpers.h"


// Declare the ACEE class

class ACEE
{
public:
    struct Operand
    {
        std::vector<std::string> elements;
        std::string getString() const;
        bool write_back = false;
    };
    struct Instruction
    {
        std::string type; // ex MOV
        std::vector<Operand> operands; // rest operands
        void print() const;
    };


    ACEE();
    ~ACEE();

    bool loadProgram(std::string path);

    bool isInstructionValid(std::string instruction);
    void execute();
    int32_t evaluateOperand(const Operand& operand);
    void executeInstruction(const Instruction& instruction);
    void printRegisters() const;
    uint8_t readByte(uint32_t address) const;
    uint32_t readWord(uint32_t address) const;
    void writeByte(uint32_t address, uint8_t value);
    void writeWord(uint32_t address, uint32_t value);

    inline z3::expr lsl(const z3::expr &l, const z3::expr &r);
    inline z3::expr lsr(const z3::expr &l, const z3::expr &r);
    inline z3::expr eor(const z3::expr &l, const z3::expr &r);
    inline z3::expr orr(const z3::expr &l, const z3::expr &r);
    inline z3::expr andd(const z3::expr &l, const z3::expr &r);

    // Concolic
    void concolic();

private:

    // program path
    std::string p_path;
    std::ofstream logFile;

    // Computing Resources
    std::array<uint8_t, 2048> memory;
    std::array<uint32_t, 16> registers;
    std::unordered_map<char, int> CPRS;
    int PC;

    std::array<uint8_t, 2048> old_memory;
    std::array<uint32_t, 16> old_registers;
    std::unordered_map<char, int> old_CPRS;
    int old_PC;

    // Helper function
    void resetProcState();
    void saveProcState();
    void revertProcState();
    
    int getRegisterNumber(const std::string& reg);

    // Per Program
    std::vector<Instruction> instructions;

    // Helper Tools
    // std::unordered_map<std::string, int> is_register_symbolic;
    std::unordered_set<std::string> validInstructions;
    std::unordered_map<std::string, int> symbol2index;
    std::unordered_map<std::string, int> reg2index;

    int32_t cmp_op1, cmp_op2;
    int cmp_op1_r, cmp_op2_r;
    
    int cmp_valid;
    bool terminated;
    bool isConcolic;

    /*************** Concolic ***************/
    z3::context ctx;
    z3::expr_vector symbolicMemory;            // symbolic memory
    z3::expr_vector symbolicRegisters;          // symbolic registers
    //z3::expr_vector lastRegisters;
    inline z3::expr getSymbolicRegister(const int &reg);
    void setSymbolicRegister(const int &reg, const z3::expr &expr);


    std::vector<int> inputRegisters;                                  // contain the 
    std::unordered_set<int> coverage;
    std::vector<int> isRegisterSymbolic;
    z3::expr_vector path_constraints; 
    std::unordered_set<std::string> paths_taken;
    


      
    z3::solver solver;                                      // contain

    // Concolic outputting
    void logCode();
    void logLine(int i);
    void logTestInput(const std::vector<int>& inputRegisters);
    void logiCoverage(const std::unordered_set<int>& cov, int iterations);
    void lognCoverage();

    void logPathConstraintsV(const z3::expr_vector & constraints);
    void logPathConstraints(const std::string &constraints);
    
    std::vector<int> determineCodeUnderTest(int bPC);

    std::vector<int> test_code;
    std::vector<std::string> n_test_code;
    std::vector<std::vector<int>> all_test_cases;

};

#endif