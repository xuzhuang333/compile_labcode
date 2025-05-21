// TODO3.0
// 人生建议:不要写编译优化=_=。评测机给出的用时是完整用时,包括项目的编译时间,运行时间(你可以试试在工程代码里加sleep然后
// 甩给评测机,Linux/Unix下的头文件是#include<unistd.h>),最后才是生成的汇编代码的运行时间。甚至后者的耗时占比大概只是个
// 零头(工程代码都有数千行,测试样例生成的汇编代码加起来基本也就这个数,而这两者的转换比恐怕不是1:1)。既然写死rs和rd然后
// 全过程load/store能显著降低工程代码量,那么你要做的就是加速大概率事件√

// 当然,写编译优化的过程能让人学到很多东西,而不管实现的到底好不好,我想这才是实验的意义所在:)

// 祝你好运!!
#include"backend/generator.h"

#include<assert.h>
#include<cstring>

#include<set>

using ir::Type;
using ir::Operand;
using ir::Operator;
using rv::rvREG;
using rv::rvOPCODE;
using rv::rv_reg;
using rv::rv_inst;

#define INT(x) std::stoi(x)

#define ARG_REG_SIZE 8
#define SAVED_REG_SIZE 11
#define ARG(opd) (cxt.allocator.count(opd.name) && cxt.allocator[opd.name].type == rvREG::a)
#define ARG_OVERFLOW(opd) (ARG(opd) && cxt.allocator[opd.name].index >= ARG_REG_SIZE)
#define ARG_OFFSET(opd) ((cxt.allocator[opd.name].index - ARG_REG_SIZE) * 4)

#define FARG_REG_SIZE 8
#define FSAVED_REG_SIZE 12
#define FARG(opd) (cxt.allocator.count(opd.name) && cxt.allocator[opd.name].type == rvREG::fa)
#define FARG_OVERFLOW(opd) (FARG(opd) && cxt.allocator[opd.name].index >= FARG_REG_SIZE)
#define FARG_OFFSET(opd) ((cxt.allocator[opd.name].index - FARG_REG_SIZE + std::max(cxt.argc - ARG_REG_SIZE, 0)) * 4)

backend::Generator::Generator(ir::Program& p, std::ofstream& f): program(p), fout(f), label_cnt(0) {}

int backend::Generator::f2li(float fval)
{
    int val;
    std::memcpy(&val, &fval, 4);
    return val;
}

// TODO3.5
backend::Scope backend::Generator::get_scope(ir::Operand opd)
{
    if(opd.name.find('_') == opd.name.npos)
    return (opd.type == Type::IntLiteral || opd.type == Type::FloatLiteral)? Scope::Literal : Scope::Temporary;
    return (opd.name.back() == '0') ? Scope::Global : Scope::Local;
}

rv_reg backend::Generator::getRs(ir::Operand opd)
{
    assert(opd.type == Type::IntPtr || opd.type == Type::FloatPtr || opd.type == Type::Int || opd.type == Type::IntLiteral);

    auto scp = get_scope(opd);
    if(scp == Scope::Global)
    {
        auto rs = rv_reg(rvREG::t, cxt.tmp++);
        cxt.add_ins(rvOPCODE::LA, rs, opd.name);
        cxt.add_ins(rvOPCODE::LW, rs, rs, 0);
        return rs;
    }
    else if(scp == Scope::Literal)
    {
        // TODO3.8;
        if(INT(opd.name) == 0)// 立即数0
            return rv_reg(rvREG::zero);

        auto rs = rv_reg(rvREG::t, cxt.tmp++);
        cxt.add_ins(rvOPCODE::LI, rs, {}, INT(opd.name));
        return rs;
    }
    else
    {        
        assert(cxt.allocator.count(opd.name));
        if(ARG_OVERFLOW(opd))// 参数溢出
        {
            auto rs = rv_reg(rvREG::t, cxt.tmp++);
            cxt.add_ins(rvOPCODE::LW, rs, rv_reg(rvREG::fp), ARG_OFFSET(opd));
            return rs;
        }
        return cxt.allocator[opd.name];
    }
}

rv_reg backend::Generator::fgetRs(ir::Operand opd)
{
    assert(opd.type == Type::Float || opd.type == Type::FloatLiteral);

    auto scp = get_scope(opd);
    if(scp == Scope::Global)
    {
        auto rs = rv_reg(rvREG::ft, cxt.ftmp++);
        cxt.add_ins(rvOPCODE::LA, rs, opd.name);
        cxt.add_ins(rvOPCODE::FLW, rs, rs, 0);
        return rs;
    }
    else if(scp == Scope::Literal)
    {
        auto rs = rv_reg(rvREG::t, cxt.tmp++);
        auto frs = rv_reg(rvREG::ft, cxt.ftmp++);
        cxt.add_ins(rvOPCODE::LI, rs, {}, f2li(std::stof(opd.name)));
        cxt.add_ins(rvOPCODE::FMVWX, frs, rs, {});
        return frs;
    }
    else
    {        
        assert(cxt.allocator.count(opd.name));
        if(FARG_OVERFLOW(opd))
        {
            auto rs = rv_reg(rvREG::ft, cxt.ftmp++);// 溢出参数
            cxt.add_ins(rvOPCODE::FLW, rs, rv_reg(rvREG::fp), FARG_OFFSET(opd));
            return rs;
        }
        return cxt.allocator[opd.name];
    }
}

rv_reg backend::Generator::getRd(ir::Operand opd)
{
    assert(opd.type == Type::Int || opd.type == Type::IntPtr || opd.type == Type::FloatPtr);

    auto scp = get_scope(opd);
    assert(scp != Scope::Literal);
    
    if(scp == Scope::Global)
        return rv_reg(rvREG::t, cxt.tmp++);
    else
    {
        if(cxt.allocator.count(opd.name))
            return cxt.allocator[opd.name];
        if(scp == Scope::Local)
        {
            assert(cxt.local < SAVED_REG_SIZE);
            return cxt.allocator[opd.name] = rv_reg(rvREG::s, ++cxt.local);
        }
        return cxt.allocator[opd.name] = rv_reg(rvREG::t, cxt.tmp++);
    }
}

rv_reg backend::Generator::fgetRd(ir::Operand opd)
{
    assert(opd.type == Type::Float);

    auto scp = get_scope(opd);
    assert(scp != Scope::Literal);
    
    if(scp == Scope::Global)
        return rv_reg(rvREG::ft, cxt.ftmp++);
    else
    {
        if(cxt.allocator.count(opd.name))
            return cxt.allocator[opd.name];
        if(scp == Scope::Local)
        {
            assert(cxt.flocal < FSAVED_REG_SIZE);
            return cxt.allocator[opd.name] = rv_reg(rvREG::fs, cxt.flocal++);
        }
        return cxt.allocator[opd.name] = rv_reg(rvREG::ft, cxt.ftmp++);
    }
}

