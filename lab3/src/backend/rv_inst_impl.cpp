#include<backend/rv_inst_impl.h>

#include<cassert>


rv::rv_reg::rv_reg(): index(-1) {}
rv::rv_reg::rv_reg(rv::rvREG _type): type(_type) {}
rv::rv_reg::rv_reg(rv::rvREG _type, int _index): type(_type), index(_index) {}
bool rv::rv_reg::operator<(const rv::rv_reg &t) const
{
    if(type == t.type)
    return index < t.index;
    return type < t.type;
};

rv::rv_inst::rv_inst() {}
rv::rv_inst::rv_inst(rv::rv_reg _rd, rv::rv_reg _rs1, rv::rv_reg _rs2, rv::rvOPCODE _op, int _imm, std::string _label):
    rd(_rd), rs1(_rs1), rs2(_rs2), op(_op), imm(_imm), label(_label), goto_label(std::string()){}

void rv::rv_inst::draw(std::ofstream &fout) const
{
    if(goto_label.size())
        fout << goto_label << ":" << ENDL;
    
    if(op == rvOPCODE::STR || op == rvOPCODE::LTR)
        return;

    // TODO3.9
    // 寄存器分配算法有时候能够检测出一些语义分析中难以识别的无用逻辑
    // 例如,TODO2.33中的终点寄存器统一有时候并不是必要的(譬如,if语句中有且仅有一个条件的时候其实并不需要mov)
    // 以下判断语句在测试样例62和64上能够很好的体现这一点(注释与否都不影响汇编程序的正确性)
    if((op == rvOPCODE::MV || op == rvOPCODE::FMV) && rs1.index == rd.index && rs1.type == rd.type)
        return;
    else if(op == rvOPCODE::ADDI && rs1.index == rd.index && rs1.type == rd.type && imm == 0)
        return;

    ;
    switch(op)
    {
        case rvOPCODE::ADD:
        case rvOPCODE::SUB:
        case rvOPCODE::MUL:
        case rvOPCODE::DIV:
        case rvOPCODE::REM:
        case rvOPCODE::XOR:
        case rvOPCODE::OR:
        case rvOPCODE::AND:
        case rvOPCODE::SLL:
        case rvOPCODE::SRL:
        case rvOPCODE::SRA:
        case rvOPCODE::SLT:
        case rvOPCODE::SLTU:
        case rvOPCODE::SGT:
        case rvOPCODE::FADD:
        case rvOPCODE::FSUB:
        case rvOPCODE::FMUL:
        case rvOPCODE::FDIV:
        case rvOPCODE::FLT:
        case rvOPCODE::FLE:
        case rvOPCODE::FGT:
        case rvOPCODE::FGE:
        case rvOPCODE::FEQ:
            fout << "\t" << toString(op) << "\t" << toString(rd) << ", " << toString(rs1) << ", " << toString(rs2) << ENDL;
            break;

        case rvOPCODE::SEQZ:
        case rvOPCODE::SNEZ:
        case rvOPCODE::MV:
        case rvOPCODE::FMV:
        case rvOPCODE::FMVWX:
        case rvOPCODE::FMVXW:
        case rvOPCODE::CSRR:
        case rvOPCODE::CSRW:
            fout << "\t" << toString(op) << "\t" << toString(rd) <<", " << toString(rs1) << ENDL;
            break;
        case rvOPCODE::F2I:
        case rvOPCODE::I2F:
            fout << "\t" << toString(op) << "\t" << toString(rd) <<", " << toString(rs1) << ENDL;
            break;

        case rvOPCODE::SW:
        case rvOPCODE::FSW:
            fout << "\t" << toString(op) << "\t" << toString(rs2) << ", " << imm << "(" << toString(rs1) << ")" << ENDL;
            break;
            
        case rvOPCODE::LW:
        case rvOPCODE::FLW:
            fout << "\t" << toString(op) << "\t" << toString(rd) << ", " << imm << "(" << toString(rs1) << ")" << ENDL;
            break;

        case rvOPCODE::ADDI:
        case rvOPCODE::XORI:
        case rvOPCODE::ORI:
        case rvOPCODE::ANDI:
        case rvOPCODE::SLLI:
        case rvOPCODE::SRLI:
        case rvOPCODE::SRAI:
        case rvOPCODE::SLTI:
        case rvOPCODE::SLTIU:
            fout << "\t" << toString(op) << "\t" << toString(rd) << ", " << toString(rs1) << ", " << imm << ENDL;
            break;

        case rvOPCODE::LI:
            fout << "\t" << toString(op) << "\t" << toString(rd) <<", " << imm << ENDL;
            break;

        case rvOPCODE::LA:
            fout << "\t" << toString(op) << "\t" << toString(rd) <<", " << label << ENDL;
            break;

        case rv::rvOPCODE::BNEZ:
            fout << "\t" << toString(op) << "\t" << toString(rs1) <<", " << label << ENDL;
            break;

        case rvOPCODE::CALL:
        case rvOPCODE::J:
            fout << "\t" << toString(op) << "\t" << label << ENDL;
            break;

        case rvOPCODE::RET:
        case rvOPCODE::NOP:
            fout << "\t" << toString(op) << ENDL;
            break;
        default:
            assert(0 && "invalid opcode");
            break;
    }
}


std::string rv::toString(rv::rv_reg reg)
{
    switch (reg.type)
    {
    case rv::rvREG::zero:
    case rv::rvREG::ra:
    case rv::rvREG::sp:
    case rv::rvREG::gp:
    case rv::rvREG::tp:
    case rv::rvREG::fp:
    case rv::rvREG::frm:
        return toString(reg.type);

    case rv::rvREG::s:
        assert(reg.index != 0);
    case rv::rvREG::t:
    case rv::rvREG::a:
    case rv::rvREG::ft:
    case rv::rvREG::fs:
    case rv::rvREG::fa:
        return toString(reg.type) + std::to_string(reg.index);
    default:
        assert(0 && "invalid register");
        break;
    }
    return "";
}