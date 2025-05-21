#include"backend/rv_def.h"

#include<cassert>

std::string rv::toString(rv::rvREG reg)
{
    switch (reg)
    {
    case rvREG::zero: return "zero";
    case rvREG::ra: return "ra";
    case rvREG::sp: return "sp";
    case rvREG::gp: return "gp";
    case rvREG::tp: return "tp";
    case rvREG::fp: return "fp";
    case rvREG::t: return "t";
    case rvREG::s: return "s";
    case rvREG::a: return "a";
    case rvREG::ft: return "ft";
    case rvREG::fs: return "fs";
    case rvREG::fa: return "fa";
    case rvREG::frm: return "frm";

    default:
        assert(0 && "invalid register type");
        break;
    }
    return "";  
}

std::string rv::toString(rv::rvOPCODE op)
{
    switch (op)
    {
    case rvOPCODE::ADD: return "add";
    case rvOPCODE::SUB: return "sub";
    case rvOPCODE::XOR: return "xor";
    case rvOPCODE::OR: return "or";
    case rvOPCODE::AND: return "and";
    case rvOPCODE::SLL: return "sll";
    case rvOPCODE::SRL: return "srl";
    case rvOPCODE::SRA: return "sra";
    case rvOPCODE::SLT: return "slt";
    case rvOPCODE::SLTU: return "sltu";
    case rvOPCODE::ADDI: return "addi";
    case rvOPCODE::XORI: return "xori";
    case rvOPCODE::ORI: return "ori";
    case rvOPCODE::ANDI: return "andi";
    case rvOPCODE::SLLI: return "slli";
    case rvOPCODE::SRLI: return "srli";
    case rvOPCODE::SRAI: return "srai";
    case rvOPCODE::SLTI: return "slti";
    case rvOPCODE::SLTIU: return "sltiu";
    case rvOPCODE::LW: return "lw";
    case rvOPCODE::SW: return "sw";
    case rvOPCODE::BEQ: return "beq";
    case rvOPCODE::BNE: return "bne";
    case rvOPCODE::BLT: return "blt";
    case rvOPCODE::BGE: return "bge";
    case rvOPCODE::BLTU: return "bltu";
    case rvOPCODE::BGEU: return "bgeu";
    case rvOPCODE::JAL: return "jal";
    case rvOPCODE::JALR: return "jalr";
    case rvOPCODE::LA: return "la";
    case rvOPCODE::LI: return "li";
    case rvOPCODE::MV: return "mv";
    case rvOPCODE::J: return "j";


    case rvOPCODE::MUL: return "mul";
    case rvOPCODE::DIV: return "div";
    case rvOPCODE::REM: return "rem";
    
    case rvOPCODE::RET: return "ret";
    case rvOPCODE::NOP: return "nop";
    case rvOPCODE::BNEZ: return "bnez";
    case rvOPCODE::CALL: return "call";
    
    case rvOPCODE::SGT: return "sgt";
    case rvOPCODE::SEQZ: return "seqz";
    case rvOPCODE::SNEZ: return "snez";

    case rvOPCODE::FADD: return "fadd.s";
    case rvOPCODE::FSUB: return "fsub.s";
    case rvOPCODE::FMUL: return "fmul.s";
    case rvOPCODE::FDIV: return "fdiv.s";
    case rvOPCODE::FLT: return "flt.s";
    case rvOPCODE::FLE: return "fle.s";
    case rvOPCODE::FGT: return "fgt.s";
    case rvOPCODE::FGE: return "fge.s";
    case rvOPCODE::FEQ: return "feq.s";
    case rvOPCODE::FSW: return "fsw";
    case rvOPCODE::FLW: return "flw";
    case rvOPCODE::F2I: return "fcvt.w.s";
    case rvOPCODE::I2F: return "fcvt.s.w"; 

    case rvOPCODE::FMV: return "fmv.s";
    case rvOPCODE::FMVWX: return "fmv.w.x";
    case rvOPCODE::FMVXW: return "fmv.x.w";

    case rvOPCODE::CSRR: return "csrr";
    case rvOPCODE::CSRW: return "csrw";

    default:
        assert(0 && "invalid opcode");
        break;
    }
    return "";
}