void backend::Generator::get_arg(ir::Operand opd)
{
    if(opd.type == Type::IntPtr || opd.type == Type::FloatPtr || opd.type == Type::Int || opd.type == Type::IntLiteral)
        cxt.allocator[opd.name] = rv_reg(rvREG::a, cxt.argc++);
    else
        cxt.allocator[opd.name] = rv_reg(rvREG::fa, cxt.fargc++);
}

void backend::Generator::gen_func(ir::Function &func)
{
    cxt.clear();
    
    fout << "\t.global\t" << func.name << ENDL;
    fout << "\t.type\t" << func.name << ", @function" << ENDL;
    fout << func.name << ":" << ENDL;
    
    for(auto &opd : func.ParameterList)
    {
        assert(opd.type != Type::null);
        get_arg(opd);
    }    

    for(int i = 0 ; i < int(func.InstVec.size()) ; i++)
    {
        auto &ins = func.InstVec[i]; 
        if(ins->op == Operator::alloc)
        {
            cxt.stack[ins->des.name] = cxt.arr_size;
            cxt.arr_size += INT(ins->op1.name) * 4;
        }
        else
        {
            gen_instr(*ins, func);
            if(ins->op == Operator::_goto)// 标记所有的goto语块
                cxt.goto_list.push_back(i);
        }
        cxt.prefixsum.push_back(cxt.pseudocode.size());
    }

    for(auto idx : cxt.goto_list)// 计算所有跳转指令的偏移量,并在目标位置设置标签
    {
        auto &ins = cxt.pseudocode[cxt.prefixsum[idx] - 1];// 跳转指令总是goto语块的最后一条指令
        int target = (idx + ins.imm > 0) ? cxt.prefixsum[idx + ins.imm - 1] : 0;
        auto &goto_label = cxt.pseudocode[target].goto_label;
        if(goto_label.size())
            ins.label = goto_label;
        else
            ins.label = goto_label = "goto_label" + std::to_string(label_cnt++);
    }

    cxt.reallocate();// 寄存器重分配

    int top = cxt.arr_size;
    int frame_size = cxt.arr_size + (cxt.local + cxt.flocal + 2) * 4;// 局部数组 + 保存寄存器 + fp寄存器 + ra寄存器

    context ini, rel;
    ini.add_ins(rvOPCODE::MV, rv_reg(rvREG::fp), rv_reg(rvREG::sp), {});

    ini.add_ins(rvOPCODE::ADDI, rv_reg(rvREG::sp), rv_reg(rvREG::sp), -frame_size);
    ini.add_ins(rvOPCODE::SW, {}, rv_reg(rvREG::sp), rv_reg(rvREG::fp), top);
    top += 4;
    ini.add_ins(rvOPCODE::SW, {}, rv_reg(rvREG::sp), rv_reg(rvREG::ra), top);
    top += 4;    
    for(int i = 1 ; i <= cxt.local ; i++)
    {
        ini.add_ins(rvOPCODE::SW, {}, rv_reg(rvREG::sp), rv_reg(rvREG::s, i), top);
        top += 4;
    }
    for(int i = 0 ; i < cxt.flocal ; i++)
    {
        ini.add_ins(rvOPCODE::FSW, {}, rv_reg(rvREG::sp), rv_reg(rvREG::fs, i), top);
        top += 4;
    }

    for(int i = cxt.flocal - 1; i >= 0 ; i--)
    {
        top -= 4;
        rel.add_ins(rvOPCODE::FLW, rv_reg(rvREG::fs, i), rv_reg(rvREG::sp), top);
    }
    for(int i = cxt.local; i >= 1 ; i--)
    {
        top -= 4;
        rel.add_ins(rvOPCODE::LW, rv_reg(rvREG::s, i), rv_reg(rvREG::sp), top);
    }
    top -= 4;
    rel.add_ins(rvOPCODE::LW, rv_reg(rvREG::ra), rv_reg(rvREG::sp), top);

    rel.add_ins(rvOPCODE::MV, rv_reg(rvREG::sp), rv_reg(rvREG::fp), {});
        
    for(auto &ins : ini.pseudocode)
        ins.draw(fout);
    for(auto &ins : cxt.pseudocode)
    {
        if(ins.op == rvOPCODE::STR || ins.op == rvOPCODE::LTR)
        {
            ins.draw(fout);
            for(auto &ins : ins.expand)
                ins.draw(fout);
        }
        else
        {
            if(ins.op == rvOPCODE::RET)
            {
                for(auto &ins : rel.pseudocode)
                    ins.draw(fout);
            }
            ins.draw(fout);
        }
    }

    fout << ENDL;
}
void backend::Generator::gen_instr(ir::Instruction &ins, ir::Function &func)
{
    auto &op1 = ins.op1, &op2 = ins.op2, &des = ins.des;
    
    auto des_scope = get_scope(des);
    

    // TODO3.13.1
    // 舍入模式切换
    rv_reg old;
    if(ins.op == Operator::cvt_i2f || ins.op == Operator::cvt_f2i)
    {
    	old = rv_reg(rvREG::t, cxt.tmp++);
    	auto now = rv_reg(rvREG::t, cxt.tmp++); 
        cxt.add_ins(rvOPCODE::CSRR, old, rv_reg(rvREG::frm), {});
        cxt.add_ins(rvOPCODE::LI, now, {}, 1);
        cxt.add_ins(rvOPCODE::CSRW, rv_reg(rvREG::frm), now, {});
    }
    
    switch(ins.op)
    {
        case Operator::def:
            assert(cxt.allocator.count(des.name) == 0);
            switch(des_scope)
            {
                case Scope::Local:// 局部变量声明
                    switch(get_scope(op1))
                    {
                        case Scope::Global:
                            cxt.add_ins(rvOPCODE::LA, getRd(des), op1.name);
                            cxt.add_ins(rvOPCODE::LW, getRd(des), getRs(des), 0);
                            break;
                        case Scope::Local:
                            if(ARG_OVERFLOW(op1))
                                cxt.add_ins(rvOPCODE::LW, getRd(des), rv_reg(rvREG::fp), ARG_OFFSET(op1));
                            else
                                cxt.add_ins(rvOPCODE::MV, getRd(des), getRs(op1), {});
                            break;
                        case Scope::Temporary:
                            cxt.add_ins(rvOPCODE::MV, getRd(des), getRs(op1), {});
                            break;
                        case Scope::Literal:
                            cxt.add_ins(rvOPCODE::LI, getRd(des), {}, INT(op1.name));
                            break;
                        default:
                            break;
                    }
                    break;
                case Scope::Temporary:// 立即数加载
                    cxt.add_ins(rvOPCODE::LI, getRd(des), {}, INT(op1.name));
                    break;
                default:
                    assert(0 && "invalid operator: def");
                    break;
            } 
            break;

        case Operator::mov:
            switch(des_scope)
            {
                case Scope::Global:// 全局变量赋值
                    {
                        auto base = rv_reg(rvREG::t, cxt.tmp++);
                        cxt.add_ins(rvOPCODE::LA, base, des.name);
                        cxt.add_ins(rvOPCODE::SW,{}, base, getRs(op1), 0);
                    }
                    break;
                case Scope::Local:// 局部变量赋值
                    assert(cxt.allocator.count(des.name));
                    // 赋值的目标变量是溢出参数
                    if(ARG_OVERFLOW(des))
                    {
                        rv_reg rs;
                        switch(get_scope(op1))
                        {
                            case Scope::Global:
                                rs = rv_reg(rvREG::t, cxt.tmp++);
                                cxt.add_ins(rvOPCODE::LA, rs, op1.name);
                                cxt.add_ins(rvOPCODE::LW, rs, rs, 0);
                                break;
                            case Scope::Local:
                            case Scope::Temporary:
                                rs = getRs(op1);
                                break;
                            case Scope::Literal:
                                rs = rv_reg(rvREG::t, cxt.tmp++);
                                cxt.add_ins(rvOPCODE::LI, rs, {}, INT(op1.name));
                                break;
                            default:
                                break;
                        }
                        cxt.add_ins(rvOPCODE::SW, {}, rv_reg(rvREG::fp), rs, ARG_OFFSET(des));
                    }
                    else
                    {
                        switch(get_scope(op1))
                        {
                            case Scope::Global:
                                cxt.add_ins(rvOPCODE::LA, getRd(des), op1.name);
                                cxt.add_ins(rvOPCODE::LW, getRd(des), getRs(des), 0);
                                break;
                            case Scope::Local:
                            case Scope::Temporary:
                                cxt.add_ins(rvOPCODE::MV, getRd(des), getRs(op1), {});
                                break;
                            case Scope::Literal:
                                cxt.add_ins(rvOPCODE::LI, getRd(des), {}, INT(op1.name));
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case Scope::Temporary:// 临时变量赋值
                    switch(get_scope(op1))
                    {
                        case Scope::Global:// 将全局变量,局部变量加载到临时寄存器以避免修改变量本身
                            cxt.add_ins(rvOPCODE::LA, getRd(des), op1.name);
                            cxt.add_ins(rvOPCODE::LW, getRd(des), getRs(des), 0);
                            break;
                        case Scope::Local:
                            if(ARG_OVERFLOW(op1))
                                cxt.add_ins(rvOPCODE::LW, getRd(des), rv_reg(rvREG::fp), ARG_OFFSET(op1));
                            else
                                cxt.add_ins(rvOPCODE::MV, getRd(des), getRs(op1), {});
                            break;
                        case Scope::Temporary:// 在与或计算时将计算结果统一到某个临时寄存器,见TODO2.33
                            cxt.add_ins(rvOPCODE::MV, getRd(des), getRs(op1), {});
                            break;
                        default:
                            break;
                    }   
                    break;
                default:
                    assert(0 && "invalid operator: mov");
                    break;
            }
            break;

        // TODO3.7;
        // 对所有计算类的IR指令,des_scope = Scope::Temporary
        case Operator::addi:
            assert(op2.type == Type::IntLiteral);
            cxt.add_ins(rvOPCODE::ADDI, getRd(des), getRs(op1), INT(op2.name));
            break;
        case Operator::add:
            cxt.add_ins(rvOPCODE::ADD, getRd(des), getRs(op1), getRs(op2));
            break;
        case Operator::sub:
            cxt.add_ins(rvOPCODE::SUB, getRd(des), getRs(op1), getRs(op2));
            break;
        case Operator::mul:
            cxt.add_ins(rvOPCODE::MUL, getRd(des), getRs(op1), getRs(op2));
            break;
        case Operator::div:
            cxt.add_ins(rvOPCODE::DIV, getRd(des), getRs(op1), getRs(op2));
            break;
        case Operator::mod:
            cxt.add_ins(rvOPCODE::REM, getRd(des), getRs(op1), getRs(op2));
            break;
        case Operator::lss:
            cxt.add_ins(rvOPCODE::SLT, getRd(des), getRs(op1), getRs(op2));
            break;
        case Operator::leq:
            cxt.add_ins(rvOPCODE::SGT, getRd(des), getRs(op1), getRs(op2));
            cxt.add_ins(rvOPCODE::XORI, getRd(des), getRs(des), 1);
            break;
        case Operator::gtr:
            cxt.add_ins(rvOPCODE::SGT, getRd(des), getRs(op1), getRs(op2));
            break;
        case Operator::geq:
            cxt.add_ins(rvOPCODE::SLT, getRd(des), getRs(op1), getRs(op2));
            cxt.add_ins(rvOPCODE::XORI, getRd(des), getRs(des), 1);    
            break;
        case Operator::eq:
            cxt.add_ins(rvOPCODE::XOR, getRd(des), getRs(op1), getRs(op2));
            cxt.add_ins(rvOPCODE::SEQZ, getRd(des), getRs(des), {});
            break;
        case Operator::neq:
            cxt.add_ins(rvOPCODE::XOR, getRd(des), getRs(op1), getRs(op2));
            cxt.add_ins(rvOPCODE::SNEZ, getRd(des), getRs(des), {});
            break;
        case Operator::_or:
            cxt.add_ins(rvOPCODE::OR, getRd(des), getRs(op1), getRs(op2));
            cxt.add_ins(rvOPCODE::SNEZ, getRd(des), getRs(des), {});
            break;
        case Operator::_and:
            cxt.add_ins(rvOPCODE::SNEZ, getRd(op1), getRs(op1), {});
            cxt.add_ins(rvOPCODE::SNEZ, getRd(op2), getRs(op2), {});
            cxt.add_ins(rvOPCODE::AND, getRd(des), getRs(op1), getRs(op2));
            break;
        
        case Operator::getptr:
            if(get_scope(op1) == Scope::Global)// 全局数组
            {
                cxt.add_ins(rvOPCODE::LA, getRd(des), op1.name);
                if(op2.type == Type::IntLiteral)
                    cxt.add_ins(rvOPCODE::ADDI, getRd(des), getRs(des), INT(op2.name) * 4);
                else
                {
                    cxt.add_ins(rvOPCODE::SLLI, getRd(op2), getRs(op2), 2);
                    cxt.add_ins(rvOPCODE::ADD, getRd(des), getRs(des), getRs(op2));
                }
            }
            else// 局部数组
            {
                if(ARG(op1))
                {
                    if(ARG_OVERFLOW(op1))
                        cxt.add_ins(rvOPCODE::LW, getRd(des), rv_reg(rvREG::fp), ARG_OFFSET(op1));
                    else
                        cxt.add_ins(rvOPCODE::MV, getRd(des), getRs(op1), {});

                    if(op2.type == Type::IntLiteral)
                        cxt.add_ins(rvOPCODE::ADDI, getRd(des), getRs(des), INT(op2.name) * 4);
                    else
                    {
                        cxt.add_ins(rvOPCODE::SLLI, getRd(op2), getRs(op2), 2);
                        cxt.add_ins(rvOPCODE::ADD, getRd(des), getRs(des), getRs(op2));
                    }

                    if(ARG_OVERFLOW(op1))
                        cxt.add_ins(rvOPCODE::SW, {}, rv_reg(rvREG::fp), getRs(des), ARG_OFFSET(des));
                }
                else
                {
                    assert(cxt.stack.count(op1.name));
                    cxt.add_ins(rvOPCODE::ADDI, getRd(des), rv_reg(rvREG::sp), cxt.stack[op1.name]);
                    if(op2.type == Type::IntLiteral)
                        cxt.add_ins(rvOPCODE::ADDI, getRd(des), getRs(des), INT(op2.name) * 4);
                    else
                    {
                        cxt.add_ins(rvOPCODE::SLLI, getRd(op2), getRs(op2), 2);
                        cxt.add_ins(rvOPCODE::ADD, getRd(des), getRs(des), getRs(op2));
                    }
                }
            }
            break;
        case Operator::store:
            if(get_scope(op1) == Scope::Global)// 全局数组
            {
                auto base = rv_reg(rvREG::t, cxt.tmp++);
                cxt.add_ins(rvOPCODE::LA, base, op1.name);
                if(op2.type == Type::IntLiteral)
                {
                    if(op1.type == Type::IntPtr)
                        cxt.add_ins(rvOPCODE::SW, {}, base, getRs(des), INT(op2.name) * 4);
                    else
                        cxt.add_ins(rvOPCODE::FSW, {}, base, fgetRs(des), INT(op2.name) * 4);
                }
                else
                {
                    cxt.add_ins(rvOPCODE::SLLI, getRd(op2), getRs(op2), 2);
                    cxt.add_ins(rvOPCODE::ADD, base, base, getRs(op2));
                    if(op1.type == Type::IntPtr)
                        cxt.add_ins(rvOPCODE::SW, {}, base, getRs(des), 0);
                    else
                        cxt.add_ins(rvOPCODE::FSW, {}, base, fgetRs(des), 0);
                }
            }
            else// 局部数组
            {
                auto base = rv_reg(rvREG::t, cxt.tmp++);
                if(ARG(op1))
                {
                    if(ARG_OVERFLOW(op1))// 存储的目标数组指针是溢出参数
                        cxt.add_ins(rvOPCODE::LW, base, rv_reg(rvREG::fp), ARG_OFFSET(op1));
                    else
                        cxt.add_ins(rvOPCODE::MV, base, getRs(op1), {});
                    
                    if(op2.type == Type::IntLiteral)
                    {
                        if(op1.type == Type::IntPtr)
                            cxt.add_ins(rvOPCODE::SW, {}, base, getRs(des), INT(op2.name) * 4);
                        else
                            cxt.add_ins(rvOPCODE::FSW, {}, base, fgetRs(des), INT(op2.name) * 4);
                    }
                    else
                    {
                        cxt.add_ins(rvOPCODE::SLLI, getRd(op2), getRs(op2), 2);
                        cxt.add_ins(rvOPCODE::ADD, base, base, getRs(op2));
                        if(op1.type == Type::IntPtr)
                            cxt.add_ins(rvOPCODE::SW, {}, base, getRs(des), 0);
                        else
                            cxt.add_ins(rvOPCODE::FSW, {}, base, fgetRs(des), 0);
                    }
                }
                else
                {
                    cxt.add_ins(rvOPCODE::ADDI, base, rv_reg(rvREG::sp), cxt.stack[op1.name]);
                    if(op2.type == Type::IntLiteral)
                    {
                        if(op1.type == Type::IntPtr)
                            cxt.add_ins(rvOPCODE::SW, {}, base, getRs(des), INT(op2.name) * 4);
                        else
                            cxt.add_ins(rvOPCODE::FSW, {}, base, fgetRs(des), INT(op2.name) * 4);
                    }
                    else
                    {
                        cxt.add_ins(rvOPCODE::SLLI, getRd(op2), getRs(op2), 2);
                        cxt.add_ins(rvOPCODE::ADD, base, base, getRs(op2));
                        if(op1.type == Type::IntPtr)
                            cxt.add_ins(rvOPCODE::SW, {}, base, getRs(des), cxt.stack[op1.name]);
                        else
                            cxt.add_ins(rvOPCODE::FSW, {}, base, fgetRs(des), cxt.stack[op1.name]);
                    }
                }
            }
            break;
        case Operator::load:
            if(get_scope(op1) == Scope::Global)// 全局数组
            {
                auto base = rv_reg(rvREG::t, cxt.tmp++);
                cxt.add_ins(rvOPCODE::LA, base, op1.name);
                if(op2.type == Type::IntLiteral)
                {
                    if(op1.type == Type::IntPtr)
                        cxt.add_ins(rvOPCODE::LW, getRd(des), base, INT(op2.name) * 4);
                    else
                        cxt.add_ins(rvOPCODE::FLW, fgetRd(des), base, INT(op2.name) * 4);
                }
                else
                {
                    cxt.add_ins(rvOPCODE::SLLI, getRd(op2), getRs(op2), 2);
                    cxt.add_ins(rvOPCODE::ADD, base, base, getRs(op2));
                    if(op1.type == Type::IntPtr)
                        cxt.add_ins(rvOPCODE::LW, getRd(des), base, 0);
                    else
                        cxt.add_ins(rvOPCODE::FLW, fgetRd(des), base, 0);
                }
            }
            else// 局部数组
            {
                if(ARG(op1))
                {
                    rv_reg base = rv_reg(rvREG::t, cxt.tmp++);
                    if(ARG_OVERFLOW(op1))// 存储的目标数组指针是溢出参数
                        cxt.add_ins(rvOPCODE::LW, base, rv_reg(rvREG::fp), ARG_OFFSET(op1));
                    else
                        cxt.add_ins(rvOPCODE::MV, base, getRs(op1), {});
                    
                    if(op2.type == Type::IntLiteral)
                    {
                        if(op1.type == Type::IntPtr)
                            cxt.add_ins(rvOPCODE::LW, getRd(des), base, INT(op2.name) * 4);
                        else
                            cxt.add_ins(rvOPCODE::FLW, fgetRd(des), base, INT(op2.name) * 4);
                    }
                    else
                    {
                        cxt.add_ins(rvOPCODE::SLLI, getRd(op2), getRs(op2), 2);
                        cxt.add_ins(rvOPCODE::ADD, base, base, getRs(op2));
                        if(op1.type == Type::IntPtr)
                            cxt.add_ins(rvOPCODE::LW, getRd(des), base, 0);
                        else
                            cxt.add_ins(rvOPCODE::FLW, fgetRd(des), base, 0);
                    }
                }
                else
                {
                    auto base = rv_reg(rvREG::t, cxt.tmp++);
                    cxt.add_ins(rvOPCODE::ADDI, base, rv_reg(rvREG::sp), cxt.stack[op1.name]);
                    if(op2.type == Type::IntLiteral)
                    {
                        if(op1.type == Type::IntPtr)
                            cxt.add_ins(rvOPCODE::LW, getRd(des), base, INT(op2.name) * 4);
                        else
                            cxt.add_ins(rvOPCODE::FLW, fgetRd(des), base, INT(op2.name) * 4);
                    }
                    else
                    {
                        cxt.add_ins(rvOPCODE::SLLI, getRd(op2), getRs(op2), 2);
                        cxt.add_ins(rvOPCODE::ADD, base, base, getRs(op2));
                        if(op1.type == Type::IntPtr)
                            cxt.add_ins(rvOPCODE::LW, getRd(des), base, cxt.stack[op1.name]);
                        else
                            cxt.add_ins(rvOPCODE::FLW, fgetRd(des), base, cxt.stack[op1.name]);
                    }
                }

            }
            break;
        
        case Operator::_goto:
            switch(op1.type)
            {
                case Type::Int:
                    cxt.add_ins(rvOPCODE::BNEZ, {}, getRs(op1), INT(des.name));
                    break;

                case Type::Float:
                    cxt.add_ins(rvOPCODE::FMVXW, rv_reg(rvREG::t, cxt.tmp), fgetRs(op1), {});
                    cxt.add_ins(rvOPCODE::BNEZ, {}, rv_reg(rvREG::t, cxt.tmp++), INT(des.name));
                    break;
                case Type::null:
                    cxt.add_ins(rvOPCODE::J, {}, {}, INT(des.name));
                    break;
                default:
                    assert(0 && "invalid cond in goto");
                    break;
            }
            break;

        case Operator::__unuse__:
            cxt.add_ins(rvOPCODE::NOP, {});
            break;
        
        case Operator::call:
            {
                auto call = dynamic_cast<ir::CallInst*>(&ins);
                if(call->op1.name != "_global")// 全局函数只用于存储数据段信息
                {
                    int a_cnt = 0, fa_cnt = 0;
                    context mva, ovf;
                    for(auto &opd : call->argumentList)
                    if(opd.type == Type::IntPtr || opd.type == Type::FloatPtr || opd.type == Type::Int || opd.type == Type::IntLiteral)
                    {
                        if(a_cnt < ARG_REG_SIZE)
                        {
                            auto rd = rv_reg(rvREG::a, a_cnt);
                            switch(get_scope(opd))
                            {
                                case Scope::Global:
                                    mva.add_ins(rvOPCODE::LA, rd, opd.name);
                                    mva.add_ins(rvOPCODE::LW, rd, rd, 0);
                                    break;
                                case Scope::Local:
                                case Scope::Temporary:
                                    mva.add_ins(rvOPCODE::MV, rd, getRs(opd), {});
                                    break;
                                case Scope::Literal:
                                    mva.add_ins(rvOPCODE::LI, rd, {}, INT(opd.name));
                                default:
                                    break;
                            } 
                        }
                        else
                        {
                            rv_reg rs;
                            switch(get_scope(opd))
                            {
                                case Scope::Global:
                                    rs = rv_reg(rvREG::t, cxt.tmp++);
                                    ovf.add_ins(rvOPCODE::LA, rs, opd.name);
                                    ovf.add_ins(rvOPCODE::LW, rs, rs, 0);        
                                    break;
                                case Scope::Local:
                                case Scope::Temporary:
                                    rs = getRs(opd);
                                    break;
                                case Scope::Literal:
                                    rs = rv_reg(rvREG::t, cxt.tmp++);
                                    ovf.add_ins(rvOPCODE::LI, rs, {}, INT(opd.name));
                                default:
                                    break;
                            }
                            ovf.add_ins(rvOPCODE::SW, {}, rv_reg(rvREG::sp), rs, (a_cnt - ARG_REG_SIZE) * 4);
                        }
                        a_cnt++;
                    }

                    int arg_ovf_cnt = std::max(a_cnt - ARG_REG_SIZE, 0);
                    for(auto &opd : call->argumentList)
                    if(opd.type == Type::Float || opd.type == Type::FloatLiteral)
                    {
                        if(fa_cnt < FARG_REG_SIZE)
                        {
                            auto rd = rv_reg(rvREG::fa, fa_cnt);
                            switch(get_scope(opd))
                            {
                                case Scope::Global:
                                    mva.add_ins(rvOPCODE::LA, rv_reg(rvREG::t, cxt.tmp), opd.name);
                                    mva.add_ins(rvOPCODE::FLW, rd, rv_reg(rvREG::t, cxt.tmp++), 0);
                                    break;
                                case Scope::Local:
                                case Scope::Temporary:
                                    mva.add_ins(rvOPCODE::FMV, rd, fgetRs(opd), {});
                                    break;
                                case Scope::Literal:
                                    mva.add_ins(rvOPCODE::LI, rv_reg(rvREG::t, cxt.tmp), {}, f2li(std::stof(opd.name)));
                                    mva.add_ins(rvOPCODE::FMVWX, rd, rv_reg(rvREG::t, cxt.tmp++), {});
                                default:
                                    break;
                            } 
                        }
                        else
                        {
                            rv_reg frs;
                            switch(get_scope(opd))
                            {
                                case Scope::Global:
                                    ovf.add_ins(rvOPCODE::LA, rv_reg(rvREG::t, cxt.tmp), opd.name);
                                    ovf.add_ins(rvOPCODE::FLW, frs, rv_reg(rvREG::t, cxt.tmp++), 0);        
                                    break;
                                case Scope::Local:
                                case Scope::Temporary:
                                    frs = fgetRs(opd);
                                    break;
                                case Scope::Literal:
                                    cxt.add_ins(rvOPCODE::LI, rv_reg(rvREG::t, cxt.tmp), {}, f2li(std::stof(op1.name)));
                                    cxt.add_ins(rvOPCODE::FMVWX, frs, rv_reg(rvREG::t, cxt.tmp++), {});
                                default:
                                    break;
                            }
                            ovf.add_ins(rvOPCODE::FSW, {}, rv_reg(rvREG::sp), frs, (fa_cnt - FARG_REG_SIZE + arg_ovf_cnt) * 4);
                        }
                        fa_cnt++;
                    }

                    int ret_save = (func.returnType == Type::IntPtr || func.returnType == Type::FloatPtr || func.returnType == Type::Int || func.returnType == Type::IntLiteral);
                    int fret_save = (func.returnType == Type::Float || func.returnType == Type::FloatLiteral);
                    int arg_save = std::min(std::min(cxt.argc, std::max(a_cnt, ret_save)), ARG_REG_SIZE);
                    int farg_save = std::min(std::min(cxt.fargc, std::max(fa_cnt, fret_save)), FARG_REG_SIZE);
                    
                    int arg_size = (arg_save + farg_save) * 4;
                    // TODO3.11
                    if(arg_size)// 先保存参数寄存器
                    {
                        cxt.add_ins(rvOPCODE::ADDI, rv_reg(rvREG::sp), rv_reg(rvREG::sp), -arg_size);
                        for(int i = 0 ; i < arg_save ; i++)
                            cxt.add_ins(rvOPCODE::SW, {}, rv_reg(rvREG::sp), rv_reg(rvREG::a, i), i * 4);
                        for(int i = 0 ; i < farg_save ; i++)
                            cxt.add_ins(rvOPCODE::FSW, {}, rv_reg(rvREG::sp), rv_reg(rvREG::fa, i), i * 4);
                    }

                    // 然后修改参数寄存器s
                    for(auto &ins : mva.pseudocode)
                        cxt.pseudocode.push_back(ins);

                    // 然后保存临时寄存器
                    cxt.add_ins(rvOPCODE::STR, {});

                    // 最后溢出参数压栈
                    int ovf_size = (arg_ovf_cnt + std::max(fa_cnt - FARG_REG_SIZE, 0)) * 4;
                    if(ovf_size)
                    {
                        cxt.add_ins(rvOPCODE::ADDI, rv_reg(rvREG::sp), rv_reg(rvREG::sp), -ovf_size);
                        for(auto &ins : ovf.pseudocode)
                            cxt.pseudocode.push_back(ins);
                    }
                    
                    // TODO3.12
                    // 神秘の数,不把栈这样调一下就不对,调了就对了,甚至再给大一点又会挂掉,真让人摸不着头脑
                    // 其他同学有的也有这个问题,我实在没看出来这样做有什么意义,也没找到依赖库函数的ABI和汇编实现,反正试出来这样能过:)
                    // 如果你是用load/store全程在栈上跑的,那你可能还不会遇到这个问题,反正我室友是这样的(BYD gou运太好了,这就是我们免费十连稳出限定的实力吗?)
                    if(call->op1.name == "putfloat")
                        cxt.add_ins(rvOPCODE::ADDI, rv_reg(rvREG::sp), rv_reg(rvREG::sp), -4);
                    else if(call->op1.name == "putfarray")
                        cxt.add_ins(rvOPCODE::ADDI, rv_reg(rvREG::sp), rv_reg(rvREG::sp), -12);

                    // 函数调用
                    cxt.add_ins(rvOPCODE::CALL, op1.name);
                    
                    if(call->op1.name == "putfloat")
                        cxt.add_ins(rvOPCODE::ADDI, rv_reg(rvREG::sp), rv_reg(rvREG::sp), 4);
                    else if(call->op1.name == "putfarray")
                        cxt.add_ins(rvOPCODE::ADDI, rv_reg(rvREG::sp), rv_reg(rvREG::sp), 12);

                    // 溢出参数弹栈
                    if(ovf_size)
                        cxt.add_ins(rvOPCODE::ADDI, rv_reg(rvREG::sp), rv_reg(rvREG::sp), ovf_size);
                    
                    // 恢复临时寄存器
                    cxt.add_ins(rvOPCODE::LTR, {});

                    // 保存返回值(可能会覆盖临时寄存器)
                    switch(des.type)
                    {
                        case Type::Int:
                            cxt.add_ins(rvOPCODE::MV, getRd(des), rv_reg(rvREG::a, 0), {});
                            break;
                        case Type::Float:
                            cxt.add_ins(rvOPCODE::FMV, fgetRd(des), rv_reg(rvREG::fa, 0), {});
                            break;
                        case Type::null:
                            break;
                        default:
                            assert(0 && "invalid call");
                            break;
                    }

                    // 恢复参数寄存器
                    if(arg_size)
                    {
                        for(int i = farg_save - 1 ; i >= 0 ; i--)
                            cxt.add_ins(rvOPCODE::FLW, rv_reg(rvREG::fa, i), rv_reg(rvREG::sp), i * 4);
                        for(int i = arg_save - 1 ; i >= 0 ; i--)
                            cxt.add_ins(rvOPCODE::LW, rv_reg(rvREG::a, i), rv_reg(rvREG::sp), i * 4);
                        cxt.add_ins(rvOPCODE::ADDI, rv_reg(rvREG::sp), rv_reg(rvREG::sp), arg_size);
                    }

                    // 恢复栈帧寄存器
                    cxt.add_ins(rv::rvOPCODE::LW, rv_reg(rvREG::fp), rv_reg(rvREG::sp), cxt.arr_size);
                }
            }
            break;
        case Operator::_return:
            switch(op1.type)
            {
                case Type::Int:
                    switch(get_scope(op1))
                    {
                        case Scope::Global:
                            cxt.add_ins(rvOPCODE::LA, rv_reg(rvREG::a, 0), op1.name);
                            cxt.add_ins(rvOPCODE::LW, rv_reg(rvREG::a, 0), rv_reg(rvREG::a, 0), 0);
                            break;
                        case Scope::Local:
                            if(ARG_OVERFLOW(op1))
                                cxt.add_ins(rvOPCODE::LW, rv_reg(rvREG::a, 0), rv_reg(rvREG::fp), ARG_OFFSET(op1));
                            else
                                cxt.add_ins(rvOPCODE::MV, rv_reg(rvREG::a, 0), getRs(op1), {});
                            break;
                        case Scope::Temporary:
                            cxt.add_ins(rvOPCODE::MV, rv_reg(rvREG::a, 0), getRs(op1), {});
                            break;
                        default:
                            break;
                    }
                    break;
                case Type::IntLiteral:
                    cxt.add_ins(rvOPCODE::LI, rv_reg(rvREG::a, 0), {}, INT(op1.name));
                    break;
                case Type::Float:
                    switch(get_scope(op1))
                    {
                        case Scope::Global:
                            cxt.add_ins(rvOPCODE::LA, rv_reg(rvREG::t, cxt.tmp), op1.name);
                            cxt.add_ins(rvOPCODE::FLW, rv_reg(rvREG::fa, 0), rv_reg(rvREG::t, cxt.tmp++), 0);
                            break;
                        case Scope::Local:
                            if(FARG_OVERFLOW(op1))
                                cxt.add_ins(rvOPCODE::FLW, rv_reg(rvREG::fa, 0), rv_reg(rvREG::fp), FARG_OFFSET(op1));
                            else
                                cxt.add_ins(rvOPCODE::FMV, rv_reg(rvREG::fa, 0), fgetRs(op1), {});
                            break;
                        case Scope::Temporary:
                            cxt.add_ins(rvOPCODE::FMV, rv_reg(rvREG::fa, 0), fgetRs(op1), {});
                            break;
                        default:
                            break;
                    }
                    break;
                case Type::FloatLiteral:
                    cxt.add_ins(rvOPCODE::LI, rv_reg(rvREG::t, cxt.tmp), {}, f2li(std::stof(op1.name)));
                    cxt.add_ins(rvOPCODE::FMVWX, rv_reg(rvREG::fa, 0), rv_reg(rvREG::t, cxt.tmp++), {});
                    break;
                case Type::null:
                default:
                    break;
            }
            cxt.add_ins(rvOPCODE::RET, {});
            break;

        case Operator::fdef:
            assert(cxt.allocator.count(des.name) == 0);
            switch(des_scope)
            {
                case Scope::Local:// 局部变量声明
                    switch(get_scope(op1))
                    {
                        case Scope::Global:
                            cxt.add_ins(rvOPCODE::LA, rv_reg(rvREG::t, cxt.tmp), op1.name);
                            cxt.add_ins(rvOPCODE::FLW, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), 0);
                            break;
                        case Scope::Local:
                            if(FARG_OVERFLOW(op1))
                                cxt.add_ins(rvOPCODE::FLW, fgetRd(des), rv_reg(rvREG::fp), FARG_OFFSET(op1));
                            else
                                cxt.add_ins(rvOPCODE::FMV, fgetRd(des), fgetRs(op1), {});
                            break;
                        case Scope::Temporary:
                            cxt.add_ins(rvOPCODE::FMV, fgetRd(des), fgetRs(op1), {});
                            break;
                        case Scope::Literal:
                            cxt.add_ins(rvOPCODE::LI, rv_reg(rvREG::t, cxt.tmp), {}, f2li(std::stof(op1.name)));
                            cxt.add_ins(rvOPCODE::FMVWX, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), {});
                            break;
                        default:
                            break;
                    }
                    break;
                case Scope::Temporary:// 立即数加载
                    cxt.add_ins(rvOPCODE::LI, rv_reg(rvREG::t, cxt.tmp), {}, f2li(std::stof(op1.name)));
                    cxt.add_ins(rvOPCODE::FMVWX, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), {});
                    break;
                default:
                    assert(0 && "invalid operator: fdef");
                    break;
            } 
            break;

        case Operator::fmov:
            switch(des_scope)
            {
                case Scope::Global:// 全局变量赋值
                    {
                        auto base = rv_reg(rvREG::t, cxt.tmp++);
                        cxt.add_ins(rvOPCODE::LA, base, des.name);
                        cxt.add_ins(rvOPCODE::FSW,{}, base, fgetRs(op1), 0);
                    }
                    break;
                case Scope::Local:// 局部变量赋值
                    assert(cxt.allocator.count(des.name));
                    // 赋值的目标变量是溢出参数
                    if(FARG_OVERFLOW(des))
                    {
                        rv_reg frs;
                        switch(get_scope(op1))
                        {
                            case Scope::Global:
                                frs = rv_reg(rvREG::ft, cxt.ftmp++);
                                cxt.add_ins(rvOPCODE::LA, rv_reg(rvREG::t, cxt.tmp), op1.name);
                                cxt.add_ins(rvOPCODE::LW, frs, rv_reg(rvREG::t, cxt.tmp++), 0);
                                break;
                            case Scope::Local:
                            case Scope::Temporary:
                                frs = fgetRs(op1);
                                break;
                            case Scope::Literal:
                                frs = rv_reg(rvREG::ft, cxt.ftmp++);
                                cxt.add_ins(rvOPCODE::LI, rv_reg(rvREG::t, cxt.tmp), {}, f2li(std::stof(op1.name)));
                                cxt.add_ins(rvOPCODE::FMVWX, frs, rv_reg(rvREG::t, cxt.tmp++), {});
                                break;
                            default:
                                break;
                        }
                        cxt.add_ins(rvOPCODE::FSW, {}, rv_reg(rvREG::fp), frs, ARG_OFFSET(des));
                    }
                    else
                    {
                        switch(get_scope(op1))
                        {
                            case Scope::Global:
                                cxt.add_ins(rvOPCODE::LA, rv_reg(rvREG::t, cxt.tmp), op1.name);
                                cxt.add_ins(rvOPCODE::FLW, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), 0);
                                break;
                            case Scope::Local:
                            case Scope::Temporary:
                                cxt.add_ins(rvOPCODE::FMV, fgetRd(des), fgetRs(op1), {});
                                break;
                            case Scope::Literal:
                                cxt.add_ins(rvOPCODE::LI, rv_reg(rvREG::t, cxt.tmp), {}, f2li(std::stof(op1.name)));
                                cxt.add_ins(rvOPCODE::FMVWX, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), {});
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case Scope::Temporary:// 临时变量赋值
                    switch(get_scope(op1))
                    {
                        case Scope::Global:// 将全局变量,局部变量加载到临时寄存器以避免修改变量本身
                            cxt.add_ins(rvOPCODE::LA, rv_reg(rvREG::t, cxt.tmp), op1.name);
                            cxt.add_ins(rvOPCODE::FLW, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), 0);
                            break;
                        case Scope::Local:
                            if(ARG_OVERFLOW(op1))
                                cxt.add_ins(rvOPCODE::FLW, fgetRd(des), rv_reg(rvREG::fp), FARG_OFFSET(op1));
                            else
                                cxt.add_ins(rvOPCODE::FMV, fgetRd(des), fgetRs(op1), {});
                            break;
                        case Scope::Temporary:// 在与或计算时将计算结果统一到某个临时寄存器,见TODO2.33
                            cxt.add_ins(rvOPCODE::FMV, fgetRd(des), fgetRs(op1), {});
                            break;
                        default:
                            break;
                    }   
                    break;
                default:
                    assert(0 && "invalid operator: fmov");
                    break;
            }
            break;

        case Operator::fadd:
            cxt.add_ins(rvOPCODE::FADD, fgetRd(des), fgetRs(op1), fgetRs(op2));
            break;
        case Operator::fsub:
            cxt.add_ins(rvOPCODE::FSUB, fgetRd(des), fgetRs(op1), fgetRs(op2));
            break;
        case Operator::fmul:
            cxt.add_ins(rvOPCODE::FMUL, fgetRd(des), fgetRs(op1), fgetRs(op2));
            break;
        case Operator::fdiv:
            cxt.add_ins(rvOPCODE::FDIV, fgetRd(des), fgetRs(op1), fgetRs(op2));
            break;
        case Operator::flss:
            cxt.add_ins(rvOPCODE::FLT, rv_reg(rvREG::t, cxt.tmp), fgetRs(op1), fgetRs(op2));
            cxt.add_ins(rvOPCODE::I2F, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), {});
            break;
        case Operator::fleq:
            cxt.add_ins(rvOPCODE::FLE, rv_reg(rvREG::t, cxt.tmp), fgetRs(op1), fgetRs(op2));
            cxt.add_ins(rvOPCODE::I2F, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), {});
            break;
        case Operator::fgtr:
            cxt.add_ins(rvOPCODE::FGT, rv_reg(rvREG::t, cxt.tmp), fgetRs(op1), fgetRs(op2));
            cxt.add_ins(rvOPCODE::I2F, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), {});
            break;
        case Operator::fgeq:
            cxt.add_ins(rvOPCODE::FGE, rv_reg(rvREG::t, cxt.tmp), fgetRs(op1), fgetRs(op2));
            cxt.add_ins(rvOPCODE::I2F, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), {});
            break;
        case Operator::feq:
            cxt.add_ins(rvOPCODE::FEQ, rv_reg(rvREG::t, cxt.tmp), fgetRs(op1), fgetRs(op2));
            cxt.add_ins(rvOPCODE::I2F, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), {});
            break;
        case Operator::fneq:
            cxt.add_ins(rvOPCODE::FEQ, rv_reg(rvREG::t, cxt.tmp), fgetRs(op1), fgetRs(op2));
            cxt.add_ins(rvOPCODE::XORI, rv_reg(rvREG::t, cxt.tmp), rv_reg(rvREG::t, cxt.tmp), 1);
            cxt.add_ins(rvOPCODE::I2F, fgetRd(des), rv_reg(rvREG::t, cxt.tmp++), {});
            break;

        case Operator::cvt_i2f:
            cxt.add_ins(rvOPCODE::I2F, fgetRd(des), getRs(op1), {});
            break;
        case Operator::cvt_f2i:
            cxt.add_ins(rvOPCODE::F2I, getRd(des), fgetRs(op1), {});
            break;
        
        default:
            assert(0 && "invalid operator");
            break;
    }

    // TODO3.13.2
    if(ins.op == Operator::cvt_i2f || ins.op == Operator::cvt_f2i)
        cxt.add_ins(rvOPCODE::CSRW, rv_reg(rvREG::frm), old, {});
}

