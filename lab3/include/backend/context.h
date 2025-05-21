#ifndef CONTEXT_H
#define CONTEXT_H

#include "backend/rv_def.h"
#include "backend/rv_inst_impl.h"

#include<map>
#include<string>
#include<vector>
#include<fstream>

namespace backend {
// TODO3.4
enum class Scope {
    Global,
    Local,
    Temporary,
    Literal
};

struct context {
    int arr_size;
    int local;
    int flocal;
    int tmp;
    int ftmp;
    int argc;
    int fargc;
    std::map<std::string, rv::rv_reg> allocator;
    std::vector<rv::rv_inst> pseudocode;
    std::vector<int> goto_list;
    std::vector<int> prefixsum;
    std::map<std::string, int> stack;

    context();
    void clear();
    void add_ins(rv::rvOPCODE, rv::rv_reg, rv::rv_reg, rv::rv_reg, int);
    void add_ins(rv::rvOPCODE, rv::rv_reg, rv::rv_reg, rv::rv_reg);
    void add_ins(rv::rvOPCODE, rv::rv_reg, rv::rv_reg, int);
    void add_ins(rv::rvOPCODE, rv::rv_reg, std::string);
    void add_ins(rv::rvOPCODE, std::string);
    // 线性寄存器分配。反正能过测试点,姑且认为没写错:)
    void reallocate();
};


}

#endif