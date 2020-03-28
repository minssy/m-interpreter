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
    vector<VarObj> memLocal;
    //vector<VarObj>::iterator it;
    int mem_size = 0;
    int mem_local_size = 0;
public:
    /** 로컬 함수 메모리 확보 (실행중, 메모리 크기 변경을 최소화) */
    void autoResize(int n) {  
        // if (n >= mem_size) {
        //     n = (n / MEMORY_RESIZE + 1) * MEMORY_RESIZE;
        //     mem.resize(n);
        // }
        if (n > MEMORY_GLOBAL_MAX) {
            n -= MEMORY_GLOBAL_MAX;
            if (n >= mem_local_size) {
                n = (n / MEMORY_RESIZE + 1) * MEMORY_RESIZE;
                memLocal.resize(n);
            }
        }
    }
    /** 메모리 크기 확보 */
    void resize(unsigned int n) {
        if (n < MEMORY_GLOBAL_MAX) {
            mem.resize(n);
            mem_size = mem.size();
        }
        else {
            n -= MEMORY_GLOBAL_MAX;
            memLocal.resize(n);
            mem_local_size = memLocal.size();
        }
    }
    /** 주소값의 메모리를 초기화 */
    void reset(int adrs) {
        //mem[adrs].init(NON_T);
        if (adrs < MEMORY_GLOBAL_MAX)
            mem[adrs].init(NON_T);
        else {
            adrs -= MEMORY_GLOBAL_MAX;
            memLocal[adrs].init(NON_T);
        }
    }
    /** 주소값에 메모리에 쓰기 */
    void set(int adrs, VarObj dt) {
        //mem[adrs] = dt;
        if (adrs < MEMORY_GLOBAL_MAX)
            mem[adrs] = dt;
        else {
            adrs -= MEMORY_GLOBAL_MAX;
            memLocal[adrs] = dt;
        }
    }
    /** 주소값에서 메모리 읽기 */
    VarObj get(int adrs) {
        if (adrs < MEMORY_GLOBAL_MAX)
            return mem[adrs];
        else {
            adrs -= MEMORY_GLOBAL_MAX;
            return memLocal[adrs];
        }
    }
    /** 저장된 글로벌 메모리의 전체 크기 */
    int size() {
        return (int)mem.size();
    }
    /** 배열의 크기가 변경(bigger)될 때 (adrs에 addSize만큼 dt를 추가) */
    void updateExpand(int adrs, int addSize, VarObj dt) {
        if ((adrs+addSize) >= MEMORY_GLOBAL_MAX) throw tecBAD_ALLOCATE_MEMORY;
        vector<VarObj> mem_temp;

        int iEnd = mem_size + addSize;
        int step = adrs + addSize;
        mem_temp.reserve(iEnd + 20);
        for (int k=0; k<iEnd; k++) {
            if (k < adrs) {
                mem_temp.push_back(mem[k]);
            }
            else if (k < step) {
                mem_temp.push_back(dt);
            }
            else {
                mem_temp.push_back(mem[k-addSize]);
            }
        }
        mem.clear();
        mem.assign(mem_temp.begin(), mem_temp.end());

        //mem.insert(it + adrs, addSize, dt);// 복사중 에러
        mem_size += addSize;
    }
    void updateExpand(int adrs, int addSize, VarObj* dt, int lth) {
        if ((adrs+addSize) >= MEMORY_GLOBAL_MAX) throw tecBAD_ALLOCATE_MEMORY;
        vector<VarObj> mem_temp;
        
        int iEnd = mem_size + addSize;
        int step = adrs + addSize;
        mem_temp.reserve(iEnd + 20);
        for (int k=0; k<iEnd; ) {
            if (k < adrs) {
                mem_temp.push_back(mem[k++]);
            }
            else if (k < step) {
                for (int j=0; j<lth; j++) {
                    mem_temp.push_back(dt[j]);
                    k++;
                }
            }
            else {
                mem_temp.push_back(mem[k-addSize]);
                k++;
            }
        }
        mem.clear();
        mem.assign(mem_temp.begin(), mem_temp.end());

        //mem.insert(it + adrs, addSize, dt);// 복사중 에러
        mem_size += addSize;
    }
    /** 배열의 크기가 변경(smaller)될 때 (adrs부터 subSize만큼 삭제) */
    void updateShrink(int adrs, int subSize) {
        if (adrs >= MEMORY_GLOBAL_MAX) throw tecBAD_ALLOCATE_MEMORY;
        vector<VarObj>::iterator it = mem.begin();
        //mem.erase(it + adrs, it + adrs + subSize);// 복사중 에러
        int iEnd = mem_size - subSize;
        for (int k=adrs; k<iEnd; k++) {
            mem[k].init(NON_T);
            mem[k] = mem[k + subSize];
        }
        for (int k=iEnd; k<mem_size; k++) 
            mem[k].init(NON_T);

        mem.erase(it + iEnd);
        mem_size -= subSize;
    }
    /** 배열의 데이터만 삭제 (capacity 유지) */
    void updateInsert(int adrs, int subSize, int lastArrayIdx, VarObj dt) {
        if (adrs >= MEMORY_GLOBAL_MAX) throw tecBAD_ALLOCATE_MEMORY;
        if (subSize < 1) throw tecNEED_UNSIGNED_INTEGER;
        vector<VarObj>::iterator it = mem.begin();
        cout << endl << " -updateInsert " << adrs << " " << subSize << " " << lastArrayIdx;
        lastArrayIdx -= subSize;
        int k=0;
        for (k=lastArrayIdx; k>=adrs; k--) {
            cout << endl << " -updateInsert -idx:" << (k+subSize);
            mem[k+subSize].init(NON_T);
            if (mem[k].getType() != NON_T){
                mem[k+subSize] = mem[k];
                cout << " -> " << mem[k+subSize].toFullString(true);
            }
        }
        for (int j=1; j<=subSize; j++) {
            cout << endl << " -InsertVar -idx:" << (k+j);
            mem[k+j].init(NON_T);
            mem[k+j] = dt;
        }
    }
    /** 배열의 데이터만 삭제 (capacity 유지) */
    void updateInsert(int adrs, int subSize, int lastArrayIdx, std::vector<VarObj>& dt, int lth) {
        if (adrs >= MEMORY_GLOBAL_MAX) throw tecBAD_ALLOCATE_MEMORY;
        if (subSize < 1) throw tecNEED_UNSIGNED_INTEGER;
        vector<VarObj>::iterator it = mem.begin();
        cout << endl << " -updateInsert " << adrs << " " << subSize << " " << lastArrayIdx;
        lastArrayIdx -= subSize;
        int k=0;
        for (k=lastArrayIdx; k>=adrs; k--) {
            cout << endl << " -updateInsert -idx:" << (k+subSize);
            mem[k+subSize].init(NON_T);
            if (mem[k].getType() != NON_T){
                mem[k+subSize] = mem[k];
                cout << " -> " << mem[k+subSize].toFullString(true);
            }
        }
        for (int j=1; j<=subSize; ) {
            cout << endl << " -InsertObjVar -idx:" << (k+j);
            for (int m=0; m<lth; m++) {
                mem[k+j].init(NON_T);
                mem[k+j] = dt[m];
                j++;
            }
        }
    }
    /** 배열의 데이터만 삭제 (capacity 유지) */
    void updateRemove(int adrs, int subSize, int lastArrayIdx) {
        if (adrs >= MEMORY_GLOBAL_MAX) throw tecBAD_ALLOCATE_MEMORY;
        if (subSize < 1) throw tecNEED_UNSIGNED_INTEGER;
        vector<VarObj>::iterator it = mem.begin();
        lastArrayIdx -= subSize;
        int k=0;
        for (k=adrs; k<lastArrayIdx; k++) {
            mem[k].init(NON_T);
            mem[k] = mem[k + subSize];
        }
        for (int j=0; j<subSize; j++) 
            mem[k+j].init(NON_T);
    }
    
    /** 디버깅용 */
    string to_string() {
        int max = mem.size();
        string str1("  ****** global memory (allocated:");
        str1.append(std::to_string(max)).append(")*********");
        for(int i=0; i< max; i++){
            VarObj dd = mem[i];
            if (i < 2 || dd.getDataTypeStr(dd.getType()).compare("NON_T") != 0)
                str1.append("\n").append(std::to_string(i)).append(" : ").append(dd.toFullString(true));
        }
        str1.append("\n").append("  ****** local memory (allocated:");
        str1.append(std::to_string(mem_local_size)).append(")**********");
        max = memLocal.size();
        for(int i=0; i< max; i++){
            VarObj dd = memLocal[i];
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
            while (!st.empty()) { 
                VarObj kk = st.top(); 
                st.pop(); 
                cout << endl << "  - " << kk.toFullString(true);  
            } 
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