// 如果你的输出是139,这表示qemu在模拟运行你的程序时发生了错误(大概率是段错误),而不是程序输出了139
// 你可以重点检查你的栈指针,保证你的申请和释放是一一对应的
// 然后可以检查你的ra寄存器有没有正常地保存和恢复,保证你的返回地址是正确的(如果你评测的时候还没输出结果就直接卡死了,那大概率也是这个问题)
void backend::Generator::gen()
{
    gen_glb();// 数据段
    if(program.functions.size() > 0)// 代码段
    {
        fout << "\t.text" << ENDL;
        fout << "\t.align\t2" << ENDL;
        for(auto &func : program.functions)
        {
            if(func.name == "_global")
                continue;
            gen_func(func);
        }
    }
}

void backend::Generator::gen_glb()
{
    auto &glb_var = program.globalVal;
    auto &glb_func = program.functions.back();// 全局函数_global在实验二中是最后添加的
    
    std::set<std::string> _data;
    // TODO3.1;
    // 所有的全局变量和数组,其初始化值都可以在编译时求值到整数。因此_global函数中实际上只有三种指令:
    // 1.mov/fmov,且赋的值一定是字面量
    // 2.store,且存的值也一定是字面量
    // 3.return null
    // 所以,当_global的函数体不止含有return指令时,表明存在.data段
    if(glb_func.InstVec.size() > 1)
    {
        // .data段
        fout << "\t.data" << ENDL;
        fout << "\t.balign\t4" << ENDL;
        for(auto &ins : glb_func.InstVec)
        {
            if(ins->op == Operator::mov || ins->op == Operator::fmov)
            {
                assert(get_scope(ins->op1) == Scope::Literal);
                if((ins->op1.type == Type::IntLiteral && stoi(ins->op1.name) != 0) || (ins->op1.type == Type::FloatLiteral && stof(ins->op1.name) != 0))
                {
                    _data.insert(ins->des.name);
                    fout << "\t.global\t" << ins->des.name << ENDL;
                    fout << "\t.type\t" << ins->des.name << ", @object" << ENDL;
                    fout << ins->des.name << ":" << ENDL;
                    fout << (ins->op == Operator::mov ? "\t.word\t" : "\t.single\t") << ins->op1.name << ENDL;
                }
            }
            else if(ins->op == Operator::store)
            {
                assert(get_scope(ins->des) == Scope::Literal);
                if(_data.count(ins->op1.name) == 0)
                {
                    _data.insert(ins->op1.name);
                    fout << "\t.global\t" << ins->op1.name << ENDL;
                    fout << "\t.type\t" << ins->op1.name << ", @object" << ENDL;
                    fout << ins->op1.name << ":" << ENDL;
                }
                fout << (ins->op1.type == Type::IntPtr ? "\t.word\t" : "\t.single\t") << ins->des.name << ENDL;
            }
            else
                assert(ins->op == Operator::_return);
        }
        fout << ENDL;
    }
    // 当data段的符号数量和全局变量数不一致时,表明存在.bss段(暂不考虑.rodata等其他数据段)
    if(_data.size() < glb_var.size())
    {
        // .bss段
        fout << "\t.bss" << ENDL;
        fout << "\t.balign\t4" << ENDL;
        for(auto &var : glb_var)
        {
            if(_data.count(var.val.name) == 0)
            {
                fout << "\t.global\t" << var.val.name << ENDL;
                fout << "\t.type\t" << var.val.name << ", @object" << ENDL;
                fout << var.val.name << ":" << ENDL;
                fout << "\t.space\t" << std::max(var.maxlen, 1) * 4 << ENDL;
            }
        }
        fout << ENDL;
    }
}
