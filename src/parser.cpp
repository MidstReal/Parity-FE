#include "parser.h"
#include "global.h"
#include "out_utils.h"

vector<string> varbytes;
vector<string> varwords;
vector<string> vardwords;
vector<string> varqwords;

vector<long int> label_history;
long int lab_ctr = 0;

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

int is_var(string s) {
    for(int i = 0; i < varbytes.size(); i++) if(varbytes[i] == s) return 1;
    for(int i = 0; i < varwords.size(); i++) if(varwords[i] == s) return 2;
    for(int i = 0; i < vardwords.size(); i++) if(vardwords[i] == s) return 4;
    for(int i = 0; i < varqwords.size(); i++) if(varqwords[i] == s) return 8;
    return 0;
}

void tvar(string oper, string left, string right) {
    int t1 = is_var(left);
    int t2 = is_var(right);

    string oper_l = left;
    if(left[0] == '&') {
        oper_l = left.substr(1);
    }
    else if(t1 > 0) {
        oper_l = "[" + left + "]";
    }

    string oper_r = right;
    if(right[0] == '&') {
        oper_r = right.substr(1);
    }
    else if(t2 > 0) {
        oper_r = "[" + right + "]";
    }

    if(t1 > 0 && t2 > 0) {
        string r = "";
        if(t1 == 1) r = "al";
        else if(t1 == 2) r = "ax";
        else if(t1 == 4) r = "eax";
        else if(t1 == 8) r = "rax";

        outtext("mov " + r + ", " + oper_r);
        outtext(oper + " " + oper_l + ", " + r);
    }
    else{
        outtext(oper + " " + oper_l + ", " + oper_r);
    }
}
string tvarwf(string input) {
    string final;
    if(input[0] == '&') {
        final = input.substr(1);
    } 
    else if(is_var(input) > 0) {
        final = "[" + input + "]";
    } else final = input;
    return final;
}
void movta(string command, string command2) {
    if(mode16) {
        outtext("push ax");
        outtext("mov ax, [" + command.substr(1) + "]");
    } else if(mode32) {
        outtext("push eax");
        outtext("mov eax, [" + command.substr(1) + "]");
    } else if(mode64) {
        outtext("push rax");
        outtext("mov rax, [" + command.substr(1) + "]");
    }

    if(is_var(command.substr(1)) == 1) outtextWE("mov byte ");
    else if(is_var(command.substr(1)) == 2) outtextWE("mov word ");
    else if(is_var(command.substr(1)) == 4) outtextWE("mov dword ");
    else if(is_var(command.substr(1)) == 8) outtextWE("mov qword ");

    if(mode16) {
        outtext("[ax], " + command2);
        outtext("pop ax");
    } else if(mode32) {
        outtext("[eax], " + command2);
        outtext("pop eax");
    } else if(mode64) {
        outtext("[rax], " + command2);
        outtext("pop rax");
    }
}


