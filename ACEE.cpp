#include "ACEE.h"

/*
 *   Interpreter Class
 *
 */

const std::unordered_set<std::string> arith = {"add", "sub", "mul", "div", "lsl", "lsr", "and", "orr", "eor"};

ACEE::ACEE() : ctx(), solver(ctx), symbolicMemory(ctx), symbolicRegisters(ctx), path_constraints(ctx)
{

    resetProcState();
    // set the const tools
    reg2index =
        {
            {"r0", 0},
            {"r1", 1},
            {"r2", 2},
            {"r3", 3},
            {"r4", 4},
            {"r5", 5},
            {"r6", 6},
            {"r7", 7},
            {"r8", 8},
            {"r9", 9},
            {"r10", 10},
            {"fp", 11},
            {"ip", 12},
            {"sp", 13},
            {"lr", 14},
            {"pc", 15}};

    validInstructions = {
        "mov", "add", "bl", "bx", "push", "pop",
        "sub", "mul", "div", "orr", "and", "eor",
        "lsl", "lsr", "cmp", "bne", "beq", "bge",
        "blt", "bgt", "ble", "b", "ldr", "str",
        "ldrb", "strb", "mvn",
        "ace", "err", "out", "ace_begin", "ace_end"};

    inputRegisters = std::vector(4, 0);
    isRegisterSymbolic = std::vector(16, 0);

    // path_condition = z3::expr(ctx);

    // symbolicMemory = z3::expr_vector(ctx);
    // symbolicRegisters = z3::expr_vector(ctx);
    // path_constraint = z3::expr_vector(ctx);
}

ACEE::~ACEE()
{
    if (logFile.is_open())
        logFile.close();
}

/*
 * Check if instruction is valid
 *
 *
 */
bool ACEE::isInstructionValid(std::string instruction)
{
    // Split the instruction into parts
    std::vector<std::string> parts = split(instruction, ' ');

    // Check the instruction type
    std::string type = parts[0];

    if (validInstructions.find(type) != validInstructions.end())
        return true;

    if (type[type.size() - 1] == ':')
    {
        // This is a label
        if (parts.size() > 1)
        {
            // This is an invalid label
            return false;
        }
        else
        {
            // Add the label to the symbol2index map
            symbol2index[type.substr(0, type.size() - 1)] = instructions.size();
            return false;
        }
    }
    else if (type[0] == '.')
    {
        // This is a valid directive
        return false;
    }
    else
    {
        // Unknown instruction
        return false;
    }
    print_debug("IDK", "Idk how we got here wbut we should fix lol");
    return true;
}

