#ifndef GENERARATOR_H
#define GENERARATOR_H

#include "ir/ir.h"
#include "backend/context.h"

namespace backend {

struct Generator {
    ir::Program& program;         // the program to gen
    std::ofstream& fout;                 // output file
    int label_cnt;

    // TODO3.6;
    context cxt;

    Generator(ir::Program&, std::ofstream&);

    // reg allocate api
    rv::rv_reg getRd(ir::Operand);
    rv::rv_reg getRs(ir::Operand);
    rv::rv_reg fgetRd(ir::Operand);
    rv::rv_reg fgetRs(ir::Operand);

    // generate wrapper function
    // TODO3.2;
    Scope get_scope(ir::Operand);
    void get_arg(ir::Operand);
    void gen();
    void gen_glb();
    void gen_func(ir::Function &);
    void gen_instr(ir::Instruction &, ir::Function &);

    int f2li(float);
};

} // namespace backend


#endif