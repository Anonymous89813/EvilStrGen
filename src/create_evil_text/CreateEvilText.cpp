//
// Created by 黄鸿 on 2023/8/11.
//

#include "map"
#include "cstdlib"
#include "set"
#include "fstream"
#include <iostream>
#include "ctime"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <atomic>
#include <deque>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "utf.h"

#include "src/dfa.h"
#include "util/logging.h"
#include "util/mix.h"
#include "util/mutex.h"
#include "util/strutil.h"
#include "src/pod_array.h"
#include "src/prog.h"
#include "src/re2.h"
#include "src/sparse_set.h"
#include "src/stringpiece.h"
using namespace re2;

#define DeadState reinterpret_cast<State*>(1)

// Signals that the rest of the string matches no matter what it is.
#define FullMatchState reinterpret_cast<State*>(2)

#define SpecialStateMax FullMatchState

std::pair<int, bool> DFA::HamiltonDeep(std::set<State*> m, std::deque<char> Rune_q, State* T, std::vector<int> input, int n){
    int recur_num = 0;
    if (n == 0)
        return std::make_pair(recur_num, true);
    n--;
    //std::cout << Rune_q.size() << std::endl;
    for (auto i : input){
        State* ns = RunStateOnByteUnlocked(T, i);
        if (ns == NULL){
            std::cout << "state NULL" << std::endl;
            exit(1);
        }
        if (ns == DeadState){
            continue;
        }
        if (m.find(ns) == m.end()){
            m.insert(ns);
            Rune_q.push_back(i);
            recur_num ++;
            std::pair<int, bool> result = HamiltonDeep(m, Rune_q, ns, input, n);
            recur_num = recur_num +  result.first;
            if (result.second)
                return std::make_pair(recur_num, true);
            m.erase(ns);
            Rune_q.pop_back();
        }
    }
    return std::make_pair(recur_num, false);
}





// Build out all states in DFA.  Returns number of states.
int DFA::Hamilton_Deep_Random_Search(int length){
    if (!ok())
        return 0;
    // Pick out start state for unanchored search
    // at beginning of text.
    RWLocker l(&cache_mutex_);
    SearchParams params(StringPiece(), StringPiece(), &l);
    params.anchored = false;
    if (!AnalyzeSearch(&params) ||
        params.start == NULL ||
        params.start == DeadState)
        return 0;

    // Add start state to work queue.
    // Note that any State* that we handle here must point into the cache,
    // so we can simply depend on pointer-as-a-number hashing and equality.
    std::set<State*> k;
    std::deque<State*> q;
    std::deque<signed int> T;
    std::deque<State*> s_f;
    std::deque<char> q_attack_string, q_attack_string1;
    q.push_back(params.start);
    std::ofstream outfile;
    outfile.open("test.txt");

    // Compute the input bytes needed to cover all of the next pointers.
    int nnext = prog_->bytemap_range() + 1;  // + 1 for kByteEndText slot
    //std::cout << nnext << std::endl;
    std::vector<int> input(nnext);
    for (int c = 0; c < 256; c++) {
        int b = prog_->bytemap()[c];
        while (c < 256-1 && prog_->bytemap()[c+1] == b)
            c++;
        input[b] = c;
    }

    input[prog_->bytemap_range()] = kByteEndText;

    std::vector<int> output(nnext);

    q_attack_string.push_back('^');

    bool oom = false;
    while (!q.empty() && q_attack_string1.size() < length) {
        std::cout << q_attack_string1.size() << std::endl;
        State* s = q.back();
        char Char = q_attack_string.back();
        if (k.find(s) == k.end()){
            k.insert(s);
            s_f.push_back(s);
            //if (s != params.start)
            q_attack_string1.push_back(Char);
        }
        else{
            s_f.pop_back();
            k.erase(s);
            q_attack_string1.pop_back();
            q_attack_string.pop_back();
            q.pop_back();
            continue;
        }
        std::vector<int> input_random;
        int C;
        int index;
        for (int c : input)
            input_random.push_back(c);

        // random search
        while (!input_random.empty()) {
            index = rand() % input_random.size();
            C = input_random[index];
            State* ns = RunStateOnByteUnlocked(s, C);
            if (ns == NULL) {
                std::cout << "DFA Compute error" << std::endl;
                break;
            }
            if (ns == DeadState) {
                input_random.erase(input_random.begin()+index);
                continue;
            }
            if (k.find(ns) == k.end()){
                q_attack_string.push_back(C);
                q.push_back(ns);
            }
            input_random.erase(input_random.begin()+index);
        }
    }
    while (!q_attack_string1.empty()){
        outfile << q_attack_string1.front();
        q_attack_string1.pop_front();
    }
    outfile.close();
    return static_cast<int>(q.size());
}


