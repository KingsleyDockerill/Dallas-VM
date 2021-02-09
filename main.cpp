// C++ includes
#include <any>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <map>
#include <iostream>
#include <sstream>
#include <vector>
// C includes
#include <ncurses.h>

enum {
  R0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R_END
};

long registers[R_END];

#define MEMSIZE __UINT16_MAX__

uint32_t memory[MEMSIZE];

#undef getstr

std::string getstr() {
  std::string ret;
  char temp;
  while((temp = getch()) != '\n') {
    ret += temp;
  }
  return ret;
}

void handle_backslashes(std::string* str) {
  std::string copy = *str;
  *str = "";
  for(int i = 0; i < copy.size(); i++) {
    if(copy[i] == '\\') {
      i++;
      char escaped;
      switch(copy[i]) {
        case 'n' :
          escaped = '\n';
          break;
        case 't' :
          escaped = '\t';
          break;
        case 'b' :
          escaped = '\b';
          break;
        case 'r' :
          escaped = '\r';
          break;
        case '\\' :
          escaped = '\\';
          break;
      }
      *str += escaped;
    }
    else {
      *str += copy[i];
    }
  }
}

std::string read(FILE* fp) {
  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char* fcontent = (char*) malloc(size);
  fread(fcontent, 1, size, fp);
  fclose(fp);
  return fcontent;
}

std::map<std::string, std::string> branches;
bool branch = false;
std::string branchname, branchcontent;

