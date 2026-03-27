#include "parser.h"
#include "global.h"

int main(int argc, char* argv[]){
    string inputFile = argv[1];
    string outputFile;

    if (argc >= 4 && string(argv[2]) == "-o") outputFile = argv[3];

    if (argc >= 5 && string(argv[4]) == "-64"){mode64 = true;}
    else if (argc >= 5 && string(argv[4]) == "-32"){mode32 = true;}
    else if (argc >= 5 && string(argv[4]) == "-16"){mode16 = true;}
    else {mode64 = true;}

    in.open(inputFile);
    out.open(outputFile);

    if (in.is_open() && out.is_open())
    {
        while (getline(in, line))
        {
            if(line.empty()) {
                continue;
            }
            chkcom();
        }
        
    }
    

    in.close();
    out.close();
}