int DFA::Hamilton_Deep_nStepForward_Search(int length, int ForwardLength) {
    if (!ok())
        return 0;
    // Pick out start state for unanchored search
    // at beginning of text.
    RWLocker l(&cache_mutex_);
    SearchParams params(StringPiece(), StringPiece(), &l);
    params.anchored = false;
    if (!AnalyzeSearch(&params) ||
        params.start == NULL ||
        params.start == DeadState)
        return 0;

    // Add start state to work queue.
    // Note that any State* that we handle here must point into the cache,
    // so we can simply depend on pointer-as-a-number hashing and equality.
    std::set<State*> k;


    std::deque<State*> q;
    std::deque<signed int> T;
    std::deque<State*> s_f;
    std::deque<char> q_attack_string, q_attack_string1;
    q.push_back(params.start);
    std::ofstream outfile;
    outfile.open("test.txt");

    // Compute the input bytes needed to cover all of the next pointers.
    int nnext = prog_->bytemap_range() + 1;  // + 1 for kByteEndText slot
    //std::cout << nnext << std::endl;
    std::vector<int> input(nnext);
    for (int c = 0; c < 256; c++) {
        int b = prog_->bytemap()[c];
        while (c < 256-1 && prog_->bytemap()[c+1] == b)
            c++;
        input[b] = c;
    }

    input[prog_->bytemap_range()] = kByteEndText;
    //Hamilton(&k,&T,params.start,input);
    //exit(100);

    std::vector<int> output(nnext);

    q_attack_string.push_back('^');

    bool oom = false;
    int i = ForwardLength;
    while (!q.empty() && q_attack_string1.size() < length) {
        std::cout << q_attack_string1.size() << std::endl;
        State* s = q.back();
        char Char = q_attack_string.back();
        if (k.find(s) == k.end()){
            k.insert(s);
            s_f.push_back(s);
            if (s != params.start)
                q_attack_string1.push_back(Char);
        }
        else{
            i--;
            s_f.pop_back();
            k.erase(s);
            q_attack_string1.pop_back();
            q_attack_string.pop_back();
            q.pop_back();
            continue;
        }
        // random search
        if (i == ForwardLength){
            std::map<int, std::pair<int, State*>> m;
            for (auto C : input) {
                State* ns = RunStateOnByteUnlocked(s, C);
                if (C == 255){
                    continue;
                }
                if (ns == NULL) {
                    std::cout << "DFA Compute error" << std::endl;
                    break;
                }
                if (ns == DeadState) {
                    continue;
                }
                if (k.find(ns) == k.end()){
                    std::pair<int, bool> result = HamiltonDeep(k, q_attack_string1, ns, input, ForwardLength);
                    while (m.find(result.first)!= m.end()){
                        result.first++;
                    }
                    m.insert(std::make_pair(result.first, std::make_pair(C, ns)));
                    //q_attack_string.push_back(C);
                    //q.push_back(ns)
                }
            }
            for (auto rit = m.rbegin(); rit != m.rend(); rit++){
                //std::cout << rit->first << std::endl;
                q_attack_string.push_back(rit->second.first);
                q.push_back(rit->second.second);
            }
            if (m.size() == 0)
                continue;
            else
                i = 1;
        }
        else{
            for (auto C : input) {
                State* ns = RunStateOnByteUnlocked(s, C);
                if (C == 255){
                    continue;
                }
                if (ns == NULL) {
                    std::cout << "DFA Compute error" << std::endl;
                    break;
                }
                if (ns == DeadState) {
                    continue;
                }
                if (k.find(ns) == k.end()){
                    q_attack_string.push_back(C);
                    q.push_back(ns);
                }
            }
            i++;
        }

    }

    while (!q_attack_string1.empty()){
        outfile << q_attack_string1.front();
        q_attack_string1.pop_front();
    }
    outfile.close();
    return static_cast<int>(q.size());
}


std::deque<unsigned int> DFA::FindOnePath(State* s, std::set<State*> Sd, std::deque<unsigned int> AttackStr, std::vector<int> input){
    if (AttackStr.size() > 8000){
        std::ofstream  Outfile;
        Outfile.open("test.txt");
        for (auto c : AttackStr)
            Outfile << char(c);
        Outfile.close();
        std::cout << "end" << std::endl;
        exit(0);
    }
    Sd.insert(s);
    std::set<std::deque<unsigned int>> AttackStrSet;
    std::cout << AttackStr.size() << std::endl;
    for (auto c : input){
        auto ns = RunStateOnByteUnlocked(s, c);
        if (c == 255 || c == 256)
            continue;
        if (ns == NULL || ns == DeadState)
            continue;
        if (Sd.find(ns) == Sd.end()){
            auto String = AttackStr;
            //auto Sd1 = Sd;
            String.push_back(c);
            auto Str = FindOnePath(ns, Sd, String, input);
            AttackStrSet.insert(Str);
        }
    }
    if (AttackStrSet.empty())
        AttackStrSet.insert(AttackStr);
    int MaxLength = 0;
    std::deque<unsigned int> LongestStr;
    for (auto str : AttackStrSet){
        if (str.size() > MaxLength){
            MaxLength = str.size();
            LongestStr = str;
        }
        else{
            continue;
        }
    }
    return LongestStr;
}


int DFA::Hamilton_Deep_Mul_Search() {
    if (!ok())
        return 0;

    // Pick out start state for unanchored search
    // at beginning of text.
    RWLocker l(&cache_mutex_);
    SearchParams params(StringPiece(), StringPiece(), &l);
    params.anchored = false;
    if (!AnalyzeSearch(&params) ||
        params.start == NULL ||
        params.start == DeadState)
        return 0;

    // Add start state to work queue.
    // Note that any State* that we handle here must point into the cache,
    // so we can simply depend on pointer-as-a-number hashing and equality.
    std::unordered_map<State*, int> m;
    std::deque<State*> q;
    std::deque<char> q_attstr;
    m.emplace(params.start, static_cast<int>(m.size()));
    q.push_back(params.start);

    // Compute the input bytes needed to cover all of the next pointers.
    int nnext = prog_->bytemap_range() + 1;  // + 1 for kByteEndText slot
    std::vector<int> input(nnext);
    for (int c = 0; c < 256; c++) {
        int b = prog_->bytemap()[c];
        while (c < 256-1 && prog_->bytemap()[c+1] == b)
            c++;
        input[b] = c;
    }
    std::ofstream outfile;
    outfile.open("test.txt");

    input[prog_->bytemap_range()] = kByteEndText;
    std::vector<int> output(nnext);
    std::set<std::deque<unsigned int>> AttackStrSet;
    std::set<State*> Sd;
    std::deque<unsigned int> AttackStr;
    // Flood to expand every state.
    auto s = params.start;
    Sd.insert(s);
    for (int c : input) {
        State* ns = RunStateOnByteUnlocked(s, c);
        if (ns == NULL || ns == DeadState)
            continue;
        if (Sd.find(ns) == Sd.end()){
            auto String = AttackStr;
            String.push_back(c);
            auto Str = FindOnePath(ns, Sd, String, input);
            AttackStrSet.insert(Str);
        }
    }

    int MaxLength = 0;
    std::deque<unsigned int> LongestStr;
    for (auto str : AttackStrSet){
        if (str.size() > MaxLength){
            MaxLength = str.size();
            LongestStr = str;
        }
        else{
            continue;
        }
    }

    for (auto c : LongestStr)
        outfile << char(c);
    outfile.close();
}


