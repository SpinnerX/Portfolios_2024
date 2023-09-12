#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <unistd.h>
using namespace std;

#define DEBUG 0

void die(int line_number = 0) {
	cout << "Syntax Error on line " << line_number << endl;
	exit(1);
}

bool isNum(string s) {
    for (auto c : s)
        if (!isdigit(c)) return false;
    return true;
}

string var_to_assembly_register(string variable) {
    if (variable.size() > 1) return "ERROR";
    if (variable[0] == 'A') return "R4";
    if (variable[0] == 'B') return "R5";
    if (variable[0] == 'C') return "R6";
    if (variable[0] == 'I') return "R7";
    if (variable[0] == 'J') return "R8";
    if (variable[0] == 'X') return "R9";
    if (variable[0] == 'Y') return "R10";
    if (variable[0] == 'Z') return "R11";
    return "ERROR";
}

string branch_operators(string op){
	if(op == "<") return "BLT";
	else if(op == ">") return "BGT";
	else if(op == "<=") return "BLE";
	else if(op == ">=") return "BGE";
	else if(op == "==") return "BEQ";
	else if(op == "!=") return "BNE";
	
	return "ERROR";
}

int main(int argc, char **argv) {
	//If we pass any parameters, we'll just generate an assembly file
	//Otherwise we will generate an assembly file, assemble it, and run it
	bool assemble_only = false;
	if (argc > 1) assemble_only = true;
	ofstream outs("main.s"); //This is the assembly file we're creating



	if (!outs) {
		cout << "Error opening file.\n";
		return -1;
	}

	outs << ".global main\nmain:\n"; //Get the file ready to rock
	outs << "\tPUSH {LR}\n\tPUSH {R4-R12}\n\n";

    for (int i = 0; i < 13; i++)
        outs << "\tMOV R" << i << ", #0\n";

   	string data_strings = ".data\n";
	int line_counter = 0;
    int prev_label = 0;

	while (cin) {
		string s;
		getline(cin, s);
		line_counter++;
		if (!cin) break;

		// uppercasifying and checking when to terminate the compiler
		transform(s.begin(), s.end(), s.begin(), ::toupper);
		auto it = s.find("QUIT"); //TERMINATE COMPILER
		if (it != string::npos) break;


		// turns "s" which is our input to parsing out the code syntax
		stringstream ss(s); //Turn s into a stringstream
		
		int line_number; // this is how we set our line number into the code!
		ss >> line_number;

		if (!ss) die(line_counter);
        if (line_number <= prev_label) die(line_counter);
		
		outs << "line_" << line_number << ":\n"; //Write each line number to the file ("line_20:")
		
		string command;
		ss >> command;
		

		// Checking if the command is going to represent a comment
		if (command == "REM") {
            getline(ss, command);
            outs << "\t// " << command << "\n";
		}
        else if (command == "LET") {
            // Setting variables up
            string lhs;
            ss >> lhs;

            // If variable_name doesn't contain the right character, die.
            if (var_to_assembly_register(lhs) == "ERROR") die(line_counter);

            string assembly_lhs_name = var_to_assembly_register(lhs);
            string rhs_variable;
            ss >> command;

			if (!ss) die(line_counter);

            ss >> rhs_variable;
            
			if (isNum(rhs_variable)) outs << "\tMOV " << assembly_lhs_name << ", #" << rhs_variable << endl;
            else {
                outs << "\tMOV " << assembly_lhs_name << ", " << var_to_assembly_register(rhs_variable) << endl;
                char op;
                ss >> op;
                
				ss >> rhs_variable;
                
				if (op == '+') {
                    outs << "\tADD " << assembly_lhs_name << ", " << var_to_assembly_register(rhs_variable) << endl;
                }
                else if (op == '-') {
                    outs << "\tSUB " << assembly_lhs_name << ", " << assembly_lhs_name << ", " << var_to_assembly_register(rhs_variable) << endl;
                }
                else if (op == '*') {
                    outs << "\tMUL " << assembly_lhs_name << ", " << var_to_assembly_register(rhs_variable) << endl;
                }
                else die(line_number);
            }
        }

        else if (command == "PRINT") {
            string str;
            ss >> str;
            
			if (str.size() == 1){
                string reg_name = var_to_assembly_register(str);
                if (reg_name == "ERROR") die(line_number);
                outs << "\tMOV R0, " << reg_name << '\n';
                outs << "\tbl print_number" << '\n';
            }
            else if (str[0] == '"'){
                // Fetch the rest of the string
                string temp;
                getline(ss, temp);
                str.append(temp);

                if (str[str.size()-1] != '"') die(line_counter);

                outs << "\tLDR R0, =line_" << line_number << "_string" << '\n';
                outs << "\tbl print_string" << '\n';
                
				data_strings.append("line_" + to_string(line_number) + "_string: .asciz " + str + '\n');
            }
            else die(line_counter);
        }

        else if (command == "IF") {
            // Take input until "THEN" comes up.
            string lhs, rhs, op, then, go, line_no, els;
            
			// ss >> lhs >> op >> rhs;
			ss >> lhs;
			if (!ss) die(line_number);
            
			ss >> op;
			if (!ss) die(line_number);
            
			ss >> rhs;
			if (!ss) die(line_number);
            outs << "\tCMP " << var_to_assembly_register(lhs) << ", " << var_to_assembly_register(rhs) << endl;

            ss >> then;
			if (!ss or then != "THEN") die(line_number);
            
			ss >> go;
			if (!ss or go != "GOTO") die(line_number);
            
			ss >> line_no;
			if (!ss) die(line_number);
			
			// Checking for operators: <, >, <=, >=, ==, !=
			// ERROR if its either or.
			string branch = branch_operators(op);
			if(branch == "ERROR") die(line_number);
            
			outs << "\t" << branch << " line_" << line_no << endl;
            
			ss >> els;
			if (!ss or els != "ELSE") continue;
            
			ss >> go >> line_no;
			// ss >> line_no;
            outs << "\tBAL line_" << line_no << endl;
        }
        else if (command == "INPUT") {
            string variable;
            ss >> variable;

			if (!ss) die(line_counter);
            variable = var_to_assembly_register(variable);
            
			if (variable == "ERROR") die(line_number);
            
			outs << "\tbl read_number" << endl;
            outs << "\tMOV " << variable << ", R0" << endl;
        }
		else if (command == "GOTO") {
			int target;
			ss >> target; if (!ss) die(line_counter);
			outs << "\tBAL line_" << target << endl;
		}
		else if (command == "EXIT" or command == "END") outs << "\tBAL quit\n";
        else die(line_counter);
	}

	//Clean up the file at the bottom
	outs << "\nquit:\n\tMOV R0, #42\n\tPOP {R4-R12}\n\tPOP {PC}\n"; //Finish the code and return
    outs << data_strings << endl;
	outs.close(); //Close the file

	if (assemble_only) return 0; //When you're debugging you should run bb8 with a parameter

	//print.o is made from the Makefile, so make sure you make your code
	if (system("g++ main.s print.o")) { //Compile your assembler code and check for errors
		cout << "Assembling failed, which means your compiler screwed up.\n";
		return 1;
	}

	//We've got an a.out now, so let's run it!
	cout << "Compilation successful. Executing program now." << endl;
	execve("a.out", nullptr, nullptr);
}
