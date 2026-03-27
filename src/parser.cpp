#include "parser.h"
#include "global.h"
#include "out_utils.h"

set<string> all_regs = {
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", 
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", 
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
    "ax", "bx", "cx", "dx", "si", "di", "bp", "sp", 
    "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
    "al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl", 
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
    "ah", "bh", "ch", "dh"
};

vector<string> varbytes;
vector<string> varwords;
vector<string> vardwords;
vector<string> varqwords;

vector<long int> label_history;
long int lab_ctr = 0;
long int if_ctr = 0;

bool isfunc = false;

void type(vector<string>& command){
    for(int i = 0; i < command.size(); i++){
        if(command[i].length() >= 2 && command[i][1] == ':'){
            if(command[i][0] == 'b'){
                command[i] = "byte " + command[i].substr(2);
            } else if(command[i][0] == 'w'){
                command[i] = "word " + command[i].substr(2);
            } else if(command[i][0] == 'd'){
                command[i] = "dword " + command[i].substr(2);
            } else if(command[i][0] == 'q'){
                command[i] = "qword " + command[i].substr(2);
            }
        }
    }
}
void farg(vector<string>& arg){
    for(int i = 0; i < arg.size(); i++){
        if(arg[i].length() >= 5 && arg[i].substr(0, 5) == "$arg_"){
            int argNum = stoi(arg[i].substr(5)); 
            if(mode64) arg[i] = "[rbp+" + to_string(argNum * 8 + 16) + "]";
            else if(mode32) arg[i] = "[ebp+" + to_string(argNum * 4 + 8) + "]";
            else if(mode16) arg[i] = "[bp+" + to_string(argNum * 2 + 4) + "]";
        }
    }
}

bool is_register(std::string s) {
    return all_regs.find(s) != all_regs.end();
}

int is_var(string s) {
    for(int i = 0; i < varbytes.size(); i++) if(varbytes[i] == s) return 1;
    for(int i = 0; i < varwords.size(); i++) if(varwords[i] == s) return 2;
    for(int i = 0; i < vardwords.size(); i++) if(vardwords[i] == s) return 4;
    for(int i = 0; i < varqwords.size(); i++) if(varqwords[i] == s) return 8;
    return 0;
}

string grbs(int size, char typef) {
    if (size == 1) {
        if (typef == 'a') return "al";
        if (typef == 'b') return "bl";
        if (typef == 'c') return "cl";
        if (typef == 'd') return "dl";
    }
    if (size == 2) {
        if (typef == 'a') return "ax";
        if (typef == 'b') return "bx";
        if (typef == 'c') return "cx";
        if (typef == 'd') return "dx";
    }
    if (size == 4) {
        if (typef == 'a') return "eax";
        if (typef == 'b') return "ebx";
        if (typef == 'c') return "ecx";
        if (typef == 'd') return "edx";
    }
    if (size == 8) {
        if (typef == 'a') return "rax";
        if (typef == 'b') return "rbx";
        if (typef == 'c') return "rcx";
        if (typef == 'd') return "rdx";
    }
    return ""; 
}