void chkcom(){    
    size_t first = line.find_first_not_of(" \t\r\n");
    if(first == string::npos) return;
    line = line.substr(first);

    //outtext("\n;--------------------" + line + "--------------------");

    vector<string> command;
    vector<string> arg;
    int ccom = 0;
    int carg = 0;
    bool inbkt = false;
    bool cmf = false;

    if(line[0] == '`') {
        outtext(line.substr(1, line.length() - 2));
        return;
    }

    for(int i = 0; i < line.length(); i++) {
        if(line[i] == ';') break;
        if(ccom >= command.size()) command.push_back("");
        if(line[i] == '(') {
            inbkt = true;
            continue;
        }
        if(line[i] == ')') {
            inbkt = false;
            continue;
        }

        if(inbkt) {
            if(carg >= arg.size()) arg.push_back("");
            if(line[i] == ',') {
                if(!arg[carg].empty()) carg++;
            } else{
                arg[carg] += line[i];
            }
        } else{
            if(line[i] == ' ' || line[i] == '\t') {
                if(!command[ccom].empty()) ccom++;
            } else{
                command[ccom] += line[i];
            }
        }
    }

    for(auto& a : arg) {
        size_t start = a.find_first_not_of(" \t");
        if(start != string::npos) a = a.substr(start);
    }
    string aft = line.substr(line.find('=') + 1);
    int aftpos = line.find('=');

    size_t start = aft.find_first_not_of(" \t");
    if(start != string::npos) aft = aft.substr(start);

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
        else if(command.size() >= 3 && command[1] == "=") {
            if(command[0][0] == '*'){
                movta(command[0], tvarwf(command[2]));
            }
            else tvar("mov", command[0], command[2]);
        }
        else if(command.size() >= 3 && command[1] == "+=") tvar("add", command[0], command[2]);
        else if(command.size() >= 3 && command[1] == "-=") tvar("sub", command[0], command[2]);
        else if(command.size() >= 3 && command[1] == "*=") tvar("imul", command[0], command[2]);
        else if(command.size() >= 3 && command[1] == "/=") tvar("idiv", command[0], command[2]);
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

                if(last_id < 0) {
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

        else if(command[0] == "<Console>") outtext("format PE Console");
        else if(command[0] == "<ELF64>") outtext("format ELF64 executable 3");

        #ifdef _WIN32
            else if(command[0] == "<iData>") outtext("section '.idata' data import readable");
            else if(command[0] == "<Data>") outtext("section '.data' data readable writeable");
            else if(command[0] == "<Code>") outtext("section '.code' code readable writeable executable");
        #elif __linux__
            else if(command[0] == "<Data>") outtext("segment readable writeable");
            else if(command[0] == "<Code>") outtext("segment readable executable");
        #endif

        else if(command[0] == "byte" || command[0] == "char") {outtext(command[1] + " db " + tvarwf(aft)); varbytes.push_back(command[1]);}
        else if(command[0] == "short") {outtext(command[1] + " dw " + tvarwf(aft)); varwords.push_back(command[1]);}
        else if(command[0] == "int") {outtext(command[1] + " dd " + tvarwf(aft)); vardwords.push_back(command[1]);}
        else if(command[0] == "bigint") {outtext(command[1] + " dq " + tvarwf(aft)); varqwords.push_back(command[1]);}
        else if(command[0] == "const") {outtext(command[1] + " equ " + tvarwf(aft)); }

        else if(command[0] == "res"){
            if(command[1] == "byte" || command[1] == "char") {outtext(command[2] + " rb " + tvarwf(aft)); varbytes.push_back(command[2]);}
            else if(command[1] == "short") {outtext(command[2] + " rw " + tvarwf(aft)); varwords.push_back(command[2]);}
            else if(command[1] == "int") {outtext(command[2] + " rd " + tvarwf(aft)); vardwords.push_back(command[2]);}
            else if(command[1] == "bigint") {outtext(command[2] + " rq " + tvarwf(aft)); varqwords.push_back(command[2]);}
        }


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
            if(command[0].size() >= 3 && command[0].substr(0, 3) == "!!!"){
                string regs[] = {"rax", "rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                for(int i = 0;i<arg.size(); i++) {
                    outtext("mov " + regs[i] + ", " + tvarwf(arg[i]));
                }
                outtext("syscall");
            }else if(command[0].size() >= 2 && command[0].substr(0, 2) == "!!"){
                string regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                for(int i = 0;i<arg.size(); i++) {
                    outtext("push " + regs[i]);
                }
                for(int i = 0;i<arg.size(); i++) {
                    outtext("mov " + regs[i] + ", " + tvarwf(arg[i]));
                }
                outtext("call " + command[0].substr(2));

                for(int i = 6;i>=arg.size(); i--) {
                    outtext("pop " + regs[i]);
                }
            }else if(command[0].size() >= 1 && command[0].substr(0, 1) == "!"){
                string regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                for(int i = 0;i<arg.size(); i++) {
                    outtext("mov " + regs[i] + ", " + tvarwf(arg[i]));
                }
                outtext("call " + command[0].substr(1));
            } else{
                for(int i = arg.size() - 1; i >= 0; i--) {
                    if(!arg[i].empty()) {
                        outtext("push " + tvarwf(arg[i]));
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
}