int DFA::Hamilton_Deep_MoreUnbgous_Search(int length) {
    if (!ok())
        return 0;
    // Pick out start state for unanchored search
    // at beginning of text.
    RWLocker l(&cache_mutex_);
    SearchParams params(StringPiece(), StringPiece(), &l);
    params.anchored = false;
    if (!AnalyzeSearch(&params) ||
        params.start == NULL ||
        params.start == DeadState)
        return 0;

    // Add start state to work queue.
    // Note that any State* that we handle here must point into the cache,
    // so we can simply depend on pointer-as-a-number hashing and equality.
    std::set<State*> k;
    std::deque<State*> q;
    std::deque<signed int> T;
    std::deque<State*> s_f;
    std::deque<int> q_attack_string, q_attack_string1;
    q.push_back(params.start);
    std::ofstream outfile;
    outfile.open("test.txt");
    // Compute the input bytes needed to cover all of the next pointers.
    int nnext = prog_->bytemap_range() + 1;  // + 1 for kByteEndText slot
    //std::cout << nnext << std::endl;
    std::vector<int> input(nnext);
    for (int c = 0; c < 256; c++) {
        int b = prog_->bytemap()[c];
        while (c < 256-1 && prog_->bytemap()[c+1] == b)
            c++;
        input[b] = c;
    }
    std::multimap<int, int> weight_input;
    //auto ret = Char_Weight(100, params.start);

    input[prog_->bytemap_range()] = kByteEndText;
    //Hamilton(&k,&T,params.start,input);
    //exit(100);
    bool ischange = false;
    std::vector<int> output(nnext);

    q_attack_string.push_back('^');
    int Len_Cout = 0;
    bool oom = false;
    while (!q.empty() && q_attack_string1.size() < length) {
        if ((Len_Cout + 1) <= q_attack_string1.size()) {
            std::cout << q_attack_string1.size() << std::endl;
            Len_Cout += 1;
        }

        State* s = q.back();
        int Char = q_attack_string.back();
        if (k.find(s) == k.end()){
            k.insert(s);
            s_f.push_back(s);
            if (s != params.start)
                q_attack_string1.push_back(Char);
        }
        else{
            s_f.pop_back();
            k.erase(s);
            q_attack_string1.pop_back();
            q_attack_string.pop_back();
            q.pop_back();
            continue;
        }
        std::map<int, std::pair<int, State*>> m;
        std::vector<int> input_random;
        int C;
        int index;
        for (int c : input)
            input_random.push_back(c);

        while (!input_random.empty()) {
            index = rand() % input_random.size();
            C = input_random[index];
            State* ns = RunStateOnByteUnlocked(s, C);
            if (C == 255){
                input_random.erase(input_random.begin()+index);
                continue;
            }
            if (ns == NULL) {
                std::cout << "DFA Compute error" << std::endl;
                break;
            }
            if (ns == DeadState) {
                input_random.erase(input_random.begin()+index);
                continue;
            }
            if (k.find(ns) == k.end()){
                m.insert(std::make_pair(ns->ninst_, std::make_pair(C, ns)));
            }
            input_random.erase(input_random.begin()+index);
        }
        for (auto rit = m.begin(); rit != m.end(); rit++){
            //std::cout << rit->first << std::endl;
            q_attack_string.push_back(rit->second.first);
            q.push_back(rit->second.second);
        }

    }
    std::cout << q_attack_string1.size()<< std::endl;
    if (q_attack_string1.size() != 0)
        for (auto it = q_attack_string1.begin(); it != q_attack_string1.end(); it++){
            outfile << char(*it);
        }

    outfile.close();
    RuneToStringFile(q_attack_string1);
    return static_cast<int>(q.size());
}


int DFA::OutPathNum(State* s, std::vector<int> input, std::set<State*> StateSet, int Deep_Length){
    if (Deep_Length == 0)
        return 1;
    Deep_Length--;
    int count = 0;
    for (auto c : input){
        auto ns = RunStateOnByteUnlocked(s,c);
        if (ns != NULL && ns != DeadState  && StateSet.find(ns) == StateSet.end() )
            //if (ns != NULL && ns != DeadState)
            count += OutPathNum(ns, input, StateSet, Deep_Length);
    }
    return count;
}