void tvar(string oper, string left, string right) {
    int t1 = is_var(left);
    int t2 = is_var(right);

    string oper_l = left;
    if(t1 > 0) oper_l = "[" + left + "]";
    string oper_r = right;
    if(t2 > 0) oper_r = "[" + right + "]";

    if (t1 > 0 && t2 > 0) {
        string r = "";
        if(t1 == 1) r = "al";
        else if(t1 == 2) r = "ax";
        else if(t1 == 4) r = "eax";
        else if(t1 == 8) r = "rax";
        
        outtext("mov " + r + ", " + oper_r);
        outtext(oper + " " + oper_l + ", " + r);
    } else {
        outtext(oper + " " + oper_l + ", " + oper_r);
    }
}
void chkcom(){    
    size_t first = line.find_first_not_of(" \t\r\n");
    if (first == string::npos) return;
    line = line.substr(first);

    //outtext("\n;--------------------" + line + "--------------------");

    vector<string> command;
    vector<string> arg;
    int ccom = 0;
    int carg = 0;
    bool inbkt = false;
    bool cmf = false;

    if (line[0] == '`') {
        outtext(line.substr(1, line.length() - 2));
        return;
    }

    for (int i = 0; i < line.length(); i++) {
        if(line[i] == ';') break;
        if (ccom >= command.size()) command.push_back("");
        if (line[i] == '(') {
            inbkt = true;
            continue;
        }
        if (line[i] == ')') {
            inbkt = false;
            continue;
        }

        if (inbkt) {
            if (carg >= arg.size()) arg.push_back("");
            if (line[i] == ',') {
                if (!arg[carg].empty()) carg++;
            } else {
                arg[carg] += line[i];
            }
        } else {
            if (line[i] == ' ' || line[i] == '\t') {
                if (!command[ccom].empty()) ccom++;
            } else {
                command[ccom] += line[i];
            }
        }
    }

    for (auto& a : arg) {
        size_t start = a.find_first_not_of(" \t");
        if (start != string::npos) a = a.substr(start);
    }
    string aft = line.substr(line.find('=') + 1);
    int aftpos = line.find('=');
    if(command.size() >= 1){
        type(command);
        farg(command);
        
        if(command[0] == "#mode64"){
            mode64 = true; mode32 = false; mode16 = false;
        } else if(command[0] == "#mode32"){
            mode64 = false; mode32 = true; mode16 = false;
        } else if(command[0] == "#mode16"){
            mode64 = false; mode32 = false; mode16 = true;
        }
        else if(command[0] == "<<<") {
            if(arg[0] == "all") {
                if(mode64) { outtext("push rax\npush rcx\npush rdx\npush rbx\npush rbp\npush rsi\npush rdi");}
                else {outtext("pusha");}
                return;
            }
            if(command.size() > 1) outtext("push " + command[1]);
            type(arg);
            for(int i = 0; i < arg.size(); i++) if(!arg[i].empty()) outtext("push " + arg[i]);
        }
        else if(command[0] == ">>>") {
            if(mode64) { outtext("pop rax\npop rcx\npop rdx\npop rbx\npop rbp\npop rsi\npop rdi");}
            else {outtext("popa");}
            if(command.size() > 1) outtext("pop " + command[1]);
            type(arg);
            for(int i = 0; i < arg.size(); i++) if(!arg[i].empty()) outtext("pop " + arg[i]);
        }
        else if(command.size() >= 3 && command[1] == "=") tvar("mov", command[0], command[2]);
        else if(command.size() >= 3 && command[1] == "+=") tvar("add", command[0], command[2]);
        else if(command.size() >= 3 && command[1] == "-=") tvar("sub", command[0], command[2]);
        else if(command[0] == "syscall"){
            if(!arg.empty()) outtext("mov rax, " + arg[0]);
            outtext("syscall");
        } 
        else if(command[0] == "if" && arg.size() >= 3){
            type(arg);
            tvar("cmp", arg[0], arg[2]);
            label_history.push_back(lab_ctr);
            if(arg[1] == "==") outtext("jne .LBL_" + to_string(lab_ctr));
            else if(arg[1] == "!=") outtext("je .LBL_" + to_string(lab_ctr));
            else if(arg[1] == "<")  outtext("jge .LBL_" + to_string(lab_ctr));
            else if(arg[1] == "<=") outtext("jg " + to_string(lab_ctr));
            else if(arg[1] == ">")  outtext("jle " + to_string(lab_ctr));
            else if(arg[1] == ">=") outtext("jl " + to_string(lab_ctr));
            lab_ctr++;
        }
        else if(command[0] == "while" && arg.size() >= 3){
            type(arg);
            outtext(".LBL_START_" + to_string(lab_ctr) + ":");
            tvar("cmp", arg[0], arg[2]);
            if(arg[1] == "==")      outtext("jne .LBL_END_" + to_string(lab_ctr));
            else if(arg[1] == "!=") outtext("je .LBL_END_" + to_string(lab_ctr));
            else if(arg[1] == "<")  outtext("jge .LBL_END_" + to_string(lab_ctr));
            else if(arg[1] == "<=") outtext("jg .LBL_END_" + to_string(lab_ctr));
            else if(arg[1] == ">")  outtext("jle .LBL_END_" + to_string(lab_ctr));
            else if(arg[1] == ">=") outtext("jl .LBL_END_" + to_string(lab_ctr));
            label_history.push_back(-(lab_ctr + 1)); 
            lab_ctr++;
        }
        else if(command[0] == "end."){
            if(!label_history.empty()){
                long int last_id = label_history.back();
                label_history.pop_back();

                if (last_id < 0) {
                    long int actual_id = (-last_id) - 1;
                    outtext("jmp .LBL_START_" + to_string(actual_id));
                    outtext(".LBL_END_" + to_string(actual_id) + ":");
                } else {
                    outtext(".LBL_" + to_string(last_id) + ":");
                }
            }
            else if(isfunc){
                if(mode64) {outtext("pop rbp");}
                else if(mode32) {outtext("pop ebp");}
                else if(mode16) {outtext("pop bp");}
                outtext("ret");
                isfunc = false;
            }
        }
        else if(command[0] == "func"){
            outtext(command[1] + ":");
            if(mode64) {outtext("push rbp"); outtext("mov rbp, rsp");}
            else if(mode32) {outtext("push ebp"); outtext("mov ebp, esp");}
            else if(mode16) {outtext("push bp"); outtext("mov bp, sp");}
            isfunc = true;
        }
        else if(command[0] == "label"){
            outtext(command[1] + ":");
        }
        else if(command[0] == "goto"){
            if(arg.size() >= 2)
                outtext(arg[1] + " " + arg[0]);
            else if(arg.size() >= 1)
                outtext("jmp " + arg[0]);
        }

        else if(command[0] == "byte" || command[0] == "char") {outtext(command[1] + " db " + aft); varbytes.push_back(command[1]);}
        else if(command[0] == "short") {outtext(command[1] + " dw " + aft); varwords.push_back(command[1]);}
        else if(command[0] == "int") {outtext(command[1] + " dd " + aft); vardwords.push_back(command[1]);}
        else if(command[0] == "bigint") {outtext(command[1] + " dq " + aft); varqwords.push_back(command[1]);}
        else if(command[0] == "const") {outtext(command[1] + " equ " + aft); }


        else if(command[0][0] == '@'){
            if(arg.size() >= 2) outtext(command[0].substr(1) + " " + arg[0] + ", " + arg[1]);
            else if(arg.size() >= 1) outtext(command[0].substr(1) + " " + arg[0]);
            else outtext(command[0].substr(1));
        }
        else {
            if(command[0].back() == ':'){
                outtext(command[0]);
            } else {
                cmf = true;
            }
        }

        if(cmf){
            type(arg);
            for (int i = arg.size() - 1; i >= 0; i--) {
                if (!arg[i].empty()) {
                    outtext("push " + arg[i]);
                }
            }
            outtext("call " + command[0]);
            if(!arg.empty()){
                int wordSize = mode64 ? 8 : (mode32 ? 4 : 2);
                string reg = mode64 ? "rsp" : (mode32 ? "esp" : "sp");
                outtext("add " + reg + ", " + to_string(arg.size() * wordSize));
            }
        }
    }
}