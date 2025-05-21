#include<backend/context.h>

#include<cassert>
#include<set>

using rv::rvOPCODE;
using rv::rvREG;
using rv::rv_reg;
using rv::rv_inst;

#define VALID(r) (r.index != -1 && (r.type == rvREG::t || r.type == rvREG::ft))

backend::context::context(): 
    arr_size(0), local(0), flocal(0), tmp(0), ftmp(0), argc(0), fargc(0),
    allocator(std::map<std::string, rv_reg>()), pseudocode(std::vector<rv_inst>()),
    goto_list(std::vector<int>()), prefixsum(std::vector<int>()),
    stack(std::map<std::string, int>()) {}

void backend::context::clear()
{
    arr_size = 0;
    local = flocal = 0;
    tmp = ftmp = 0;
    argc = fargc = 0;
    allocator.clear();
    pseudocode.clear();
    goto_list.clear();
    prefixsum.clear();
    stack.clear();
}

void backend::context::add_ins(rvOPCODE op, rv_reg rd, rv_reg rs1, rv_reg rs2, int imm)
{
    pseudocode.push_back(rv_inst(rd, rs1, rs2, op, imm, {}));
}
void backend::context::add_ins(rvOPCODE op, rv_reg rd, rv_reg rs1, rv_reg rs2)
{
    pseudocode.push_back(rv_inst(rd, rs1, rs2, op, {}, {}));
}

void backend::context::add_ins(rvOPCODE op, rv_reg rd, rv_reg rs1, int imm)
{
    pseudocode.push_back(rv_inst(rd, rs1, {}, op, imm, {}));
}

void backend::context::add_ins(rvOPCODE op, rv_reg rd, std::string label)
{
    pseudocode.push_back(rv_inst(rd, {}, {}, op, {}, label));
}

void backend::context::add_ins(rvOPCODE op, std::string label)
{
    pseudocode.push_back(rv_inst({}, {}, {}, op, {}, label));
}

void backend::context::reallocate()
{
    std::map<rv_reg, int> last;
    int len = pseudocode.size();
    for(int i = 0; i < len ; i++)
    {
        auto &ins = pseudocode[i];
        if(VALID(ins.rs1))
            last[ins.rs1] = i;
        if(VALID(ins.rs2))
            last[ins.rs2] = i;
        if(VALID(ins.rd))
            last[ins.rd] = i;
    }

    std::map<rv_reg, int> realloc;

    std::vector<int> free_reg = {6, 5, 4, 3, 2, 1, 0};
    std::set<int> active_reg;
    std::set<int> saved_temp_reg;

    std::vector<int> free_freg = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    std::set<int> active_freg;
    std::set<int> saved_temp_freg;

    for(int i = 0; i < len ; i++)
    {
        int active_size = (active_reg.size() + active_freg.size()) * 4;
        int saved_temp_size = (saved_temp_reg.size() + saved_temp_freg.size()) * 4;
        if(pseudocode[i].op == rvOPCODE::STR && active_size)
        {
            pseudocode[i].expand.push_back(rv_inst(rv_reg(rvREG::sp), rv_reg(rvREG::sp), {}, rvOPCODE::ADDI, -active_size, {}));
            int top = 0;
            for(auto iter = active_reg.begin() ; iter != active_reg.end() ; iter++)
            {
                pseudocode[i].expand.push_back(rv_inst({}, rv_reg(rvREG::sp), rv_reg(rvREG::t, *iter), rvOPCODE::SW, top, {}));
                top += 4;
            }
            for(auto iter = active_freg.begin() ; iter != active_freg.end() ; iter++)
            {
                pseudocode[i].expand.push_back(rv_inst({}, rv_reg(rvREG::sp), rv_reg(rvREG::ft, *iter), rvOPCODE::FSW, top, {}));
                top += 4;
            }
            saved_temp_reg = active_reg;
            saved_temp_freg = active_freg;
            continue;
        }
        else if(pseudocode[i].op == rvOPCODE::LTR && saved_temp_size)
        {
            int top = saved_temp_size;
            for(auto iter = saved_temp_freg.rbegin() ; iter != saved_temp_freg.rend() ; iter++)
            {
                top -= 4;
                pseudocode[i].expand.push_back(rv_inst(rv_reg(rvREG::ft, *iter), rv_reg(rvREG::sp), {}, rvOPCODE::FLW, top, {}));
            }
            for(auto iter = saved_temp_reg.rbegin() ; iter != saved_temp_reg.rend() ; iter++)
            {
                top -= 4;
                pseudocode[i].expand.push_back(rv_inst(rv_reg(rvREG::t, *iter), rv_reg(rvREG::sp), {}, rvOPCODE::LW, top, {}));
            }
            pseudocode[i].expand.push_back(rv_inst(rv_reg(rvREG::sp), rv_reg(rvREG::sp), {}, rvOPCODE::ADDI, saved_temp_size, {}));
            saved_temp_reg.clear();
            saved_temp_freg.clear();
            continue;
        }

        auto &rs1 = pseudocode[i].rs1;
        auto &rs2 = pseudocode[i].rs2;
        auto &rd = pseudocode[i].rd;

        if(VALID(rs1))
        {
            assert(realloc.count(rs1));
            rs1.index = realloc[rs1];
        }
        if(VALID(rs2))
        {
            assert(realloc.count(rs2));
            rs2.index = realloc[rs2];
        }
        
        for(auto &loc : realloc)
        if(last.count(loc.first) && last[loc.first] <= i)
        {
            if(loc.first.type == rvREG::t)
            {
                active_reg.erase(loc.second);
                free_reg.push_back(loc.second);
            }
            else
            {
                active_freg.erase(loc.second);
                free_freg.push_back(loc.second);
            }
            last.erase(loc.first);
        }

        if(VALID(rd))
        {
            if(realloc.count(rd))
                rd.index = realloc[rd];
            else
            {
                if(rd.type == rvREG::t)
                {
                    assert(free_reg.size());
                    rd.index = realloc[rd] = free_reg.back();
                    active_reg.insert(free_reg.back());
                    free_reg.pop_back();
                }
                else
                {
                    assert(free_freg.size());
                    rd.index = realloc[rd] = free_freg.back();
                    active_freg.insert(free_freg.back());
                    free_freg.pop_back();
                }
            }
        }
    }
}