std::multimap<int, int> DFA::Char_Weight(int length, State* begin) {

    // Pick out start state for unanchored search
    // at beginning of text.
    RWLocker l(&cache_mutex_);
    SearchParams params(StringPiece(), StringPiece(), &l);
    params.anchored = false;
    std::ofstream outfile;
    outfile.open("test.txt");
    // Add start state to work queue.
    // Note that any State* that we handle here must point into the cache,
    // so we can simply depend on pointer-as-a-number hashing and equality.
    std::set<State*> k;
    std::deque<State*> q;
    std::deque<signed int> T;
    std::deque<State*> s_f;
    std::deque<int> q_attack_string, q_attack_string1;
    q.push_back(begin);

    // Compute the input bytes needed to cover all of the next pointers.
    int nnext = prog_->bytemap_range() + 1;  // + 1 for kByteEndText slot
    //std::cout << nnext << std::endl;
    std::vector<int> input(nnext);
    for (int c = 0; c < 256; c++) {
        int b = prog_->bytemap()[c];
        while (c < 256-1 && prog_->bytemap()[c+1] == b)
            c++;
        input[b] = c;
    }

    std::multimap<int, int> weight_input;

    for (auto c : input)
        weight_input.insert(std::make_pair(1000, c));


    input[prog_->bytemap_range()] = kByteEndText;
    //Hamilton(&k,&T,params.start,input);
    //exit(100);

    std::vector<int> output(nnext);

    q_attack_string.push_back('^');

    bool oom = false;
    while (!q.empty() && q_attack_string1.size() < length) {
        std::cout << q_attack_string1.size() << std::endl;
        State *s = q.back();
        int Char = q_attack_string.back();
        if (k.find(s) == k.end()) {
            k.insert(s);
            s_f.push_back(s);
            //std::multimap<int, int>::iterator it;
            for (auto it = weight_input.begin(); it != weight_input.end(); it++) {
                if (it->second == Char) {
                    auto new_it = std::make_pair(it->first + 2, it->second);
                    weight_input.erase(it);
                    weight_input.insert(new_it);
                    break;
                }
            }
            //auto it = --weight_input.end();
            if (s != params.start)
                q_attack_string1.push_back(Char);
        } else {
            int C = q_attack_string1.back();
            for (auto it = weight_input.begin(); it != weight_input.end(); it++) {
                if (it->second == C) {
                    auto new_it = std::make_pair(it->first - 2, it->second);
                    weight_input.erase(it);
                    weight_input.insert(new_it);
                    break;
                }
            }
            s_f.pop_back();
            k.erase(s);
            q_attack_string1.pop_back();
            q_attack_string.pop_back();
            q.pop_back();
            continue;
        }
        std::deque<std::pair<State *, int>> NegativeQ, PositiveQ;

        // random search
        std::vector<int> input_random;
        int C;
        int index;
        for (int c: input)
            input_random.push_back(c);

        // random search
        for (auto &it: weight_input) {
            auto C = it.second;
            State *ns = RunStateOnByteUnlocked(s, C);
            if (C == 255) {
                continue;
            }
            if (ns == NULL) {
                std::cout << "DFA Compute error" << std::endl;
                break;
            }
            if (ns == DeadState) {
                continue;
            }
            if (k.find(ns) == k.end()) {
                q.push_back(ns);
                q_attack_string.push_back(C);
                if (it.first > 1000)
                    PositiveQ.push_back(std::make_pair(ns, C));
                else
                    NegativeQ.push_back(std::make_pair(ns, C));
            }
        }

        while (!NegativeQ.empty()) {
            q.push_back(NegativeQ.front().first);
            q_attack_string.push_back(NegativeQ.front().second);
            NegativeQ.pop_front();
        }
        while (!PositiveQ.empty()) {
            q.push_back(PositiveQ.back().first);
            q_attack_string.push_back(PositiveQ.back().second);
            PositiveQ.pop_back();
        }
    }
    return weight_input;
}

float DFA::count_InstIDNum_InState(State *s, int upper_bound) {
    int Ret = 0;
    if (s->ninst_ >= upper_bound)
        for (int i = 0; i < upper_bound; i++){
            if (s->inst_[i] > 233)
                continue;
            Ret += s->inst_[i];
        }
    else
        for (int i = 0; i < s->ninst_; i++){
            Ret += s->inst_[i];
        }
    return Ret;
}






int DFA::Hamilton_Deep_Search(int length, int restart_times, std::string regex, int regex_id, string ReDoS_file) {
    if (!ok())
        return 0;
    // Pick out start state for unanchored search
    // at beginning of text.
    RWLocker l(&cache_mutex_);
    SearchParams params(StringPiece(), StringPiece(), &l);
    std::vector<std::string> Str_IN_Regexp;
    std::vector<std::string> Str_IN_Input;
    auto start = std::chrono::high_resolution_clock::now();
    Str_IN_Input = Str_IN_Regexp;
    params.anchored = false;
    if (!AnalyzeSearch(&params) ||
        params.start == NULL ||
        params.start == DeadState)
        return 0;
    // Add start state to work queue.
    // Note that any State* that we handle here must point into the cache,
    // so we can simply depend on pointer-as-a-number hashing and equality.
    std::set<State*> k;
    std::deque<State*> q;
    std::deque<signed int> T;
    std::deque<int> q_attack_string, q_attack_string1, New_State_Count, attack_string_max ;
    q.push_back(params.start);



    int nnext = prog_->bytemap_range() + 1;
    std::vector<int> input(nnext);
    for (int c = 0; c < 256; c++) {
        int b = prog_->bytemap()[c];
        while (c < 256-1 && prog_->bytemap()[c+1] == b)
            c++;
        input[b] = c;
    }

    //将字符表赋予不同的排列组合
    bool mark = true;
    for (auto C : input) {
//        for (auto str : Str_IN_Regexp){
//            if (str.length() == 1)
//                if (str.find(char(C)) != str.npos){
//                    mark = false;
//                    break;
//                }
//        }
        if (mark){
            std::string string;
            string += char(C);
            Str_IN_Input.emplace_back(string);
        }
//        else
//            mark = true;
    }
    q_attack_string.push_back('^');
    New_State_Count.push_back(1);
    int Len_Count = 0;
    while (!q.empty() && q_attack_string1.size() < length && !q_attack_string.empty() && !New_State_Count.empty()) {
        if (attack_string_max.size() < q_attack_string1.size()){
            attack_string_max = q_attack_string1;
        }
        if ((Len_Count + 300) <= q_attack_string1.size()) {
            start = std::chrono::high_resolution_clock::now();
            std::cout << q_attack_string1.size() << std::endl;
            Len_Count += 300;
        }
        else {
            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double,std::ratio<1,1>> duration_s(now-start);
            if (duration_s.count() > 10)
                return 0;
        }
        //std::cout << q_attack_string1.size() << std::endl;
        State* s = q.back();
        int str_index = q_attack_string.back();
        int State_nun = New_State_Count.back();
        if (k.find(s) == k.end()){
            auto it = q.end();
            k.insert(s);
            if (s != params.start)
                q_attack_string1.push_back(str_index);
        }
        else{
            q.pop_back();
            k.erase(s);
            q_attack_string1.pop_back();
            New_State_Count.pop_back();
            q_attack_string.pop_back();
            continue;
        }
        //std::multimap<float, std::pair<int, std::deque<State*>>> m_str;
        std::map<float, std::pair<int, std::deque<State*>>> m_str;
        for (int i = Str_IN_Input.size()-1; i >= 0; i--) {
            auto C = Str_IN_Input[i];
            if (C.length() > 1){
                auto s1 = s;
                int before_str_ninst = s->ninst_;
                bool If_insert = true;
                std::deque<State*> q1;
                for (auto char1 : C){
                    State* ns = RunStateOnByteUnlocked(s1, char1);
                    if (ns == NULL) {
                        std::cout << "DFA Compute error" << std::endl;
                        If_insert = false;
                        break;
                    }
                    if (ns == DeadState) {
                        If_insert = false;
                        break;
                    }
                    if (k.find(ns) == k.end()){
                        q1.push_back(ns);
                    }
                    else{
                        If_insert = false;
                        break;
                    }
                    s1 = ns;
                }
                if (If_insert && k.find(s1) == k.end()){
                    float InstIDNum = count_InstIDNum_InState(s1, restart_times) / 100000 + s1->ninst_;
                    m_str.insert(std::make_pair(InstIDNum , std::make_pair(i, q1)));
                }
                continue;
            }
            else
            {
                //unsigned long int ctoi = C[0];
                State* ns = RunStateOnByteUnlocked(s, input[i]);
                if (C[0] == 0)
                    continue;
                if (ns == nullptr) {
                    std::cout << "DFA Compute error" << std::endl;
                    break;
                }
                if (ns == DeadState) {
                    continue;
                }
                if (k.find(ns) == k.end() /*&& !ns->IsMatch()*/){
                    float InstIDNum = count_InstIDNum_InState(ns, restart_times) / 100000;
                    for (int i = 0; i < ns->ninst_; i++){
                        Prog::Inst* ip = prog_->inst(ns->inst_[i]);
                        InstIDNum++;
                        if (ns->inst_[i] == 1)
                            break;
                    }
                    std::deque<State*> q_single;
                    q_single.push_back(ns);
//                    if (m_str.find(InstIDNum) != m_str.end()){
//                        std::cout << "pos" << std::endl;
//                        if (ns->inst_[0] == m_str.find(InstIDNum)->second.second.back()->inst_[0]){
//                            std::cout << "pos1" << std::endl;
//                        }
//                    }
                    m_str.insert(std::make_pair(InstIDNum, std::make_pair(i, q_single)));
                }
            }
        }
        for (auto it = m_str.begin(); it != m_str.end(); it++){
            for (auto S : it->second.second)
                q.push_back(S);
            New_State_Count.push_back(it->second.second.size());
            q_attack_string.push_back(it->second.first);
        }
        m_str.erase(m_str.begin(), m_str.end());
    }

    if (q_attack_string.empty() || q.empty() || New_State_Count.empty()){
        std::cout << "can't generate" << std::endl;
        return 2;
    }
    std::ofstream outfile;
    outfile.open("test.txt");
    for (auto it : attack_string_max){
        outfile << Str_IN_Input[it];
    }
    outfile.close();
    //RuneToStringFile(q_attack_string1);

    std::ofstream Outfile;
    std::string filename = "test.txt";
//    Outfile_regex.open(filename, ios::app);
//    Outfile_regex << regex << "\n";
//    std::cout << q_attack_string1.size() << std::endl;
//    std::cout << regex + " has redos" << std::endl;
//    std::ofstream Outfile;
//    filename = R"(D:\hhcode\PureEvilTxTGen\PurEvilUnmatch\)" + std::to_string(regex_id) + ".txt";
    std::cout << "write file" << std::endl;
    Outfile.open(ReDoS_file);
    for (auto it : q_attack_string1){
        Outfile << Str_IN_Input[it];
    }
    Outfile.close();
//    Outfile_regex.close();
    return 1;
}

