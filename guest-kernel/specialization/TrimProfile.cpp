#include <string> 
#include <unistd.h> 
#include <set>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

map<int, set<string> > SysCallFuncs;
set<string> IrqFuncs;

vector<string> tokenize(string line) {
	vector<string> toks;
	size_t idx = line.find(":");
	if(idx == string::npos) {
		printf(": not found in line -> %s\n", line.c_str());
		return toks;
	}
	toks.push_back(line.substr(0, idx));
	toks.push_back(line.substr(idx + 1, line.size()));
	return toks;
}

void ParseProfile(char * fname) {
	ifstream file(fname);
   	string line;
   	bool in_irq = false;
    while (getline(file, line)) {
    	// if(line.size() < 19 || line.substr(0, 19) != "logging function : ") {
	    //     // printf("%s\n", line.c_str());
	    //     continue;
    	// }
    	// line = line.substr(19, line.size());
    	vector<string> toks = tokenize(line);
    	int syscall = stoi(toks[0]);
    	string func = toks[1];

    	if(func == "irq_enter") {
    		if(in_irq) {
    			printf("irq_enter on irq_enter\n");
    		}
    		in_irq = true;
    		continue;
    	}
    	if(func == "irq_exit") {
    		if(!in_irq) {
    			printf("irq_exit without irq_enter\n");
    		}
    		in_irq = false;
    		continue;
    	}
    	if(in_irq) IrqFuncs.insert(func);
    	else SysCallFuncs[syscall].insert(func);
    }
    file.close();
}

void write_stats(char * name) {
	string bname = name;
	string fname = bname + "_stats";
	std::ofstream outfile(fname);
	for(auto const& entry : SysCallFuncs) {
		outfile << entry.first << " " << entry.second.size() << "\n";
	}
	outfile << "irq" << " " << IrqFuncs.size() + 2 << "\n";
	outfile.close();
}

void write_profile(char * name) {
	string bname = name;
	string fname = bname + "_profile";
	std::ofstream outfile(fname);
	for(auto const& entry : SysCallFuncs) {
		for(auto const& func : SysCallFuncs[entry.first]) {
			outfile << entry.first << ":" << func << "\n";
		}
	}
	outfile << 0 << ":" << "irq_enter" << "\n";
	for(auto const& func : IrqFuncs) {
		outfile << 0 << ":" << func << "\n";
	}
	outfile << 0 << ":" << "irq_exit" << "\n";
	outfile.close();
}

int main(int argc, char ** argv) {
	ParseProfile(argv[1]);
	write_stats(argv[2]);
	write_profile(argv[2]);
}