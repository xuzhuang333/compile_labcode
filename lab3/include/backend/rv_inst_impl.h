#ifndef RV_INST_IMPL_H
#define RV_INST_IMPL_H

#include "backend/rv_def.h"

#include<fstream>
#include<vector>

#define ENDL std::endl

namespace rv {

struct rv_reg {
    rvREG type;
    int index;
    
    rv_reg();
    rv_reg(rvREG);
    rv_reg(rvREG, int);
    bool operator<(const rv_reg &t) const;
};
std::string toString(rv_reg reg);

struct rv_inst {
    rv_reg rd, rs1, rs2;            // operands of rv inst
    rvOPCODE op;                    // opcode of rv inst
    
    int imm;                        // optional, in immediate inst
    std::string label;              // optional, in beq/jarl inst
    std::string goto_label;
    std::vector<rv_inst> expand;

    rv_inst();
    rv_inst(rv_reg, rv_reg, rv_reg, rvOPCODE, int, std::string);

    void draw(std::ofstream &) const;
};


};

#endif