//



/*void DFA::IncreaseDeep(State* s, int n, std::unordered_map<State*, std::string>  StrSet, std::string string){
    s->deep = s->deep + n;
    if (s->child.empty()){
        if (StrSet.find(s) != StrSet.end()){
            auto str = StrSet.find(s)->second;
            StrSet.erase(s);
            StrSet.insert(std::make_pair(s, string + str));
        }
        return;
    }
    for (auto ns : s->child){
        IncreaseDeep(ns, n, StrSet, string);
    }
    return;
}

bool DFA::FindIfFather(State *s, State *ns) {
    while (s != nullptr){
        if (s == ns)
            return false;
        s = s->parent;
    }
    return true;
}*/

/* int DFA::BuildAllStates(const Prog::DFAStateCallback& cb) {
     if (!ok())
         return 0;
     //State_Char_Chain SCC;
     // Pick out start state for unanchored search
     // at beginning of text.
     RWLocker l(&cache_mutex_);
     SearchParams params(StringPiece(), StringPiece(), &l);
     params.anchored = false;
     if (!AnalyzeSearch(&params) ||
         params.start == NULL ||
         params.start == DeadState)
         return 0;

     // Add start state to work queue.
     // Note that any State* that we handle here must point into the cache,
     // so we can simply depend on pointer-as-a-number hashing and equality.
     std::unordered_map<State*, int> m;
     std::deque<State*> q;
     std::deque<char> q_attstr;
     m.emplace(params.start, static_cast<int>(m.size()));
     q.push_back(params.start);

     // Compute the input bytes needed to cover all of the next pointers.
     int nnext = prog_->bytemap_range() + 1;  // + 1 for kByteEndText slot
     //std::cout << nnext << std::endl;
     std::vector<int> input(nnext);
     for (int c = 0; c < 256; c++) {
         int b = prog_->bytemap()[c];
         while (c < 256-1 && prog_->bytemap()[c+1] == b)
             c++;
         input[b] = c;
     }
     //=std::cout << "test" << std::endl;
     std::unordered_map<State*, std::string>  StrSet;
     std::ofstream outfile;
     outfile.open("test.txt");
     input[prog_->bytemap_range()] = kByteEndText;
     // Scratch space for the output.
     std::vector<int> output(nnext);
     q.front()->deep = 1;
     // Flood to expand every state.
     bool oom = false;
     while (!q.empty()) {
         State* s = q.front();
         q.pop_front();
         if (StrSet.find(s) == StrSet.end())
             StrSet.insert(std::make_pair(s, ""));
         auto ST_str = StrSet.find(s);
         bool is_inc = false;
         //std::multimap<int, State*> Order_S;
         for (int c : input) {
             State* ns = RunStateOnByteUnlocked(s, c);
             if (ns == NULL) {
                 oom = true;
                 break;
             }
             if (ns == DeadState) {
                 output[ByteMap(c)] = -1;
                 continue;
             }

             if (m.find(ns) == m.end()) {
                 ns->deep = s->deep + 1;
                 s->child.insert(ns);
                 ns->parent = s;
                 is_inc = true;
                 StrSet.insert(std::make_pair(ns,ST_str->second + char(c)));
                 m.emplace(ns, static_cast<int>(m.size()));
                 q.push_back(ns);
                 //Order_S.insert(std::make_pair(ST_str->second.size()+1, ns));
             }
             else {
                 if (FindIfFather(s, ns) && ns != DeadState && ns != params.start && ns->deep + 6 < s->deep){
                     ns->parent->child.erase(ns);
                     s->child.insert(ns);
                     ns->parent = s;
                     IncreaseDeep(ns, s->deep - ns->deep + 1, StrSet, ST_str->second);
                 }
             }
             output[ByteMap(c)] = m[ns];
         }
         if (is_inc){
             StrSet.erase(ST_str);
         }
         if (cb)
             cb(oom ? NULL : output.data(),
                s == FullMatchState || s->IsMatch());
         if (oom)
             break;
     }
     int max = 0;
     std::string maxStr;
     for (auto it : StrSet){
         if (it.second.size() > max){
             maxStr =it.second;
             max = it.second.size();
         }
     }
     std::cout << max << std::endl;
     outfile << maxStr;
     for (auto it : StrSet){
         if (!it.second.empty())
             outfile << it.second << "M";
     }
     outfile.close();
     return static_cast<int>(m.size());
 }
*/
// 一下是最简单的全覆盖算法，使用了广度搜索
// Build out all states in DFA.  Returns number of states.
int DFA::BFS_DFA_Cover_Random(int length, std::string regex, int regex_id, std::string ReDoS_file) {
    if (!ok())
        return 0;
    // Pick out start state for unanchored search
    // at beginning of text.
    RWLocker l(&cache_mutex_);
    SearchParams params(StringPiece(), StringPiece(), &l);
    params.anchored = false;
    if (!AnalyzeSearch(&params) ||
        params.start == NULL ||
        params.start == DeadState)
        return 0;

    // Add start state to work queue.
    // Note that any State* that we handle here must point into the cache,
    // so we can simply depend on pointer-as-a-number hashing and equality.
    std::unordered_map<State*, int> m;
    std::deque<State*> q;
    std::deque<char> q_attstr;
    m.emplace(params.start, static_cast<int>(m.size()));
    q.push_back(params.start);

    // Compute the input bytes needed to cover all of the next pointers.
    int nnext = prog_->bytemap_range();  // + 1 for kByteEndText slot
    //std::cout << nnext << std::endl;
    std::map<int, vector<int>> index_rune;
    std::vector<int> input(nnext);
    for (int c = 0; c < 256 - 1;) {
        int b = prog_->bytemap()[c];
        while (c < 256-1 && prog_->bytemap()[c] == b){
            if (index_rune.find(b) == index_rune.end()){
                vector<int> V;
                V.emplace_back(c);
                index_rune.insert(std::make_pair(b, V));
            }
            else {
                index_rune.find(b)->second.emplace_back(c);
            }
            c++;
        }
        input[b] = c - 1;
    }
    std::unordered_map<State*, std::string>  StrSet;
    unsigned long mem_counting = 0;
    bool oom = false;
    while (!q.empty()) {
        State* s = q.front();
        q.pop_front();
        if (StrSet.find(s) == StrSet.end())
            StrSet.insert(std::make_pair(s, ""));
        auto ST_str = StrSet.find(s);
        bool is_inc = false;
        for (int i = 0; i < input.size(); i++) {
            State* ns = RunStateOnByteUnlocked(s, input[i]);
            if (ns == NULL) {
                oom = true;
                break;
            }
            if (ns == DeadState) {
                continue;
            }
            if (m.find(ns) == m.end() && !ns->IsMatch()) {
                mem_counting += (ST_str->second.length() + 1);
                is_inc = true;
                srand(m.size());
                int index = rand() % index_rune.find(i)->second.size();
                StrSet.insert(std::make_pair(ns,ST_str->second + char(index_rune.find(i)->second[index])));
                m.emplace(ns, static_cast<int>(m.size()));
                q.push_back(ns);
            }
        }
        if (is_inc){
            mem_counting -= (ST_str->second.length());
            StrSet.erase(ST_str);
        }
        if (oom)
            break;
        if (mem_counting > length)
            break;
    }
    if (mem_counting >= length)
    {
//        std::ofstream Outfile_regex;
//        std::string filename = R"(D:\\hhcode\\PureEvilTxTGen\\backtrack_attack_txt\\regex_list\\)" + std::to_string(regex_id) + ".txt";;
//        Outfile_regex.open(filename);
//        Outfile_regex << regex;
//        //Outfile_regex << "/home/huangh/PureEvilTxTGen/attack_BackTrack100000snort_text/" + std::to_string(regex_id) + ".txt";
//        std::cout << m.size() << std::endl;
        std::cout << regex + " has redos" << std::endl;
        std::ofstream Outfile;
//        filename = R"(D:\\hhcode\\PureEvilTxTGen\\backtrack_attack_txt\\)" + std::to_string(regex_id) + ".txt";
        std::cout << "write file" << std::endl;

//      write regexes
        Outfile.open(ReDoS_file);
//        string filename = "test.txt";
//        Outfile.open(filename);
        for (auto str1 : StrSet)
            Outfile << (str1.second) ;
        Outfile << '@';
//        Outfile.close();
//        std::ofstream Outfile;
//        std::string filename = "test.txt";
//        Outfile.open(filename);
//        for (const auto& str1 : StrSet)
//            Outfile << (str1.second);
//        Outfile.close();
        return 1;
        //Outfile_regex.close();
    } else{
        std::string Str;
        for (auto str1 : StrSet)
            Str.append(str1.second);
        std::string Str_Out;
        while (Str_Out.length() < length)
            Str_Out.append(Str);
        std::cout << regex + " has redos" << std::endl;
        std::ofstream Outfile;
        Outfile.open(ReDoS_file);
        std::cout << "write file" << std::endl;
        Outfile << (Str_Out);
    }
        return 0;
}