void interpret_line(std::string line) {
  std::stringstream linestream(line);
  std::string keyword;
  if(!std::getline(linestream, keyword, ' ')) return;
  if(branch) {
    if(keyword == "END") {
      branch = false;
      branches.insert(std::pair<std::string, std::string>(branchname, branchcontent));
      branchname = "";
      branchcontent = "";
    }
    else {
      std::string line;
      branchcontent += keyword + ' ';
      while(std::getline(linestream, line, '\n')) {
        branchcontent += line + ' ';
      }
      branchcontent += '\n';
    }
  }
  else {
    if(keyword == "STR") {
      std::string sreg;
      if(!std::getline(linestream, sreg, ' ')) {
        std::cerr << "Not enough for STR\n";
        std::exit(1);
      }
      if(sreg[0] != 'R') {
        std::cerr << "Expected register to store str in\n";
        std::exit(1);
      }
      sreg.erase(sreg.begin());
      int reg = std::stoi(sreg);
      int i;
      for(i = 0; i < __UINT16_MAX__ && memory[i]; i++);
      if(reg >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      registers[reg] = i;
      std::string str;
      std::getline(linestream, str, '"');
      if(!std::getline(linestream, str, '"')) {
        std::cerr << "Not enough for STR\n";
        std::exit(1);
      }
      handle_backslashes(&str);
      int pos, j = 0;
      for(pos = i; str[j] != '\0'; pos++, j++) {
        memory[pos] = str[j];
      }
      memory[++j] = '\0';
    }
    else if(keyword == "REV") {
      std::string code;
      if(!std::getline(linestream, code, ' ')) {
        std::cerr << "Not enough for REV\n";
        std::exit(1);
      }
      if(code[0] != '#') {
        std::cerr << "Expected number to be passed to REV\n";
        std::exit(1);
      }
      code.erase(code.begin());
      switch(std::stoi(code)) {
        case 1 :
          int location;
          for(location = registers[R0]; memory[location] !=   '\0'; location++) {
            printw("%c", memory[location]);
          }
          break;
        case 2 :
          printw("%c", registers[R0]);
          break;
        case 3 :
          printw("%d", registers[R0]);
          break;
        case 4 :
          registers[R0] = getch();
          break;
        case 5 : {
          std::string str = getstr();
          int pos;
          for(pos = 0; memory[pos]; pos++);
          for(auto a: str) {
            memory[pos] = a;
            pos++;
          }
          memory[++pos] = '\0';
          break;
        }
        case 6 :
          clear();
          break;
      }
    }
    else if(keyword == "RET") {
      std::string code;
      if(!std::getline(linestream, code)) {
        refresh();
        getch();
        endwin();
        std::exit(0);
      }
      refresh();
      getch();
      endwin();
      std::exit(std::stoi(code));
    }
    else if(keyword == "LD") {
      std::string reg;
      if(!std::getline(linestream, reg, ' ')) {
        std::cerr << "Not enough for LD\n";
        std::exit(1);
      }
      if(reg[0] != 'R') {
        std::cerr << "Non-register passed to LD\n";
        std::exit(1);
      }
      reg.erase(reg.begin());
      if(std::stoi(reg) >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      int val;
      std::string val_;
      if(!std::getline(linestream, val_, ' ')) {
        std::cerr << "Not enough for LD\n";
        std::exit(1);
      }
      if(val_[0] == '#') {
        val_.erase(val_.begin());
        val = std::stoi(val_);
      }
      else if(val_[0] == 'R') {
        val_.erase(val_.begin());
        if(std::stoi(val_) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val = registers[std::stoi(val_)];
      }
      else {
        val = memory[std::stoi(val_)];
      }
      registers[std::stoi(reg)] = val;
    }
    else if(keyword == "ADD") {
      std::string reg;
      if(!std::getline(linestream, reg, ' ')) {
        std::cerr << "Not enough for ADD\n";
        std::exit(1);
      }
      if(reg[0] != 'R') {
        std::cerr << "Non-register passed to ADD\n";
        std::exit(1);
      }
      reg.erase(reg.begin());
      if(std::stoi(reg) >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      int val1;
      std::string val_1;
      if(!std::getline(linestream, val_1, ' ')) {
        std::cerr << "Not enough for ADD\n";
        std::exit(1);
      }
      if(val_1[0] == '#') {
        val_1.erase(val_1.begin());
        val1 = std::stoi(val_1);
      }
      else if(val_1[0] == 'R') {
        val_1.erase(val_1.begin());
        if(std::stoi(val_1) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val1 = registers[std::stoi(val_1)];
      }
      else {
        val1 = memory[std::stoi(val_1)];
      }
      int val2;
      std::string val_2;
      if(!std::getline(linestream, val_2, ' ')) {
        std::cerr << "Not enough for ADD\n";
        std::exit(1);
      }
      if(val_2[0] == '#') {
        val_2.erase(val_2.begin());
        val2 = std::stoi(val_2);
      }
      else if(val_2[0] == 'R') {
        val_2.erase(val_2.begin());
        if(std::stoi(val_2) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val2 = registers[std::stoi(val_2)];
      }
      else {
        val2 = memory[std::stoi(val_2)];
      }
      registers[std::stoi(reg)] = val1 + val2;
    }
    else if(keyword == "SUB") {
      std::string reg;
      if(!std::getline(linestream, reg, ' ')) {
        std::cerr << "Not enough for SUB\n";
        std::exit(1);
      }
      if(reg[0] != 'R') {
        std::cerr << "Non-register passed to SUB\n";
        std::exit(1);
      }
      reg.erase(reg.begin());
      if(std::stoi(reg) >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      int val1;
      std::string val_1;
      if(!std::getline(linestream, val_1, ' ')) {
        std::cerr << "Not enough for SUB\n";
        std::exit(1);
      }
      if(val_1[0] == '#') {
        val_1.erase(val_1.begin());
        val1 = std::stoi(val_1);
      }
      else if(val_1[0] == 'R') {
        val_1.erase(val_1.begin());
        if(std::stoi(val_1) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val1 = registers[std::stoi(val_1)];
      }
      else {
        val1 = memory[std::stoi(val_1)];
      }
      int val2;
      std::string val_2;
      if(!std::getline(linestream, val_2, ' ')) {
        std::cerr << "Not enough for SUB\n";
        std::exit(1);
      }
      if(val_2[0] == '#') {
        val_2.erase(val_2.begin());
        val2 = std::stoi(val_2);
      }
      else if(val_2[0] == 'R') {
        val_2.erase(val_2.begin());
        if(std::stoi(val_2) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val2 = registers[std::stoi(val_2)];
      }
      else {
        val2 = memory[std::stoi(val_2)];
      }
      registers[std::stoi(reg)] = val1 - val2;
    }
    else if(keyword == "MUL") {
      std::string reg;
      if(!std::getline(linestream, reg, ' ')) {
        std::cerr << "Not enough for MUL\n";
        std::exit(1);
      }
      if(reg[0] != 'R') {
        std::cerr << "Non-register passed to MUL\n";
        std::exit(1);
      }
      reg.erase(reg.begin());
      if(std::stoi(reg) >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      int val1;
      std::string val_1;
      if(!std::getline(linestream, val_1, ' ')) {
        std::cerr << "Not enough for MUL\n";
        std::exit(1);
      }
      if(val_1[0] == '#') {
        val_1.erase(val_1.begin());
        val1 = std::stoi(val_1);
      }
      else if(val_1[0] == 'R') {
        val_1.erase(val_1.begin());
        if(std::stoi(val_1) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val1 = registers[std::stoi(val_1)];
      }
      else {
        val1 = memory[std::stoi(val_1)];
      }
      int val2;
      std::string val_2;
      if(!std::getline(linestream, val_2, ' ')) {
        std::cerr << "Not enough for MUL\n";
        std::exit(1);
      }
      if(val_2[0] == '#') {
        val_2.erase(val_2.begin());
        val2 = std::stoi(val_2);
      }
      else if(val_2[0] == 'R') {
        val_2.erase(val_2.begin());
        if(std::stoi(val_2) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val2 = registers[std::stoi(val_2)];
      }
      else {
        val2 = memory[std::stoi(val_2)];
      }
      registers[std::stoi(reg)] = val1 * val2;
    }
    else if(keyword == "DIV") {
      std::string reg;
      if(!std::getline(linestream, reg, ' ')) {
        std::cerr << "Not enough for DIV\n";
        std::exit(1);
      }
      if(reg[0] != 'R') {
        std::cerr << "Non-register passed to DIV\n";
        std::exit(1);
      }
      reg.erase(reg.begin());
      if(std::stoi(reg) >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      int val1;
      std::string val_1;
      if(!std::getline(linestream, val_1, ' ')) {
        std::cerr << "Not enough for DIV\n";
        std::exit(1);
      }
      if(val_1[0] == '#') {
        val_1.erase(val_1.begin());
        val1 = std::stoi(val_1);
      }
      else if(val_1[0] == 'R') {
        val_1.erase(val_1.begin());
        if(std::stoi(val_1) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val1 = registers[std::stoi(val_1)];
      }
      else {
        val1 = memory[std::stoi(val_1)];
      }
      int val2;
      std::string val_2;
      if(!std::getline(linestream, val_2, ' ')) {
        std::cerr << "Not enough for DIV\n";
        std::exit(1);
      }
      if(val_2[0] == '#') {
        val_2.erase(val_2.begin());
        val2 = std::stoi(val_2);
      }
      else if(val_2[0] == 'R') {
        val_2.erase(val_2.begin());
        if(std::stoi(val_2) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val2 = registers[std::stoi(val_2)];
      }
      else {
        val2 = memory[std::stoi(val_2)];
      }
      if(val2 == 0) {
        std::cerr << "Zero division error\n";
        std::exit(1);
      }
      registers[std::stoi(reg)] = val1 / val2;
    }
    else if(keyword == "POW") {
      std::string reg;
      if(!std::getline(linestream, reg, ' ')) {
        std::cerr << "Not enough for POW\n";
        std::exit(1);
      }
      if(reg[0] != 'R') {
        std::cerr << "Non-register passed to POW\n";
        std::exit(1);
      }
      reg.erase(reg.begin());
      if(std::stoi(reg) >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      int val1;
      std::string val_1;
      if(!std::getline(linestream, val_1, ' ')) {
        std::cerr << "Not enough for POW\n";
        std::exit(1);
      }
      if(val_1[0] == '#') {
        val_1.erase(val_1.begin());
        val1 = std::stoi(val_1);
      }
      else if(val_1[0] == 'R') {
        val_1.erase(val_1.begin());
        if(std::stoi(val_1) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val1 = registers[std::stoi(val_1)];
      }
      else {
        val1 = memory[std::stoi(val_1)];
      }
      int val2;
      std::string val_2;
      if(!std::getline(linestream, val_2, ' ')) {
        std::cerr << "Not enough for POW\n";
        std::exit(1);
      }
      if(val_2[0] == '#') {
        val_2.erase(val_2.begin());
        val2 = std::stoi(val_2);
      }
      else if(val_2[0] == 'R') {
        val_2.erase(val_2.begin());
        if(std::stoi(val_2) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val2 = registers[std::stoi(val_2)];
      }
      else {
        val2 = memory[std::stoi(val_2)];
      }
      registers[std::stoi(reg)] = std::pow(val1, val2);
    }
    else if(keyword == "AND") {
      std::string reg;
      if(!std::getline(linestream, reg, ' ')) {
        std::cerr << "Not enough for AND\n";
        std::exit(1);
      }
      if(reg[0] != 'R') {
        std::cerr << "Non-register passed to AND\n";
        std::exit(1);
      }
      reg.erase(reg.begin());
      if(std::stoi(reg) >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      int val1;
      std::string val_1;
      if(!std::getline(linestream, val_1, ' ')) {
        std::cerr << "Not enough for AND\n";
        std::exit(1);
      }
      if(val_1[0] == '#') {
        val_1.erase(val_1.begin());
        val1 = std::stoi(val_1);
      }
      else if(val_1[0] == 'R') {
        val_1.erase(val_1.begin());
        if(std::stoi(val_1) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val1 = registers[std::stoi(val_1)];
      }
      else {
        val1 = memory[std::stoi(val_1)];
      }
      int val2;
      std::string val_2;
      if(!std::getline(linestream, val_2, ' ')) {
        std::cerr << "Not enough for AND\n";
        std::exit(1);
      }
      if(val_2[0] == '#') {
        val_2.erase(val_2.begin());
        val2 = std::stoi(val_2);
      }
      else if(val_2[0] == 'R') {
        val_2.erase(val_2.begin());
        if(std::stoi(val_2) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val2 = registers[std::stoi(val_2)];
      }
      else {
        val2 = memory[std::stoi(val_2)];
      }
      registers[std::stoi(reg)] = val1 & val2;
    }
    else if(keyword == "OR") {
      std::string reg;
      if(!std::getline(linestream, reg, ' ')) {
        std::cerr << "Not enough for OR\n";
        std::exit(1);
      }
      if(reg[0] != 'R') {
        std::cerr << "Non-register passed to OR\n";
        std::exit(1);
      }
      reg.erase(reg.begin());
      if(std::stoi(reg) >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      int val1;
      std::string val_1;
      if(!std::getline(linestream, val_1, ' ')) {
        std::cerr << "Not enough for OR\n";
        std::exit(1);
      }
      if(val_1[0] == '#') {
        val_1.erase(val_1.begin());
        val1 = std::stoi(val_1);
      }
      else if(val_1[0] == 'R') {
        val_1.erase(val_1.begin());
        if(std::stoi(val_1) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val1 = registers[std::stoi(val_1)];
      }
      else {
        val1 = memory[std::stoi(val_1)];
      }
      int val2;
      std::string val_2;
      if(!std::getline(linestream, val_2, ' ')) {
        std::cerr << "Not enough for OR\n";
        std::exit(1);
      }
      if(val_2[0] == '#') {
        val_2.erase(val_2.begin());
        val2 = std::stoi(val_2);
      }
      else if(val_2[0] == 'R') {
        val_2.erase(val_2.begin());
        if(std::stoi(val_2) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val2 = registers[std::stoi(val_2)];
      }
      else {
        val2 = memory[std::stoi(val_2)];
      }
      registers[std::stoi(reg)] = val1 | val2;
    }
    else if(keyword == "XOR") {
      std::string reg;
      if(!std::getline(linestream, reg, ' ')) {
        std::cerr << "Not enough for XOR\n";
        std::exit(1);
      }
      if(reg[0] != 'R') {
        std::cerr << "Non-register passed to XOR\n";
        std::exit(1);
      }
      reg.erase(reg.begin());
      if(std::stoi(reg) >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      int val1;
      std::string val_1;
      if(!std::getline(linestream, val_1, ' ')) {
        std::cerr << "Not enough for XOR\n";
        std::exit(1);
      }
      if(val_1[0] == '#') {
        val_1.erase(val_1.begin());
        val1 = std::stoi(val_1);
      }
      else if(val_1[0] == 'R') {
        val_1.erase(val_1.begin());
        if(std::stoi(val_1) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val1 = registers[std::stoi(val_1)];
      }
      else {
        val1 = memory[std::stoi(val_1)];
      }
      int val2;
      std::string val_2;
      if(!std::getline(linestream, val_2, ' ')) {
        std::cerr << "Not enough for XOR\n";
        std::exit(1);
      }
      if(val_2[0] == '#') {
        val_2.erase(val_2.begin());
        val2 = std::stoi(val_2);
      }
      else if(val_2[0] == 'R') {
        val_2.erase(val_2.begin());
        if(std::stoi(val_2) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val2 = registers[std::stoi(val_2)];
      }
      else {
        val2 = memory[std::stoi(val_2)];
      }
      registers[std::stoi(reg)] = val1 ^ val2;
    }
    else if(keyword == "NOT") {
      std::string reg;
      if(!std::getline(linestream, reg, ' ')) {
        std::cerr << "Not enough for NOT\n";
        std::exit(1);
      }
      if(reg[0] != 'R') {
        std::cerr << "Non-register passed to NOT\n";
        std::exit(1);
      }
      reg.erase(reg.begin());
      if(std::stoi(reg) >= R_END) {
        std::cerr << "Illegal register\n";
        std::exit(1);
      }
      int val;
      std::string val_;
      if(!std::getline(linestream, val_, ' ')) {
        std::cerr << "Not enough for NOT\n";
        std::exit(1);
      }
      if(val_[0] == '#') {
        val_.erase(val_.begin());
        val = std::stoi(val_);
      }
      else if(val_[0] == 'R') {
        val_.erase(val_.begin());
        if(std::stoi(val_) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val = registers[std::stoi(val_)];
      }
      else {
        val = memory[std::stoi(val_)];
      }
      registers[std::stoi(reg)] = ~val;
    }
    else if(keyword == "SET") {
      std::string location;
      if(!std::getline(linestream, location, ' ')) {
        std::cerr << "Not enough for SET\n";
        std::exit(1);
      }
      if(!std::isdigit(location[0])) {
        std::cerr << "Non-memory location for SET\n";
        std::exit(1);
      }
      int loc = std::stoi(location);
      if(loc > sizeof(memory) / sizeof(uint32_t)) {
        std::cerr << "Invalid memory location\n";
        std::exit(1);
      }
      std::string val;
      int val_;
      if(!std::getline(linestream, val)) {
        std::cerr << "Not enough for SET\n";
        std::exit(1);
      }
      if(val[0] == '#') {
        val.erase(val.begin());
        val_ = std::stoi(val);
      }
      else if(val[0] == 'R') {
        val.erase(val.begin());
        if(std::stoi(val) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val_ = registers[std::stoi(val)];
      }
      else {
        val_ = memory[std::stoi(val)];
      }
      memory[loc] = val_;
    }
    else if(keyword == "CMP") {
      std::string val;
      int val_1;
      if(!std::getline(linestream, val, ' ')) {
        std::cerr << "Not enough for CMP\n";
        std::exit(1);
      }
      if(val[0] == '#') {
        val.erase(val.begin());
        val_1 = std::stoi(val);
      }
      else if(val[0] == 'R') {
        val.erase(val.begin());
        if(std::stoi(val) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val_1 = registers[std::stoi(val)];
      }
      else {
        val_1 = memory[std::stoi(val)];
      }
      int val_2;
      if(!std::getline(linestream, val, ' ')) {
        std::cerr << "Not enough for CMP\n";
        std::exit(1);
      }
      if(val[0] == '#') {
        val.erase(val.begin());
        val_2 = std::stoi(val);
      }
      else if(val[0] == 'R') {
        val.erase(val.begin());
        if(std::stoi(val) >= R_END) {
          std::cerr << "Illegal register\n";
          std::exit(1);
        }
        val_2 = registers[std::stoi(val)];
      }
      else {
        val_2 = memory[std::stoi(val)];
      }
      std::string op;
      if(!std::getline(linestream, op, ' ')) {
        std::cerr << "Not enough for CMP\n";
        std::exit(1);
      }
      int op_ = std::stoi(op);
      std::string execute, execute_;
      while(std::getline(linestream, execute_, ' ')) {
        execute += execute_;
        execute += ' ';
      }
      switch(op_) {
        case 1 :
          if(val_1 == val_2) {
            interpret_line(execute);
          }
          break;
        case 2 :
          if(val_1 != val_2) {
            interpret_line(execute);
          }
          break;
        case 3 :
          if(val_1 < val_2) {
            interpret_line(execute);
          }
          break;
        case 4 :
          if(val_1 > val_2) {
            interpret_line(execute);
          }
          break;
        case 5 :
          if(val_1 <= val_2) {
            interpret_line(execute);
          }
          break;
        case 6 :
          if(val_1 >= val_2) {
            interpret_line(execute);
          }
          break;
      }
    }
    else if(keyword == "CALL") {
      std::string branchname_, temp, content;
      std::stringstream content_;
      if(!std::getline(linestream, branchname_)) {
        std::cerr << "Not enough for CALL\n";
        std::exit(1);
      }
      content = branches.at(branchname_);
      content_ = std::stringstream(content);
      while(std::getline(content_, temp, '\n')) {
        interpret_line(temp);
      }
    }
    else {
      branchname = keyword;
      branch = true;
    }
  }
}

int main() {
  try {
    std::string in;
    std::cout << "Enter file to run... ";
    getline(std::cin, in);
    FILE* file = fopen(in.c_str(), "r");
    if(file == NULL) {
      std::cout << "Fatal: cannot open file\n";
      std::exit(1);
    }
    initscr();
    cbreak();
    noecho();
    std::string line;
    std::stringstream filestream = std::stringstream(read(file));
    while(std::getline(filestream, line, '\n')) {
      interpret_line(line);
    }
  }
  catch(std::exception& e) {
    std::cerr << "Got runtime error, recheck your syntax. Msg: " << e.what() << "\n";
    std::exit(1);
  }
}