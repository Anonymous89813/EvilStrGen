#include <iostream>
#include "src/re2.h"
#include "src/prog.h"
#include "src/regexp.h"
#include "fstream"
using namespace re2;

//\w[s123456789qwertyuioasdfghjkzxcvbnASDFGHJQWERTYUZXCVBNMdacabcdcbmrtyw1248]
int main(int argc, char *argv[]) {
    if (argc != 6){
        std::cout << "Usage: EvilStrGen [Regex] [OutputFile] [EngineType] [Attack String Length]" << std::endl;
        std::cout << "[Regex] is a file which contain a regex" << std::endl;
        std::cout << "[OutputFile] is a file where the candidate attack string will be write to" << std::endl;
        std::cout << "[EngineType] is the id of regex engine "
                     "\n 1 ---- RE2"
                     "\n 2 ---- Rust"
                     "\n 3 ---- Go"
                     "\n 4 ---- SRM"
                     "\n 5 ---- NonBacktracking"
                     "\n 6 ---- awk"
                     "\n 7 ---- grep"
                     "\n 8 ---- Hyperscan"
                     "\n 9 ---- Java"
                     "\n 10 ---- JavaScript"
                     "\n 11 ---- PCRE2"
                     "\n 12 ---- Perl"
                     "\n 13 ---- PHP"
                     "\n 14 ---- Python"
                     "\n 15 ---- Boost"
                     "\n 16 ---- Net"
                     "\n 17 ---- Backtracking"
                     "\n 18 ---- NonBacktracking" << std::endl;
        std::cout << "[Attack String Length] is MaxLength of candidate attack string" << std::endl;
        return 0;
    }
    if (std::stoi(argv[5]) == 1){
        int ID = 1;
        int length = std::stoi(argv[4]);
        std::ifstream in_regex(argv[1]);
        std::string regex;
        std::string InputDirectoryName(argv[2]);
        while (std::getline(in_regex, regex))
        {
            std::string InputFileName = InputDirectoryName + "/" + std::to_string(ID) + ".txt";
            regex = regex + '$';
            re2::RE2 Regex = re2::RE2(regex);
            re2::Prog* P = Regex.RetProg();
            if (P == nullptr){
                ID++;
                continue;
            }
            std::cout << ID << std::endl;
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::RE2, length, regex, 1, 3, InputFileName);
            ID++;
        }
        return 1;
    }
    std::string regex;
    std::ifstream in_regex(argv[1]);
    std::getline(in_regex, regex);
    int length = std::stoi(argv[4]);
    int EngineID = std::stoi(argv[3]);
    regex = regex;
    re2::RE2 Regex = re2::RE2(regex);
    re2::Prog* P = Regex.RetProg();
    switch (EngineID) {
        case 1:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::RE2, length, regex, 1, 3, argv[2]);
            break;
        case 2:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::Rust, length, regex, 1, 3, argv[2]);
            break;
        case 3:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::Go, length, regex, 1, 3, argv[2]);
            break;
        case 4:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::SRM, length, regex, 1, 3, argv[2]);
            break;
        case 5:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::NonBacktracking, length, regex, 1, 3, argv[2]);
            break;
        case 6:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::awk, length, regex, 1, 3, argv[2]);
            break;
        case 7:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::grep, length, regex, 1, 3, argv[2]);
            break;
        case 8:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::Hyperscan, length, regex, 1, 3, argv[2]);
            break;
        case 9:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::Java, length, regex, 1, 3, argv[2]);
            break;
        case 10:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::JavaScript, length, regex, 1, 3, argv[2]);
            break;
        case 11:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::PCRE2, length, regex, 1, 3, argv[2]);
            break;
        case 12:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::Perl, length, regex, 1, 3, argv[2]);
            break;
        case 13:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::PHP, length, regex, 1, 3, argv[2]);
            break;
        case 14:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::Python, length, regex, 1, 3, argv[2]);
            break;
        case 15:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::Boost, length, regex, 1, 3, argv[2]);
            break;
        case 16:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::Net, length, regex, 1, 3, argv[2]);
            break;
        case 17:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::Backtracking, length, regex, 1, 3, argv[2]);
            break;
        case 18:
            P->EvilStrGen(Prog::kFullMatch, Prog::ALLSTRAT_ON, re2::Prog::NonBacktracking, length, regex, 1, 3, argv[2]);
            break;
    }

}