int DFA::BFS_DFA_Hy(int length, std::string regex, int regex_id, std::string ReDoS_file) {
    if (!ok())
        return 0;
    // Pick out start state for unanchored search
    // at beginning of text.
    RWLocker l(&cache_mutex_);
    SearchParams params(StringPiece(), StringPiece(), &l);
    params.anchored = false;
    if (!AnalyzeSearch(&params) ||
        params.start == NULL ||
        params.start == DeadState)
        return 0;

    // Add start state to work queue.
    // Note that any State* that we handle here must point into the cache,
    // so we can simply depend on pointer-as-a-number hashing and equality.
    std::unordered_map<State*, int> m;
    std::deque<State*> q;
    std::deque<char> q_attstr;
    m.emplace(params.start, static_cast<int>(m.size()));
    q.push_back(params.start);

    // Compute the input bytes needed to cover all of the next pointers.
    int nnext = prog_->bytemap_range();  // + 1 for kByteEndText slot
    //std::cout << nnext << std::endl;
    std::map<int, vector<int>> index_rune;
    std::vector<int> input(nnext);
    for (int c = 0; c < 256 - 1;) {
        int b = prog_->bytemap()[c];
        while (c < 256-1 && prog_->bytemap()[c] == b){
            if (index_rune.find(b) == index_rune.end()){
                vector<int> V;
                V.emplace_back(c);
                index_rune.insert(std::make_pair(b, V));
            }
            else {
                index_rune.find(b)->second.emplace_back(c);
            }
            c++;
        }
        input[b] = c - 1;
    }
    std::unordered_map<State*, std::string>  StrSet;
    unsigned long mem_counting = 0;
    bool oom = false;
    while (!q.empty()) {
        State* s = q.front();
        q.pop_front();
        if (StrSet.find(s) == StrSet.end())
            StrSet.insert(std::make_pair(s, ""));
        auto ST_str = StrSet.find(s);
        bool is_inc = false;
        for (int i = 0; i < input.size(); i++) {
            State* ns = RunStateOnByteUnlocked(s, input[i]);
            if (ns == NULL) {
                oom = true;
                break;
            }
            if (ns == DeadState) {
                continue;
            }
            if (m.find(ns) == m.end() && !ns->IsMatch()) {
                mem_counting += (ST_str->second.length() + 1);
                is_inc = true;
                srand(m.size());
                int index = rand() % index_rune.find(i)->second.size();
                StrSet.insert(std::make_pair(ns,ST_str->second + char(input[i])));
                m.emplace(ns, static_cast<int>(m.size()));
                q.push_back(ns);
            }
        }
        if (is_inc){
            mem_counting -= (ST_str->second.length());
            StrSet.erase(ST_str);
        }
        if (oom)
            break;
        if (mem_counting > length)
            break;
    }
    if (mem_counting >= length)
    {
//        std::ofstream Outfile_regex;
//        std::string filename = R"(D:\\hhcode\\PureEvilTxTGen\\backtrack_attack_txt\\regex_list\\)" + std::to_string(regex_id) + ".txt";;
//        Outfile_regex.open(filename);
//        Outfile_regex << regex;
//        //Outfile_regex << "/home/huangh/PureEvilTxTGen/attack_BackTrack100000snort_text/" + std::to_string(regex_id) + ".txt";
//        std::cout << m.size() << std::endl;
        std::cout << regex + " has redos" << std::endl;
        std::ofstream Outfile;
//        filename = R"(D:\\hhcode\\PureEvilTxTGen\\backtrack_attack_txt\\)" + std::to_string(regex_id) + ".txt";
        std::cout << "write file" << std::endl;

//      write regexes
        Outfile.open(ReDoS_file);
//        string filename = "test.txt";
//        Outfile.open(filename);
        for (auto str1 : StrSet)
            Outfile << (str1.second) ;
        Outfile << '@';
//        Outfile.close();
//        std::ofstream Outfile;
//        std::string filename = "test.txt";
//        Outfile.open(filename);
//        for (const auto& str1 : StrSet)
//            Outfile << (str1.second);
//        Outfile.close();
        return 1;
        //Outfile_regex.close();
    } else{
            std::string Str;
            for (auto str1 : StrSet)
            Str.append(str1.second);
            std::string Str_Out;
            while (Str_Out.length() < length)
            Str_Out.append(Str);
            std::cout << regex + " has redos" << std::endl;
            std::ofstream Outfile;
            Outfile.open(ReDoS_file);
            std::cout << "write file" << std::endl;
            Outfile << (Str_Out);
    }
        return 0;
}

