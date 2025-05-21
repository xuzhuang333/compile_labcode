#ifndef RV_DEF_H
#define RV_DEF_H

#include<string>

namespace rv {

// rv interger registers
//TODO3.3;
enum class rvREG {
    zero,// 硬编码0
    ra,// 返回地址
    sp,// 栈指针
    gp,// 全局指针
    tp,// 线程指针
    fp,// 栈帧指针(s0)
    t,// 临时寄存器
    s,// 保存寄存器
    a,// 参数寄存器
    ft,// 浮点临时寄存器
    fs,// 浮点保存寄存器
    fa,// 浮点参数寄存器
    frm,// 舍入位状态
};
std::string toString(rvREG r);  // implement this in ur own way

// rv32i instructions
// add instruction u need here!
enum class rvOPCODE {
    // RV32I Base Integer Instructions
    ADD, SUB, XOR, OR, AND, SLL, SRL, SRA, SLT, SLTU,       // arithmetic & logic
    ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI, SLTI, SLTIU,   // immediate
    LW, SW,                                                 // load & store
    BEQ, BNE, BLT, BGE, BLTU, BGEU,                         // conditional branch
    JAL, JALR,                                              // jump

    // RV32M Multiply Extension
    MUL, DIV, REM, 
    // RV32F / D Floating-Point Extensions

    // Pseudo Instructions
    LA, LI, MV, J, RET, NOP, BNEZ,CALL,
    SGT, SEQZ, SNEZ,

    // TODO3.10;
    STR, LTR,// store/load temporary registers, used as marks to reallocate, not actually existed

    FADD, FSUB, FMUL, FDIV,
    FLT, FLE, FGT, FGE, FEQ,
    FSW, FLW,
    F2I, I2F,
    FMV, FMVWX, FMVXW,
    CSRR, CSRW,
};
std::string toString(rvOPCODE op);  // implement this in ur own way


} // namespace rv



#endif