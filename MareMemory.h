#ifndef MARE_MEMORY_H_INCLUDED
#define MARE_MEMORY_H_INCLUDED

#include <iostream>
#include <string>
#include <vector>
#include "MareSet.h"
#include "VarObj.h"

namespace mare_vm {

using namespace std;

/** vector<VarObj>의 wrapper class */
class MareMemory {
    
private:
    vector<VarObj> mem;
    int mem_size = 0;
public:
    /** 메모리 확보 (실행시, 메모리 크기 변경을 최소화) */
    void autoResize(int n) {  
        if (n >= mem_size) {
            //n = (n / 256 + 1) * 256;
            n = (n / MEMORY_RESIZE + 1) * MEMORY_RESIZE;
            mem.resize(n);
            cout << endl << endl << n << endl;
        }
    }
    /** 메모리 크기 확보 */
    void resize(unsigned int n) {
        mem.resize(n);
        mem_size = mem.size();
    }
    /** 주소값의 메모리를 초기화 */
    void reset(int adrs) {
        mem[adrs].init(NON_T);
    }
    /** 주소값에 메모리에 쓰기 */
    void set(int adrs, VarObj dt) {
        mem[adrs] = dt;
    }
    /** 메모리의 값에 더하기 */
    //void add(int adrs, double dt) {
        //mem[adrs] += dt;
        //mem[adrs].val += dt;
        //mem[adrs].add(dt);
    //}
    /** 주소값에서 메모리 읽기 */
    VarObj get(int adrs) {
        return mem[adrs];
    }
    /** 저장된 메모리의 전체 크기 */
    int size() {
        return (int)mem.size();
    }
    
    /** 디버깅용 */
    string to_string() {
        int max = mem.size();
        string str1("  ****** memory (allocated:");
        str1.append(std::to_string(max)).append(")*****");
        for(int i=0; i< max; i++){
            VarObj dd = mem[i];
            if (i < 2 || dd.getDataTypeStr(dd.getType()).compare("NON_T") != 0)
                str1.append("\n").append(std::to_string(i)).append(" : ").append(dd.toFullString(true));
        }
        return str1;
    }
};

/** stack<VarObj> Wrapper Class */
class MareStack {
    private: 
        stack<VarObj> st;

        /** 스택의 첫번째 반환값(first element) 읽기 */
        VarObj top() {
            if (st.empty()) {
                throw tecSTACK_UNDERFLOW;
            }
            return st.top(); /* 스택의 값 */
        }
    public:
        MareStack() { }
        ~MareStack() { 
            /* 종료시, 메모리 정리 */
            cout << endl << " * MareStack destructor: " << st.size(); 
            while (!st.empty()) { VarObj kk = st.top(); st.pop(); cout << endl << "  - " << kk.toFullString(true);  } 
        }

        void push(VarObj n) { st.push(n); }
        void push(DtType dt, double n) { st.push(VarObj(dt, n)); }
        void push(DtType dt, string const& n) { st.push(VarObj(dt, n)); }

        int size() { return (int)st.size(); }
        bool empty() { return st.empty(); }

        /** type 정보 읽기 (stack의 top() 명령으로 조회만) */
        DtType topType() {
            if (st.empty()) {
                throw tecSTACK_UNDERFLOW;
            }
            return st.top().getType();
        }

        /** 읽기 & 삭제 (원래 stack의 pop()과는 처리 방식이 다름) */
        VarObj pop() {
            if (st.empty()) {
                throw tecSTACK_UNDERFLOW;
            }
            VarObj d = st.top();
            st.pop();  /* 스택의 첫번째 반환값(first element) 삭제 */
            return d;                                     
        }
        
    };

}

#endif