int DFA::BFS_DFA_Cover(int length, std::string regex, int regex_id, std::string ReDoS_file) {
    if (!ok())
        return 0;
    // Pick out start state for unanchored search
    // at beginning of text.
    RWLocker l(&cache_mutex_);
    SearchParams params(StringPiece(), StringPiece(), &l);
    params.anchored = false;
    if (!AnalyzeSearch(&params) ||
        params.start == NULL ||
        params.start == DeadState)
        return 0;

    // Add start state to work queue.
    // Note that any State* that we handle here must point into the cache,
    // so we can simply depend on pointer-as-a-number hashing and equality.
    std::unordered_map<State*, int> m;
    std::deque<State*> q;
    std::deque<char> q_attstr;
    m.emplace(params.start, static_cast<int>(m.size()));
    q.push_back(params.start);

    // Compute the input bytes needed to cover all of the next pointers.
    int nnext = prog_->bytemap_range();  // + 1 for kByteEndText slot
    //std::cout << nnext << std::endl;
    std::map<int, vector<int>> index_rune;
    std::vector<int> input(nnext);
    for (int c = 0; c < 256 - 1;) {
        int b = prog_->bytemap()[c];
        while (c < 256-1 && prog_->bytemap()[c] == b){
            if (index_rune.find(b) == index_rune.end()){
                vector<int> V;
                V.emplace_back(c);
                index_rune.insert(std::make_pair(b, V));
            }
            else {
                index_rune.find(b)->second.emplace_back(c);
            }
            c++;
        }
        input[b] = c - 1;
    }
    std::unordered_map<State*, std::string>  StrSet;
    unsigned long mem_counting = 0;
    bool oom = false;
    while (!q.empty()) {
        State* s = q.front();
        q.pop_front();
        if (StrSet.find(s) == StrSet.end())
            StrSet.insert(std::make_pair(s, ""));
        auto ST_str = StrSet.find(s);
        bool is_inc = false;
        for (int i = 0; i < input.size(); i++) {
            State* ns = RunStateOnByteUnlocked(s, input[i]);
            if (ns == NULL) {
                oom = true;
                break;
            }
            if (ns == DeadState) {
                continue;
            }
            if (m.find(ns) == m.end() && !ns->IsMatch()) {
                mem_counting += (ST_str->second.length() + 1);
                is_inc = true;
                srand(m.size());
                int index = rand() % index_rune.find(i)->second.size();
                StrSet.insert(std::make_pair(ns,ST_str->second + char(input[i])));
                m.emplace(ns, static_cast<int>(m.size()));
                q.push_back(ns);
            }
        }
        if (is_inc){
            mem_counting -= (ST_str->second.length());
            StrSet.erase(ST_str);
        }
        if (oom)
            break;
        if (mem_counting > length)
            break;
    }
    if (mem_counting >= length)
    {
//        std::ofstream Outfile_regex;
//        std::string filename = R"(D:\\hhcode\\PureEvilTxTGen\\backtrack_attack_txt\\regex_list\\)" + std::to_string(regex_id) + ".txt";;
//        Outfile_regex.open(filename);
//        Outfile_regex << regex;
//        //Outfile_regex << "/home/huangh/PureEvilTxTGen/attack_BackTrack100000snort_text/" + std::to_string(regex_id) + ".txt";
//        std::cout << m.size() << std::endl;
        std::cout << regex + " has redos" << std::endl;
        std::ofstream Outfile;
//        filename = R"(D:\\hhcode\\PureEvilTxTGen\\backtrack_attack_txt\\)" + std::to_string(regex_id) + ".txt";
        std::cout << "write file" << std::endl;

//      write regexes
        Outfile.open(ReDoS_file);
//        string filename = "test.txt";
//        Outfile.open(filename);
        for (auto str1 : StrSet)
            Outfile << (str1.second);
        Outfile << '@';
//        Outfile.close();
//        std::ofstream Outfile;
//        std::string filename = "test.txt";
//        Outfile.open(filename);
//        for (const auto& str1 : StrSet)
//            Outfile << (str1.second);
//        Outfile.close();
        return 1;
        //Outfile_regex.close();
    } else
        return 0;
}