void ACEE::concolic()
{
    terminated = false;
    isConcolic = false;
    // inputRegisters.resize(4, 0);
    resetProcState();

    // print the instructions
    print_header("Instructions");
    int cnt = 0;
    for (auto &instruction : instructions)
    {
        std::cout << cnt++ << ": " << instruction.type << " ";
        for (auto &operand : instruction.operands)
        {
            std::cout << operand.getString() << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    print_line();

    // Iterate up until we get to the place we want to begin concolic
    std::cout << "\n\nLet's execute until we get to concolic point..." << std::endl;
    PC = symbol2index["main"];
    while (PC < instructions.size() && PC >= 0)
    {
        std::cout << "PC: " << PC << " ";
        executeInstruction(instructions[PC]);
        if (terminated)
        {
            std::cout << "TERMINATED";
            break;
        }
        if (isConcolic)
        {
            print_header("Concolic Execution");
            break;
        }
    }

    /************** CONCOLIC **************/

    /*
        All input variables are symbolic, while the rest will be concrete
        At beginning of concolic, we assign 0 to all input values

        Symbolic registers will have names referring to their register number

        i.e. symbolicRegisters[0] == "0"   (--> r0)
    */

    print_message("DETERMINING CODE UNDERTEST");
    test_code = determineCodeUnderTest(PC);
    logCode();

    for (auto x : test_code)
    {
        std::cout << "\tPC: " << x << "\t";
        instructions[x].print();
    }

    print_message("We start concolic at PC: " + PC);
    print_message("Saving processor state...");
    saveProcState();

    print_message("Initializing registers...");
    symbolicRegisters.resize(16);

    // Set up symbolic registers along with random initial concrete values
    for (int i = 0; i < 4; ++i)
    {
        if (inputRegisters[i])
        {
            registers[i] = 0;
            isRegisterSymbolic[i] = 1;
            z3::expr temp = ctx.int_const(("r" + std::to_string(i)).c_str());
            print_message("\t" + i);
            symbolicRegisters.set(i, temp);
        }
    }

    int iii = 0;
    print_message("ENTERING MAIN CONCOLIC LOOP");
    // first execution
    do
    {

        /***** Before Concolic Program Executuion *******/
        std::unordered_set<int> icov;
        logLine(++iii);
        logTestInput(inputRegisters);
        isConcolic = true;
        print_header("Iteration " + std::to_string(iii));
        path_constraints = z3::expr_vector(ctx);

        /********* Concolically Execute the Program *********/
        while (isConcolic)
        {
            std::cout << "PC: " << PC << "\t";
            icov.insert(PC);
            // coverage.insert(PC);
            executeInstruction(instructions[PC]);
        }

        /***** Let's checkout current coverage *****/
        print_message("Current coverage = " + std::to_string(icov.size()) + " lines of code");
        int newlines = 0;
        for (int x : icov)
        {
            if (coverage.find(x) == coverage.end())
            {
                newlines++;
                coverage.insert(x);
            }
        }

        print_message("This iteration, we hit " + std::to_string(newlines) + " new lines.");
        if (newlines == 0)
        {
            logFile << "\n We discovered no new lines, we can end execution now" << std::endl;
            all_test_cases.pop_back();
            break;
        }

        logiCoverage(icov, iii);

        // we may as well go back!
        revertProcState();

        /**********************     Solve Path Constraint     ************************/
        // path exploration
        // path we took is in path_constraints
        // lets add them to the solver, and negate the last one
        std::cout << "~~~~~~~~~Z3~~~~~~~~~" << std::endl;
        solver.reset();

        int constraints = path_constraints.size();
        logPathConstraintsV(path_constraints);
        print_message("This run's path constraint was: " + path_constraints.to_string());

        // simplify the constraints and then add it to the set of visited paths
        for (int i = 0; i < path_constraints.size(); ++i)
        {
            z3::expr s = path_constraints[i].simplify();
            path_constraints.set(i, s);
        }
        paths_taken.insert(path_constraints.to_string());

        /********************* Path Exploration ******************/

        // Negate last path condition
        z3::expr last_constraint_negated = (!(path_constraints.back())).simplify();
        path_constraints.pop_back();
        path_constraints.push_back(last_constraint_negated);

        print_debug_message("Have we already taken path constraint <<" + path_constraints.to_string() + ">> ???");

        // As long as we find our path in paths_taken, lets keep stepping up in depth
        while ((paths_taken.find(path_constraints.to_string()) != paths_taken.end()))
        {
            if (path_constraints.empty())
            {
                print_debug_message("We can't do any more directed path exploration... exiting!");
                return;
            }

            print_debug_message("Path constraint <<" + path_constraints.to_string() + ">> already taken --> lets move up in branch depth?");
            path_constraints.pop_back();
            print_debug_message("Have we already taken path constraint <<" + path_constraints.to_string() + ">> ???");
        }

        // Solve the path
        print_debug_message("New path found! Let's try and solve it!");

        for (auto x : path_constraints)
            solver.add(x);

        logFile << "After doing path exploration, we will solve for path constraint: " << path_constraints.to_string() << std::endl;

        // As long as we are unable to satisfy this constraint, lets keep stepping down in depth
        while (solver.check() != z3::sat)
        {
            if (path_constraints.empty())
            {
                print_debug_message("No more satisfiable paths... exiting!");
                return;
            }

            print_debug_message("Path constraint <<" + path_constraints.to_string() + ">> UNSATISFIABLE... lets move up in branch depth?");
            path_constraints.pop_back();
            print_debug_message("Is path constraint <<" + path_constraints.to_string() + ">> SATISFIABLE ???");
            solver.reset();
            for (auto x : path_constraints)
                solver.add(x);
        }

        print_debug_message("We found satisfying condition!");

        // // Using satisfying model, lets get next values
        z3::model m = solver.get_model();
        print_message("Model: " + m.to_string());

        // symbolicRegisters = z3::expr_vector(ctx);
        // symbolicRegisters.resize(16);
        isRegisterSymbolic.clear();
        isRegisterSymbolic.assign(16, 0);
        

        for (int i = 0; i < 4; ++i)
        {
            if (inputRegisters[i])
            {
                isRegisterSymbolic[i] = 1;
                // logFile << "TEST: " << "r" << i << " " << m.eval(symbolicRegisters[i]).to_string()
                print_message("\t\t[DEBUG] r" + std::to_string(i) + ": model eval(" + symbolicRegisters[i].to_string() + ") --> " + m.eval(symbolicRegisters[i]).to_string());
                //logFile << "According to Z3, r" + std::to_string(i) + " does not have impact, we will pick safe value 0" << std::endl;
                // print_message("HMM-->" + symbolicRegisters[i].to_string());
                if (symbolicRegisters[i].to_string() == m.eval(symbolicRegisters[i]).to_string())
                {
                    // any value works... we'll pick 0
                    registers[i] = 0;
                    print_message("\t\t[DEBUG] We will set r" + std::to_string(i) + " --> 0");
                    logFile << "According to Z3, r" + std::to_string(i) + " does not have impact, we will pick safe value 0" << std::endl;

                }
                else
                {
                    registers[i] = m.eval(symbolicRegisters[i]).as_int64();
                    print_message("\t\t[DEBUG] We will set r" + std::to_string(i) + " --> " + std::to_string(registers[i]));
                    logFile << "According to Z3, r" + std::to_string(i) + " should be " + m.eval(symbolicRegisters[i]).to_string() << std::endl;

                }
            }
        }

        for (int i = 0; i < 16; ++i)
        {
            symbolicRegisters.pop_back();
        }
        symbolicRegisters.resize(16);

        for (int i = 0; i < 4; ++i)
        {
            if (inputRegisters[i])
            {
                isRegisterSymbolic[i] = 1;
                z3::expr temp = ctx.int_const(("r" + std::to_string(i)).c_str());
                symbolicRegisters.set(i, temp);
            }
        }

        std::cout << "~~~~~~~~~Z3~~~~~~~~~" << std::endl;

        // print_message("Current coverage = " + std::to_string(coverage.size())+ " lines of code");
        // print_message("This iteration, we hit " + std::to_string(coverage.size() - last_coverage) + " new lines.");
        // if (coverage.size() == last_coverage)
        // {
        //     print_message("We made no progress, ending concolic");

        //     break;
        // }
        // last_coverage = coverage.size();

    } while (true);

    lognCoverage();
}

/*
 * Execute code file
 *
 *
 *
 */
void ACEE::execute()
{
    terminated = false;
    resetProcState();

    // print the instructions
    print_header("Instructions");
    int cnt = 0;
    for (auto &instruction : instructions)
    {
        std::cout << cnt++ << ": " << instruction.type << " ";
        for (auto &operand : instruction.operands)
        {
            std::cout << operand.getString() << "|";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    // print the symbol2index
    print_header("Symbol to index mapping");
    for (auto &[symbol, index] : symbol2index)
    {
        std::cout << symbol << " " << index << std::endl;
    }
    std::cout << std::endl;

    /*
     *
     * Execute parsed instructions
     *
     *
     */
    std::cout << std::endl;
    std::cout << "Executing instructions" << std::endl;
    PC = symbol2index["main"];

    while (PC < instructions.size() && PC >= 0)
    {
        std::cout << "PC: " << PC << "|";
        executeInstruction(instructions[PC]);
        if (terminated)
        {
            std::cout << "TERMINATED";
            break;
        }
    }
    std::cout << std::endl;
}

/*
 * return ther first element of the operand
 */
std::string ACEE::Operand::getString() const
{
    if (elements.size() == 0)
    {
        std::cout << "This must be an ERROR" << std::endl;
        return "";
    }
    std::string ret = "";
    for (int i = 0; i < elements.size(); i++)
    {
        if (i > 0)
            ret += ',';
        ret += elements[i];
    }
    return ret;
}

/*
 * Read a byte from memory
 */

uint8_t ACEE::readByte(uint32_t address) const
{
    return memory[address];
}

/*
 * Read a word from memory
 */

uint32_t ACEE::readWord(uint32_t address) const
{
    return (memory[address] << 24) | (memory[address + 1] << 16) | (memory[address + 2] << 8) | memory[address + 3];
}

/*
 * Write a byte to memory
 */

void ACEE::writeByte(uint32_t address, uint8_t value)
{
    memory[address] = value;
}

/*
 * Write a word to memory
 */

void ACEE::writeWord(uint32_t address, uint32_t value)
{
    memory[address] = (value >> 24) & 0xFF;
    memory[address + 1] = (value >> 16) & 0xFF;
    memory[address + 2] = (value >> 8) & 0xFF;
    memory[address + 3] = value & 0xFF;
}

/*
 * Evaluate an operand
 */

int32_t ACEE::evaluateOperand(const Operand &Op)
{
    // Literal value
    if (Op.elements.size() == 1)
    {
        if (Op.elements[0][0] == '#')
        {
            return std::stoi(Op.elements[0].substr(1));
        }
        else if (reg2index.find(Op.elements[0]) != reg2index.end())
        {
            return registers[reg2index[Op.elements[0]]];
        }
        else
        {
            std::cout << "Error: Invalid operand" << std::endl;
            return -1;
        }
    }
    else if (Op.elements.size() == 2)
    {
        int val1, val2;

        if (Op.elements[0][0] == '#')
        {
            val1 = std::stoi(Op.elements[0].substr(1));
        }
        else if (reg2index.find(Op.elements[0]) != reg2index.end())
        {
            val1 = registers[reg2index[Op.elements[0]]];
        }
        else
        {
            std::cout << "Error: Invalid operand" << std::endl;
            return -1;
        }

        if (Op.elements[1][0] == '#')
        {
            val2 = std::stoi(Op.elements[1].substr(1));
        }
        else if (reg2index.find(Op.elements[1]) != reg2index.end())
        {
            val2 = registers[reg2index[Op.elements[1]]];
        }
        else
        {
            std::cout << "Error: Invalid operand" << std::endl;
            return -1;
        }
        if (Op.write_back)
        {
            // Op.elements[0] has to be a register
            registers[reg2index[Op.elements[0]]] = val1 + val2;
        }
        return val1 + val2;
    }
    else
    {
        std::cout << "Error: Invalid operand" << std::endl;
        return -1;
    }
}

/*
 * Print an instruction
 */

void ACEE::Instruction::print() const
{
    std::cout << type << " ";
    for (auto &operand : operands)
    {
        std::cout << operand.getString() << " ";
    }
    std::cout << std::endl;
}

/*
 *  Execute an instruction
 *
 *
 */
void ACEE::executeInstruction(const Instruction &instruction)
{
    // std::cout << "EXECUTING: " << instruction.type << std::endl;
    // std::string x = instruction.operands[1];
    // int g = getOperandValue(x);

    const std::string opcode = instruction.type;

    if (arith.find(opcode) != arith.end())
    {
        int op1 = evaluateOperand(instruction.operands[1]);
        int op2 = evaluateOperand(instruction.operands[2]);

        int destRegIndex = reg2index[instruction.operands[0].getString()];
        int op1RegIndex = reg2index[instruction.operands[1].getString()];
        int op2RegIndex = reg2index[instruction.operands[2].getString()];

        instruction.print();
        // std::cout << "\n\t\tsymbolics: \n";
        // for(int i = 0; i < 16; ++i)
        // {
        //     std::cout <<"\t\t\t" << i << " --> " << isRegisterSymbolic[i] << "\n";
        // }
        // std::cout << std::endl;

        if (opcode == "add")
        {
            // R <-- R+R
            if ((instruction.operands[2].getString()[0] != '#') && (isRegisterSymbolic[op1RegIndex] || isRegisterSymbolic[op2RegIndex]))
            {
                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = isRegisterSymbolic[op2RegIndex] ? symbolicRegisters[op2RegIndex] : ctx.int_val(registers[op2RegIndex]);
                z3::expr result = sym_op1 + sym_op2;
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "+" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                // This register will be symbolic (who cares if already symbolic)
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else if ((instruction.operands[2].getString()[0] == '#') && (isRegisterSymbolic[op1RegIndex]))
            {

                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = ctx.int_val(op2);
                z3::expr result = sym_op1 + sym_op2;
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "+" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else
            {
                // register no longer becomes symbolic, as we give it a concrete value
                isRegisterSymbolic[destRegIndex] = 0;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now not symbolic");
                // registers[reg2index[instruction.operands[0].getString()]] = op1 * op2;
            }
            // execute concolically
            registers[reg2index[instruction.operands[0].getString()]] = op1 + op2;
        }
        else if (instruction.type == "sub")
        {
            // R <-- R-R
            print_message(" " + std::to_string(isRegisterSymbolic[op1RegIndex]) + "   " + std::to_string(isRegisterSymbolic[op2RegIndex]));
            if ((instruction.operands[2].getString()[0] != '#') && (isRegisterSymbolic[op1RegIndex] || isRegisterSymbolic[op2RegIndex]))
            {
                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = isRegisterSymbolic[op2RegIndex] ? symbolicRegisters[op2RegIndex] : ctx.int_val(registers[op2RegIndex]);
                z3::expr result = sym_op1 - sym_op2;
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "-" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                // This register will be symbolic (who cares if already symbolic)
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else if ((instruction.operands[2].getString()[0] == '#') && (isRegisterSymbolic[op1RegIndex]))
            {

                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = ctx.int_val(op2);
                z3::expr result = sym_op1 - sym_op2;
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "-" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else
            {
                // register no longer becomes symbolic, as we give it a concrete value
                isRegisterSymbolic[destRegIndex] = 0;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now not symbolic");
                // registers[reg2index[instruction.operands[0].getString()]] = op1 * op2;
            }
            // execute concolically
            registers[reg2index[instruction.operands[0].getString()]] = op1 - op2;
        }
        else if (instruction.type == "mul")
        {
            // R <-- R*R
            if ((instruction.operands[2].getString()[0] != '#') && (isRegisterSymbolic[op1RegIndex] || isRegisterSymbolic[op2RegIndex]))
            {
                // This register will be symbolic (who cares if already symbolic)
                
                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = isRegisterSymbolic[op2RegIndex] ? symbolicRegisters[op2RegIndex] : ctx.int_val(registers[op2RegIndex]);
                z3::expr result = sym_op1 * sym_op2;
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "*" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic1");
            }
            else if ((instruction.operands[2].getString()[0] == '#') && (isRegisterSymbolic[op1RegIndex]))
            {

                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = ctx.int_val(op2);
                z3::expr result = sym_op1 * sym_op2;
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "*" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else
            {
                // register no longer becomes symbolic, as we give it a concrete value
                isRegisterSymbolic[destRegIndex] = 0;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now not symbolic");
                // registers[reg2index[instruction.operands[0].getString()]] = op1 * op2;
            }
            // execute concolically
            registers[reg2index[instruction.operands[0].getString()]] = op1 * op2;
        }
        else if (instruction.type == "div")
        {
            // R <-- R/R
            if ((instruction.operands[2].getString()[0] != '#') && (isRegisterSymbolic[op1RegIndex] || isRegisterSymbolic[op2RegIndex]))
            {
                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = isRegisterSymbolic[op2RegIndex] ? symbolicRegisters[op2RegIndex] : ctx.int_val(registers[op2RegIndex]);
                z3::expr result = sym_op1 / sym_op2;
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "/" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                // This register will be symbolic (who cares if already symbolic)
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else if ((instruction.operands[2].getString()[0] == '#') && (isRegisterSymbolic[op1RegIndex]))
            {

                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = ctx.int_val(op2);
                z3::expr result = sym_op1 / sym_op2;
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "/" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else
            {
                // register no longer becomes symbolic, as we give it a concrete value
                isRegisterSymbolic[destRegIndex] = 0;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now not symbolic");
                // registers[reg2index[instruction.operands[0].getString()]] = op1 * op2;
            }
            // execute concolically
            registers[reg2index[instruction.operands[0].getString()]] = op1 / op2;
        }
        else if (instruction.type == "orr")
        {
            // R <-- R|R
            if ((instruction.operands[2].getString()[0] != '#') && (isRegisterSymbolic[op1RegIndex] || isRegisterSymbolic[op2RegIndex]))
            {
                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = isRegisterSymbolic[op2RegIndex] ? symbolicRegisters[op2RegIndex] : ctx.int_val(registers[op2RegIndex]);
                z3::expr result = orr(sym_op1, sym_op2);
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "|" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                // This register will be symbolic (who cares if already symbolic)
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else if ((instruction.operands[2].getString()[0] == '#') && (isRegisterSymbolic[op1RegIndex]))
            {

                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = ctx.int_val(op2);
                z3::expr result = orr(sym_op1, sym_op2);
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "|" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else
            {
                // register no longer becomes symbolic, as we give it a concrete value
                isRegisterSymbolic[destRegIndex] = 0;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now not symbolic");
                // registers[reg2index[instruction.operands[0].getString()]] = op1 * op2;
            }
            // execute concolically
            registers[reg2index[instruction.operands[0].getString()]] = op1 | op2;
        }
        else if (instruction.type == "and")
        {
            // R <-- R&R
            if ((instruction.operands[2].getString()[0] != '#') && (isRegisterSymbolic[op1RegIndex] || isRegisterSymbolic[op2RegIndex]))
            {
                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = isRegisterSymbolic[op2RegIndex] ? symbolicRegisters[op2RegIndex] : ctx.int_val(registers[op2RegIndex]);
                z3::expr result = andd(sym_op1, sym_op2);
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "&" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                // This register will be symbolic (who cares if already symbolic)
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else if ((instruction.operands[2].getString()[0] == '#') && (isRegisterSymbolic[op1RegIndex]))
            {

                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = ctx.int_val(op2);
                z3::expr result = andd(sym_op1, sym_op2);
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "&" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else
            {
                // register no longer becomes symbolic, as we give it a concrete value
                isRegisterSymbolic[destRegIndex] = 0;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now not symbolic");
                // registers[reg2index[instruction.operands[0].getString()]] = op1 * op2;
            }
            // execute concolically
            registers[reg2index[instruction.operands[0].getString()]] = op1 & op2;
        }
        else if (instruction.type == "eor")
        {
            // R <-- R^R
            if ((instruction.operands[2].getString()[0] != '#') && (isRegisterSymbolic[op1RegIndex] || isRegisterSymbolic[op2RegIndex]))
            {
                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = isRegisterSymbolic[op2RegIndex] ? symbolicRegisters[op2RegIndex] : ctx.int_val(registers[op2RegIndex]);
                z3::expr result = eor(sym_op1, sym_op2);
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "^" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                // This register will be symbolic (who cares if already symbolic)
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else if ((instruction.operands[2].getString()[0] == '#') && (isRegisterSymbolic[op1RegIndex]))
            {

                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = ctx.int_val(op2);
                z3::expr result = eor(sym_op1, sym_op2);
                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "^" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else
            {
                // register no longer becomes symbolic, as we give it a concrete value
                isRegisterSymbolic[destRegIndex] = 0;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now not symbolic");
                // registers[reg2index[instruction.operands[0].getString()]] = op1 * op2;
            }
            // execute concolically
            registers[reg2index[instruction.operands[0].getString()]] = op1 ^ op2;
        }
        else if (instruction.type == "lsl")
        {

            // R <-- R<<R
            if ((instruction.operands[2].getString()[0] != '#') && (isRegisterSymbolic[op1RegIndex] || isRegisterSymbolic[op2RegIndex]))
            {
                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = isRegisterSymbolic[op2RegIndex] ? symbolicRegisters[op2RegIndex] : ctx.int_val(registers[op2RegIndex]);

                ////////////////////////////////////////////////////////////////////////////
                z3::expr result = z3::shl(sym_op1, sym_op2);
                ////////////////////////////////////////////////////////////////////////////

                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "<<" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                // This register will be symbolic (who cares if already symbolic)
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else if ((instruction.operands[2].getString()[0] == '#') && (isRegisterSymbolic[op1RegIndex]))
            {

                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = ctx.int_val(op2);

                ///////////////////////////////////////////////
                z3::expr result = lsl(sym_op1, sym_op2);
                //////////////////////////////////////////////////

                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + "<<" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else
            {
                // register no longer becomes symbolic, as we give it a concrete value
                isRegisterSymbolic[destRegIndex] = 0;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now not symbolic");
                // registers[reg2index[instruction.operands[0].getString()]] = op1 * op2;
            }
            // execute concolically
            registers[reg2index[instruction.operands[0].getString()]] = op1 << op2;
            // std::cout << "\n\n===\nr2:" << registers[2] << ", r1:" << registers[1] << "\n===\n\n";
        }
        else if (instruction.type == "lsr")
        {
            // R <-- R>>R
            if ((instruction.operands[2].getString()[0] != '#') && (isRegisterSymbolic[op1RegIndex] || isRegisterSymbolic[op2RegIndex]))
            {
                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = isRegisterSymbolic[op2RegIndex] ? symbolicRegisters[op2RegIndex] : ctx.int_val(registers[op2RegIndex]);

                ////////////////////////////////////////////////////////////////////////////
                z3::expr result = z3::lshr(sym_op1, sym_op2);
                ////////////////////////////////////////////////////////////////////////////

                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + ">>" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                // This register will be symbolic (who cares if already symbolic)
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else if ((instruction.operands[2].getString()[0] == '#') && (isRegisterSymbolic[op1RegIndex]))
            {

                z3::expr sym_op1 = isRegisterSymbolic[op1RegIndex] ? symbolicRegisters[op1RegIndex] : ctx.int_val(registers[op1RegIndex]);
                z3::expr sym_op2 = ctx.int_val(op2);

                ///////////////////////////////////////////////
                z3::expr result = z3::lshr(sym_op1, sym_op2);
                //////////////////////////////////////////////////

                print_message("\t\t[DEBUG] DOING : " + sym_op1.to_string() + ">>" + sym_op2.to_string());
                print_message("\t\t[DEBUG] RESULT : " + result.to_string());
                symbolicRegisters.set(destRegIndex, result);
                isRegisterSymbolic[destRegIndex] = 1;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now symbolic");
            }
            else
            {
                // register no longer becomes symbolic, as we give it a concrete value
                isRegisterSymbolic[destRegIndex] = 0;
                print_message("\t\t[DEBUG] r" + std::to_string(destRegIndex) + " is now not symbolic");
                // registers[reg2index[instruction.operands[0].getString()]] = op1 * op2;
            }
            // execute concolically
            registers[reg2index[instruction.operands[0].getString()]] = op1 >> op2;
        }
    }
    // Branching = {B, BX, BL, BNE, BEQ, BGE, BLT, BGT, BLE}
    else if (opcode[0] == 'b')
    {
        instruction.print();
        if (opcode == "b")
        {
            // Similar handling for b
            PC = symbol2index[instruction.operands[0].getString()];
            return;
        }

        else if (instruction.type == "bne")
        {
            //print_debug_message("ENTERBNE");
            // Similar handling for bne
            //print_debug_message("SS: " + std::to_string(isRegisterSymbolic[cmp_op1_r]) + " " + std::to_string(isRegisterSymbolic[cmp_op2_r]));
            if (isRegisterSymbolic[cmp_op1_r] || isRegisterSymbolic[cmp_op2_r])
            {
                //print_debug_message("ENTER1");
                // Symbolic
                z3::expr cond_l = (isRegisterSymbolic[cmp_op1_r]) ? symbolicRegisters[cmp_op1_r] : ctx.int_val(registers[cmp_op1_r]);
                z3::expr cond_r = (isRegisterSymbolic[cmp_op2_r]) ? symbolicRegisters[cmp_op2_r] : ctx.int_val(registers[cmp_op2_r]);
                z3::expr cond = cond_l != cond_r;
                print_message("\t\t[DEBUG] CHECKING IF : " + cond_l.to_string() + "!=" + cond_r.to_string());
                print_message("\t\t[DEBUG] CHECKING IF : " + cond.to_string());

                if (CPRS['Z'] == 0)
                {
                    // We are taking this path, so conditional held true, add this to path constrsing
                    //print_debug_message(path_constraints.to_string());
                    path_constraints.push_back(cond);
                    //print_debug_message(path_constraints.to_string());
                    PC = symbol2index[instruction.operands[0].getString()];
                    return;
                }
                else
                {
                    // We are not taking this path, so conditional not held true
                    path_constraints.push_back(!cond);
                }
            }
            else
            {
                //print_debug_message("ENTER2");

                if (CPRS['Z'] == 0)
                {
                    //print_debug_message("FUCK");
                    PC = symbol2index[instruction.operands[0].getString()];

                    return;
                }
            }
        }
        else if (instruction.type == "beq")
        {

            // Similar handling for beq
            if (isRegisterSymbolic[cmp_op1_r] || isRegisterSymbolic[cmp_op2_r])
            {
                // Symbolic
                z3::expr cond_l = (isRegisterSymbolic[cmp_op1_r]) ? symbolicRegisters[cmp_op1_r] : ctx.int_val(registers[cmp_op1_r]);
                z3::expr cond_r = (isRegisterSymbolic[cmp_op2_r]) ? symbolicRegisters[cmp_op2_r] : ctx.int_val(registers[cmp_op2_r]);
                z3::expr cond = cond_l == cond_r;
                print_message("\t\t[DEBUG] CHECKING IF : " + cond_l.to_string() + "==" + cond_r.to_string());
                print_message("\t\t[DEBUG] CHECKING IF : " + cond.to_string());

                if (CPRS['Z'] == 1)
                {
                    // We are taking this path, so conditional held true, add this to path constrsing
                    path_constraints.push_back(cond);
                    PC = symbol2index[instruction.operands[0].getString()];
                    return;
                }
                else
                {
                    // We are not taking this path, so conditional not held true
                    path_constraints.push_back(!cond);
                }
            }
            else
            {
                if (CPRS['Z'] == 1)
                {
                    PC = symbol2index[instruction.operands[0].getString()];
                    return;
                }
            }
        }
        else if (instruction.type == "bge")
        {

            if (isRegisterSymbolic[cmp_op1_r] || isRegisterSymbolic[cmp_op2_r])
            {
                // Concolic
                z3::expr cond_l = (isRegisterSymbolic[cmp_op1_r]) ? symbolicRegisters[cmp_op1_r] : ctx.int_val(registers[cmp_op1_r]);
                z3::expr cond_r = (isRegisterSymbolic[cmp_op2_r]) ? symbolicRegisters[cmp_op2_r] : ctx.int_val(registers[cmp_op2_r]);
                z3::expr cond = cond_l >= cond_r;
                print_message("\t\t[DEBUG] CHECKING IF : " + cond_l.to_string() + "!=" + cond_r.to_string());
                print_message("\t\t[DEBUG] CHECKING IF : " + cond.to_string());

                if (cmp_valid && CPRS['Z'] == 0 && CPRS['N'] == CPRS['V'])
                {
                    // We are now taking this path
                    // std::cout << "\n\n===\nr0:" << registers[0] << ", r2:" << registers[2] << "\n===\n\n";
                    path_constraints.push_back(cond);
                    PC = symbol2index[instruction.operands[0].getString()];
                    return;
                }
                else
                {
                    // std::cout << "\n\n===\nr0:" << registers[0] << ", r2:" << registers[2] << "\n===\n\n";

                    // We are not taking this path
                    path_constraints.push_back(!cond);
                }
            }
            else if (CPRS['Z'] == 0 && CPRS['N'] == CPRS['V'])
            {
                PC = symbol2index[instruction.operands[0].getString()];
                return;
            }
        }
        else if (instruction.type == "blt")
        {
            // yummm blt sandwich
            if (isRegisterSymbolic[cmp_op1_r] || isRegisterSymbolic[cmp_op2_r])
            {
                //std::cout << "FUCK" << isRegisterSymbolic[cmp_op1_r] << isRegisterSymbolic[cmp_op2_r];
                z3::expr cond_l = (isRegisterSymbolic[cmp_op1_r]) ? symbolicRegisters[cmp_op1_r] : ctx.int_val(registers[cmp_op1_r]);
                z3::expr cond_r = (isRegisterSymbolic[cmp_op2_r]) ? symbolicRegisters[cmp_op2_r] : ctx.int_val(registers[cmp_op2_r]);
                z3::expr cond = cond_l < cond_r;
                print_message("\t\t[DEBUG] CHECKING IF : " + cond_l.to_string() + "<" + cond_r.to_string());
                print_message("\t\t[DEBUG] CHECKING IF : " + cond.to_string());

                if (CPRS['N'] != CPRS['V'])
                {
                    // We are taking this path, so conditional held true, add this to path constrsing
                    path_constraints.push_back(cond);
                    PC = symbol2index[instruction.operands[0].getString()];
                    return;
                }
                else
                {
                    // We are not taking this path, so conditional not held true
                    path_constraints.push_back(!cond);
                }
            }
            else
            {
                if (CPRS['N'] != CPRS['V'])
                {
                    PC = symbol2index[instruction.operands[0].getString()];
                    return;
                }
            }
        }
        else if (instruction.type == "bgt")
        {
            // similar handling for bgt
            if (isRegisterSymbolic[cmp_op1_r] || isRegisterSymbolic[cmp_op2_r])
            {
                // Symbolic
                z3::expr cond_l = (isRegisterSymbolic[cmp_op1_r]) ? symbolicRegisters[cmp_op1_r] : ctx.int_val(registers[cmp_op1_r]);
                z3::expr cond_r = (isRegisterSymbolic[cmp_op2_r]) ? symbolicRegisters[cmp_op2_r] : ctx.int_val(registers[cmp_op2_r]);
                z3::expr cond = cond_l > cond_r;
                print_message("\t\t[DEBUG] CHECKING IF : " + cond_l.to_string() + ">" + cond_r.to_string());
                print_message("\t\t[DEBUG] CHECKING IF : " + cond.to_string());

                if (CPRS['Z'] == 0 && CPRS['N'] == CPRS['V'])
                {
                    // We are taking this path, so conditional held true, add this to path constrsing
                    path_constraints.push_back(cond);
                    PC = symbol2index[instruction.operands[0].getString()];
                    return;
                }
                else
                {
                    // We are not taking this path, so conditional not held true
                    path_constraints.push_back(!cond);
                }
            }
            else
            {
                if (CPRS['Z'] == 0 && CPRS['N'] == CPRS['V'])
                {
                    PC = symbol2index[instruction.operands[0].getString()];
                    return;
                }
            }
        }
        else if (instruction.type == "ble")
        {
            // similar handling for ble
            if (isRegisterSymbolic[cmp_op1_r] || isRegisterSymbolic[cmp_op2_r])
            {
                // Symbolic
                z3::expr cond_l = (isRegisterSymbolic[cmp_op1_r]) ? symbolicRegisters[cmp_op1_r] : ctx.int_val(registers[cmp_op1_r]);
                z3::expr cond_r = (isRegisterSymbolic[cmp_op2_r]) ? symbolicRegisters[cmp_op2_r] : ctx.int_val(registers[cmp_op2_r]);
                z3::expr cond = cond_l <= cond_r;
                print_message("\t\t[DEBUG] CHECKING IF : " + cond_l.to_string() + "<=" + cond_r.to_string());
                print_message("\t\t[DEBUG] CHECKING IF : " + cond.to_string());

                if (CPRS['Z'] == 1 && CPRS['N'] != CPRS['V'])
                {
                    // We are taking this path, so conditional held true, add this to path constrsing
                    path_constraints.push_back(cond);
                    PC = symbol2index[instruction.operands[0].getString()];
                    return;
                }
                else
                {
                    // We are not taking this path, so conditional not held true
                    path_constraints.push_back(!cond);
                }
            }
            else
            {
                if (CPRS['Z'] == 1 && CPRS['N'] != CPRS['V'])
                {
                    PC = symbol2index[instruction.operands[0].getString()];
                    return;
                }
            }
        }
        else if (instruction.type == "bx")
        {
            // simply set the PC to the value in the lr register
            PC = registers[14];
            return;
        }
        else if (instruction.type == "bl")
        {
            // Similar handling for bl
            registers[14] = PC + 1;
            PC = symbol2index[instruction.operands[0].getString()];
            return;
        }
    }
    // Register = {MOV}
    else if (instruction.type == "mov")
    {
        instruction.print();
        int val = evaluateOperand(instruction.operands[1]);
        if (instruction.operands[1].getString()[0] != '#' && isRegisterSymbolic[getRegisterNumber(instruction.operands[1].getString())])
        {
            // we are putting symbolic value in this register
            isRegisterSymbolic[getRegisterNumber(instruction.operands[0].getString())] = 1;
            print_message("\t\t[DEBUG] sym " + instruction.operands[1].getString() + "= " + symbolicRegisters[getRegisterNumber(instruction.operands[1].getString())].to_string());
            symbolicRegisters[getRegisterNumber(instruction.operands[0].getString())] = symbolicRegisters[getRegisterNumber(instruction.operands[1].getString())];
            print_message("\t\t[DEBUG] sym " + instruction.operands[0].getString() + "= " + symbolicRegisters[getRegisterNumber(instruction.operands[0].getString())].to_string());
        }
        else
        {
            // we are putting concrete value in this register
            isRegisterSymbolic[getRegisterNumber(instruction.operands[0].getString())] = 0;
        }
        // execute concretely
        registers[reg2index[instruction.operands[0].getString()]] = val;
    }
    else if (instruction.type == "mvn")
    {
        // int val = evaluateOperand(instruction.operands[1]);
        // registers[reg2index[instruction.operands[0].getString()]] = ~val;
        instruction.print();

        int val = evaluateOperand(instruction.operands[1]);
        if (instruction.operands[1].getString()[0] != '#' && isRegisterSymbolic[getRegisterNumber(instruction.operands[1].getString())])
        {
            // we are putting symbolic value in this register
            isRegisterSymbolic[getRegisterNumber(instruction.operands[0].getString())] = 1;
            print_message("\t\t[DEBUG] sym " + instruction.operands[1].getString() + "= ~" + symbolicRegisters[getRegisterNumber(instruction.operands[1].getString())].to_string());
            symbolicRegisters[getRegisterNumber(instruction.operands[0].getString())] = ~symbolicRegisters[getRegisterNumber(instruction.operands[1].getString())];
            print_message("\t\t[DEBUG] sym " + instruction.operands[0].getString() + "= " + symbolicRegisters[getRegisterNumber(instruction.operands[0].getString())].to_string());
        }
        else
        {
            // we are putting concrete value in this register
            isRegisterSymbolic[getRegisterNumber(instruction.operands[0].getString())] = 0;
        }
        // execute concretely
        registers[reg2index[instruction.operands[0].getString()]] = ~val;
    }
    // Stack = {PUSH, POP}
    else if (instruction.type == "push")
    {
        // Similar handling for push
        instruction.print();

        // decrement then push onto the stack.
        for (auto &operand : instruction.operands)
        {
            // concrete
            registers[13] -= 4;
            writeWord(registers[13], registers[reg2index[operand.getString()]]);

            // // symbolic
            // if(isRegisterSymbolic[regIndex])
            // {

            // }
        }
    }
    else if (instruction.type == "pop")
    {
        // Similar handling for pop
        instruction.print();

        // pop from the stack then increment stack pointer
        for (int i = instruction.operands.size() - 1; i >= 0; i--)
        {
            registers[reg2index[instruction.operands[i].getString()]] = readWord(registers[13]);
            registers[13] += 4;
        }
    }
    // Comparison = {CMP}
    else if (instruction.type == "cmp")
    {
        // Similar handling for cmp
        instruction.print();
        int32_t op1 = evaluateOperand(instruction.operands[0]);
        int32_t op2 = evaluateOperand(instruction.operands[1]);

        cmp_op1 = op1;
        cmp_op1_r = (instruction.operands[0].getString()[0] == '#') ? -1 : getRegisterNumber(instruction.operands[0].getString());
        // print_message("CMP_OP1_R = <" + std::to_string(cmp_op1_r) + ">");
        cmp_op2 = op2;
        cmp_op2_r = (instruction.operands[1].getString()[0] == '#') ? -1 : getRegisterNumber(instruction.operands[1].getString());
        // print_message("CMP_OP2_R = <" + std::to_string(cmp_op2_r) + ">");

        cmp_valid = 1;
        // update CPRS
        CPRS['N'] = (op1 - op2) < 0;
        CPRS['Z'] = (op1 - op2) == 0;
        CPRS['C'] = op1 >= op2;
        CPRS['V'] = (op1 < 0 && op2 >= 0 && (op1 - op2) >= 0) || (op1 >= 0 && op2 < 0 && (op1 - op2) < 0);
    }
    // Load and store = {LDR, STR}
    else if (instruction.type == "ldr")
    {
        // Similar handling for ldr
        instruction.print();
        int address = evaluateOperand(instruction.operands[1]);
        // read 4 bytes from the memory
        registers[reg2index[instruction.operands[0].getString()]] = readWord(address);
    }
    else if (instruction.type == "ldrb")
    {
        // Similar handling for ldr
        instruction.print();
        int address = evaluateOperand(instruction.operands[1]);
        // read 4 bytes from the memory
        registers[reg2index[instruction.operands[0].getString()]] = readByte(address);
    }
    else if (instruction.type == "ldm")
    {
        instruction.print();
        int base = evaluateOperand(instruction.operands[0]);
        for (int i = 1; i < instruction.operands.size(); i++)
        {
            registers[reg2index[instruction.operands[i].getString()]] = readWord(base);
            base += 4;
        }
    }
    else if (instruction.type == "str")
    {
        // Similar handling for str
        instruction.print();
        int address = evaluateOperand(instruction.operands[1]);
        // store 4 bytes to the memory
        writeWord(address, registers[reg2index[instruction.operands[0].getString()]]);
    }
    else if (instruction.type == "strb")
    {
        // Similar handling for str
        instruction.print();
        int address = evaluateOperand(instruction.operands[1]);
        // memory[address] = registers[reg2index[instruction.operands[0]]];
        //  store 4 bytes to the memory
        writeByte(address, registers[reg2index[instruction.operands[0].getString()]] & 0xFF);
    }
    else if (instruction.type == "stm")
    {
        instruction.print();
        int base = evaluateOperand(instruction.operands[0]);
        for (int i = 1; i < instruction.operands.size(); i++)
        {
            writeWord(base, registers[reg2index[instruction.operands[i].getString()]]);
            base += 4;
        }
    }
    else if (opcode == "err")
    {
        /*  Terminate */
        terminated = true;
        return;
    }
    else if (opcode == "ace")
    {
        print_message("d");
        instruction.print();
        // signify inputs
        // CAN ONLY SUPPORT r0, r1, r2, r3
        int reg = std::stoi(instruction.operands[0].getString().substr(1, 1));
        print_message("\t\t[DEBUG] Concolic input --> " + instruction.operands[0].getString());
        inputRegisters[reg] = 1;
        // print_message("input arr[" + std::to_string(reg) + "] is now 1");
    }
    else if (opcode == "out")
    {
        // output a register to std out
        std::cout << "out "
                  << instruction.operands[0].getString() << "\t\t\t"
                  << instruction.operands[0].getString() << " ==> "
                  << registers[reg2index[instruction.operands[0].getString()]] << std::endl;
    }
    else if (opcode == "ace_begin")
    {

        print_message("ace_begin");
        isConcolic = 1;
    }
    else if (opcode == "ace_end")
    {
        print_message("end concolic");
        isConcolic = 0;
    }
    else
    {
        std::cout << "Unknown instruction" << std::endl;
    }

    // Next sequential instruction
    PC++;
}

void ACEE::printRegisters() const
{
    for (int i = 0; i < registers.size(); ++i)
    {
        std::cout << "R" << i << ": " << registers[i] << std::endl;
    }
}

void ACEE::resetProcState()
{
    memset(memory.data(), 0, memory.size()); // reset memory
    registers.fill(0);                       // reset registers
    registers[13] = memory.size();           // top of' stack (stack pointer points at first item on stack)
    registers[14] = -1;                      // link registes
    PC = 0;                                  // Program counter == R15
    terminated = false;

    // Set the condition
    CPRS['N'] = 0; // negative
    CPRS['Z'] = 0; // zero
    CPRS['C'] = 0; // carry
    CPRS['V'] = 0; // overflow
    cmp_valid = 0;
}

// Helper function to save current processor state
void ACEE::saveProcState()
{
    old_memory = memory;
    old_registers = registers;
    old_CPRS = CPRS;
    old_PC = PC;
}

void ACEE::revertProcState()
{
    memory = old_memory;
    registers = old_registers;
    CPRS = old_CPRS;
    PC = old_PC;
}

bool ACEE::loadProgram(std::string path)
{
    p_path = path.substr(path.find_last_of('/') + 1, path.length() - path.find_last_of('/') - 1);
    print_message("Path: " + path);
    print_message("File: " + p_path);

    std::string output_file = "acee_" + p_path.substr(0, p_path.find_last_of('.')) + ".log";
    print_message("OUTPU: " + output_file);

    logFile.open(output_file, std::ofstream::out | std::ofstream::trunc);
    // logFile.clear();

    if (!logFile.is_open())
        std::cerr << "Failed to open log file." << std::endl;

    // get the file name from command line arg
    // open a file in read mode. and read it line by line.
    std::vector<std::string> code;
    std::ifstream file(path);
    std::string line;
    while (getline(file, line))
    {
        code.push_back(line);
    }

    for (auto &line : code)
    {
        // Preprocessing

        // Remove empty lines
        if (line.empty())
            continue;

        // Remove comments from file
        size_t found = line.find('/');
        if (found != std::string::npos)
            line = line.substr(0, found);
        found = line.find('@');
        if (found != std::string::npos)
            line = line.substr(0, found);

        // Parse the instruction and add it to the instructions vector
        std::istringstream iss(line);
        std::vector<std::string> tokens{
            std::istream_iterator<std::string>{iss},
            std::istream_iterator<std::string>{}};
        if (tokens.empty() || !isInstructionValid(tokens[0]) || tokens[0][0] == '@' || tokens[0][0] == '/')
            continue;
        // FIX HERE
        Instruction instr;
        if (tokens[0] == "push" || tokens[0] == "pop")
        {
            for (int i = 1; i < tokens.size(); i++)
            {
                Operand op;
                if (tokens[i][0] == '{' || tokens[i][0] == '[')
                {
                    tokens[i] = tokens[i].substr(1);
                }
                if (tokens[i].back() == '}' || tokens[i].back() == ',' || tokens[i].back() == ']')
                {
                    tokens[i] = tokens[i].substr(0, tokens[i].size() - 1);
                }
                op.elements.push_back(tokens[i]);
                instr.operands.push_back(op);
            }
        }
        else if (tokens[0] == "ldm" || tokens[0] == "stm")
        {
            for (int i = 1; i < tokens.size(); i++)
            {
                Operand op;
                if (tokens[i][0] == '{' || tokens[i][0] == '[')
                {
                    tokens[i] = tokens[i].substr(1);
                }
                if (tokens[i].back() == '}' || tokens[i].back() == ',' || tokens[i].back() == ']')
                {
                    tokens[i] = tokens[i].substr(0, tokens[i].size() - 1);
                }
                op.elements.push_back(tokens[i]);
                instr.operands.push_back(op);
            }
        }
        else
        { // Other instructions
            Operand op;
            bool isInBrackets = false;
            for (int i = 1; i < tokens.size(); ++i)
            {
                if (tokens[i].back() == ',')
                {
                    tokens[i] = tokens[i].substr(0, tokens[i].size() - 1);
                }
                if (tokens[i][0] == '[' && tokens[i].back() == ']')
                {
                    tokens[i] = tokens[i].substr(1);
                    tokens[i] = tokens[i].substr(0, tokens[i].size() - 1);
                    op.elements.push_back(tokens[i]);
                    instr.operands.push_back(op);
                    op.elements.clear();
                }
                else if (tokens[i][0] == '[')
                {
                    tokens[i] = tokens[i].substr(1);
                    op.elements.push_back(tokens[i]);
                    isInBrackets = true;
                }
                else if (tokens[i].back() == ']')
                {
                    tokens[i] = tokens[i].substr(0, tokens[i].size() - 1);
                    op.elements.push_back(tokens[i]);
                    instr.operands.push_back(op);
                    op.elements.clear();
                    isInBrackets = false;
                }
                else if (tokens[i].back() == '!')
                {
                    tokens[i] = tokens[i].substr(0, tokens[i].size() - 2); // Remove ']!'
                    op.elements.push_back(tokens[i]);
                    op.write_back = true;
                    instr.operands.push_back(op);
                    isInBrackets = false;
                }
                else
                {
                    op.elements.push_back(tokens[i]);
                    if (!isInBrackets)
                    {
                        instr.operands.push_back(op);
                        op.elements.clear();
                    }
                }
            }
        }
        // clean the operands
        // Keep tokenp[0] as type, Update the rest as Operand
        instr.type = tokens[0];
        instructions.push_back(instr);
        // instructions.push_back({tokens[0], std::vector<std::string>(tokens.begin() + 1, tokens.end())});
    }
    std::cout << "instructions size: " << instructions.size() << std::endl;
    return true;
}

// z3::expr ACEE::getPathCondition()
// {

// }

inline z3::expr ACEE::getSymbolicRegister(const int &reg)
{
    return symbolicRegisters[reg];
}
// test

inline void ACEE::setSymbolicRegister(const int &reg, const z3::expr &expr)
{
    symbolicRegisters[reg] = expr;
}

inline int ACEE::getRegisterNumber(const std::string &reg)
{
    // Find the position of the first digit

    return stoi(reg.substr(1, reg.size() - 1));
}

inline z3::expr ACEE::lsl(const z3::expr &l, const z3::expr &r)
{
    z3::expr bvl = z3::int2bv(32, l);
    z3::expr bvr = z3::int2bv(32, r);
    z3::expr bvres = z3::shl(bvl, bvr);
    return z3::bv2int(bvres, true);
}

inline z3::expr ACEE::lsr(const z3::expr &l, const z3::expr &r)
{
    z3::expr bvl = z3::int2bv(32, l);
    z3::expr bvr = z3::int2bv(32, r);
    z3::expr bvres = z3::lshr(bvl, bvr);
    return z3::bv2int(bvres, true);
}

inline z3::expr ACEE::eor(const z3::expr &l, const z3::expr &r)
{
    z3::expr bvl = z3::int2bv(32, l);
    z3::expr bvr = z3::int2bv(32, r);
    z3::expr bvres = bvl ^ bvr;
    return z3::bv2int(bvres, true);
}

inline z3::expr ACEE::andd(const z3::expr &l, const z3::expr &r)
{
    z3::expr bvl = z3::int2bv(32, l);
    z3::expr bvr = z3::int2bv(32, r);
    z3::expr bvres = bvl & bvr;
    return z3::bv2int(bvres, true);
}

inline z3::expr ACEE::orr(const z3::expr &l, const z3::expr &r)
{
    z3::expr bvl = z3::int2bv(32, l);
    z3::expr bvr = z3::int2bv(32, r);
    z3::expr bvres = bvl | bvr;
    return z3::bv2int(bvres, true);
}

void ACEE::logLine(int i)
{
    if (logFile.is_open())
        logFile << "====================   Iteration " << i << "   ====================" << std::endl;
}

void ACEE::logTestInput(const std::vector<int> &inputRegisters)
{
    if (logFile.is_open())
    {
        std::vector<int> fin;
        fin.assign(4, -1);
        logFile << "Test Input: \n";
        for (int i = 0; i < 4; ++i)
        {
            if (inputRegisters[i])
            {
                fin[i] = registers[i];
                logFile << "\t"
                        << "r" << i << " = " << registers[i] << "\n";
            }
        }

        all_test_cases.push_back(fin);

        logFile << std::endl;
    }
}

void ACEE::logCode()
{
    if (logFile.is_open())
    {
        logFile << "Testing inside file " << p_path << std::endl;
        logFile << "\nCode Under Test\n";
        for (auto x : test_code)
        {
            std::stringstream s;
            s << "\tPC: " << std::setw(2) << std::left << x << "\t";
            s << std::setw(5) << std::left << instructions[x].type << " ";
            for (const auto &operand : instructions[x].operands)
            {
                s << std::setw(4) << std::left << operand.getString() + " ";
            }
            logFile << std::setw(30) << std::left << s.str() << '\n';
        }
        logFile << std::endl;

        n_test_code.assign(test_code.size(), "");
    }
}

void ACEE::logiCoverage(const std::unordered_set<int> &cov, int iteration)
{
    if (logFile.is_open())
    {
        // check if we made any new
        logFile << "Coverage:\n";
        logFile << "\tIndividual Coverage:\t" << cov.size() << "/" << test_code.size() << " lines covered (" << std::fixed << std::setprecision(4) << 100.0f * cov.size() / test_code.size() << "%)\n";
        logFile << "\tCumulative Coverage:\t" << coverage.size() << "/" << test_code.size() << " lines covered (" << std::fixed << std::setprecision(4) << 100.0f * coverage.size() / test_code.size() << "%)\n";

        for (int i = 0; i < test_code.size(); ++i)
        {
            std::stringstream s;
            int x = test_code[i];
            s << "\tPC: " << std::setw(2) << std::left << x << "\t";
            s << std::setw(5) << std::left << instructions[x].type << " ";
            for (const auto &operand : instructions[x].operands)
            {
                s << std::setw(4) << std::left << operand.getString() + " ";
            }

            if (cov.count(x))
            {
                n_test_code[i].append(std::to_string(iteration) + " ");
                logFile << std::setw(30) << std::left << s.str() << "X";
            }
            else
            {
                n_test_code[i].append("  ");
                logFile << std::setw(30) << std::left << s.str();
            }
            logFile << "\n";
        }
        logFile << '\n';
    }
}

void ACEE::lognCoverage()
{
    if (logFile.is_open())
    {
        logFile << "\n\n\n~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~\n ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~\n~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~\n";
        logFile << "Final Coverage: " << coverage.size() << "/" << test_code.size() << " lines covered (" << 100.0f * coverage.size() / test_code.size() << "%)\n";

        for (int i = 0; i < test_code.size(); ++i)
        {
            std::stringstream s;
            int x = test_code[i];
            s << "\tPC: " << std::setw(2) << std::left << x << "\t";
            s << std::setw(5) << std::left << instructions[x].type << " ";
            for (auto &operand : instructions[x].operands)
            {
                s << std::setw(4) << std::left << operand.getString() << " ";
            }
            logFile << std::setw(30) << std::left << s.str() << n_test_code[i] << '\n';
        }
        logFile << std::endl;

        logFile << "Final testcases:\n";
        for (auto x : all_test_cases)
        {
            for (int i = 0; i < 4; ++i)
            {
                if (inputRegisters[i])
                    logFile << std::setw(10) << "r" + std::to_string(i) + "=" + std::to_string(x[i]) << "\t";
            }
            logFile << '\n';
        }

        logFile << std::endl;
    }
}

void ACEE::logPathConstraintsV(const z3::expr_vector &constraints)
{
    if (logFile.is_open())
    {
        z3::expr_vector n(ctx);
        logFile << "Path Constraints: \n";
        for (int i = 0; i < constraints.size(); ++i)
        {
            n.push_back(constraints[i].simplify());
            logFile << '\t' << constraints[i].simplify().to_string() << '\n';
        }
        logFile << std::endl;
        path_constraints = n;
    }
}

void ACEE::logPathConstraints(const std::string &constraints)
{
    if (logFile.is_open())
    {
        logFile << "Path Constraints: " << constraints << std::endl;
    }
}

std::vector<int> ACEE::determineCodeUnderTest(int bPC)
{
    std::vector<int> result;

    while (bPC < instructions.size())
    {
        result.push_back(bPC);

        // recursively determine code under test
        if (instructions[bPC].type == "bl")
        {
            std::vector<int> r1 = determineCodeUnderTest(symbol2index[instructions[bPC].operands[0].getString()]);
            for (int x : r1)
                result.push_back(x);
        }

        else if (instructions[bPC].type == "bx" || instructions[bPC].type == "ace_end")
        {
            return result;
        }

        bPC++;
    }

    return result;
}
