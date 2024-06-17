//
// Created by karn on 4/16/24.
//
#include "ACEE.h"



inline z3::expr lsl(const z3::expr &l, const z3::expr &r)
{
    z3::expr bvl = z3::int2bv(32, l);
    z3::expr bvr = z3::int2bv(32, r);
    z3::expr bvres = z3::shl(bvl, bvr);
    return z3::bv2int(bvres, true);
}

inline z3::expr lsr(const z3::expr &l, const z3::expr &r)
{
    z3::expr bvl = z3::int2bv(32, l);
    z3::expr bvr = z3::int2bv(32, r);
    z3::expr bvres = z3::lshr(bvl, bvr);
    return z3::bv2int(bvres, true);
}


int main(int argc, char* argv[])
{
    // Make sure we have program to analyze
    if (argc > 1)
    {
        std::cout << "First argument: " << argv[1] << std::endl;
    }
    else
    {
        std::cout << "No arguments provided" << std::endl;
        return 1;
    }


    // Create interpreter, and then execute code
    ACEE myEngine;
    print_header("loading program: " + std::string(argv[1]));
    if (!myEngine.loadProgram(argv[1])) {

        return -1;
    }

    print_header("Concolically Executing program");
    myEngine.concolic();
    print_header("returning");

    return 0;
}