int DFA::Mul_online_match(int length, std::string regex, int regex_id){
    if (!ok())
        return 0;
    // Pick out start state for unanchored search
    // at beginning of text.
    RWLocker l(&cache_mutex_);
    SearchParams params(StringPiece(), StringPiece(), &l);
    params.anchored = false;
    if (!AnalyzeSearch(&params) ||
        params.start == NULL ||
        params.start == DeadState)
        return 0;

    // Add start state to work queue.
    // Note that any State* that we handle here must point into the cache,
    // so we can simply depend on pointer-as-a-number hashing and equality.
    //std::map<position_state, int> PSMap;
    std::deque<int> attack_string;

    // Compute the input bytes needed to cover all of the next pointers.
    int nnext = prog_->bytemap_range();  // + 1 for kByteEndText slot
    //std::cout << nnext << std::endl;
    std::vector<int> input(nnext);
    for (int c = 0; c < 256; c++) {
        int b = prog_->bytemap()[c];
        while (c < 256-1 && prog_->bytemap()[c+1] == b)
            c++;
        input[b] = c;
    }
    // input[prog_->bytemap_range()] = kByteEndText;
    bool oom = false;
    bool to_quit = true;
    int Char;
    auto s = params.start;
    while (to_quit && attack_string.size() <= length) {
        State* chosen_state;
        chosen_state = DeadState;
        for (int c : input) {
            State* ns = RunStateOnByteUnlocked(s, c);
            if (ns == NULL) {
                oom = true;
                break;
            }
            if (ns == DeadState) {
                continue;
            }
            if (!ns->IsMatch() && ns != s) {
                if (chosen_state == DeadState){
                    chosen_state = ns;
                    Char = c;
                }
                else{
                    if (chosen_state->ninst_ < ns->ninst_){
                        chosen_state = ns;
                        Char = c;
                    }
                    if (chosen_state->ninst_ == ns->ninst_){
                        if (chosen_state->inst_[0] == ns->inst_[0]){
                            if (count_InstIDNum_InState(chosen_state, 100000) < count_InstIDNum_InState(ns, 100000)){
                                chosen_state = ns;
                                Char = c;
                            }
                        }
                        else if (chosen_state->inst_[0] < ns->inst_[0]){
                            chosen_state = ns;
                            Char = c;
                        }
                    }
                }
            }
        }
        if (chosen_state == DeadState)
            to_quit = false;
        else{
            attack_string.push_back(Char);
            s = chosen_state;
        }
        if (oom)
            break;
    }
        /*std::ofstream Outfile_regex;
        std::string filename = "/home/huangh/PureEvilTxTGen/OutputTxT/above20BFS/Redos_list"+ std::to_string(regex_id) + ".txt";
        Outfile_regex.open(filename);
        Outfile_regex << regex << "\n";
        Outfile_regex << "/home/huangh/PureEvilTxTGen/OutputTxT/above20BFS/" + std::to_string(regex_id) + ".txt";
        std::cout << m.size() << std::endl;
        std::cout << regex + " has redos" << std::endl;
        std::ofstream Outfile;
        filename = "/home/huangh/PureEvilTxTGen/OutputTxT/above20BFS/" + std::to_string(regex_id) + ".txt";
        std::cout << "write file" << std::endl;
        Outfile.open(filename);
        for (auto str1 : StrSet)
            Outfile << (str1.second) << '@';
        Outfile << '@';
        Outfile.close();*/
//        std::ofstream Outfile;
//        std::string filename = "test.txt";
//        Outfile.open(filename);
//        for (auto c : attack_string)
//            Outfile << char(c);
//        Outfile.close();
        //Outfile_regex.close();
    return 1;
};