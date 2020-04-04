#ifndef MARE_BASE_H_INCLUDED
#define MARE_BASE_H_INCLUDED

#include "VarObj.h"
#include "MareSet.h"
#include "MareMemory.h"
#include "MareUtil.h"
#include "MareConverter.h"
#include <map>

namespace mare_vm {

using namespace std;

class MareBase {

public:
    MareBase(int startPc, MareUtil &util) : mutil(util), srcLineNo(0), Pc(startPc), isUpdatedSymbols(false) { }
    ~MareBase() {
        cout << endl << " * disappearing...";
        intenalCode.clear();
        Gtable.clear();
        Ltable.clear();
        Itable.clear();
        ObjectMap.clear();
        nbrLITERAL.clear();
        strLITERAL.clear();
        dbgCode.clear();
        cout << "done." << endl;
    }

    /* 디버깅용 */
    void debugging(string const& msg, int lvl);
    string printInfos(bool all=true);

    /** 에러 처리 */
    void errorExit(int err, string a="\1", string b="\1");

    /** Ledger Data Format <-> Contract Data Format */
    Blob getInterCode();
    Blob getSymbolTable();
    Blob getStructSymbol();
    Blob getLiterals();
    Blob getLogs();
    Blob getMemories();

    bool setInterCode(Blob const& obj);
    bool setSymbolTable(Blob const& obj);
    bool setStructSymbolTable(Blob const& obj);
    bool setLiterals(Blob const& obj);
    bool setMemories(Blob const& priobj);
    bool isUpdatedSymbolTable() { return isUpdatedSymbols; }

protected: 
    MareUtil mutil;

    vector<char*>   intenalCode;           /* 변환이 끝난, 내부 코드 저장 */
    vector<SymTbl>  Gtable;                /* Global Variable Symbol Table */
    vector<SymTbl>  Ltable;                /* Local Variable Symbol Table */
    vector<ItemTbl> Itable;                /* Struct Item Symbol Table */
    map<string, int> ObjectMap;            /* Struct 키워드 맵  */
    MareMemory      DynamicMem;            /* 메모리 (변수 값 저장) */
    vector<double>  nbrLITERAL;            /* 수치 리터럴 저장 */
    vector<string>  strLITERAL;            /* 문자열 리터럴 저장 */
    vector<VarObj>  tmpStructObj;          /* struct obj를 담을 임시 공간 */

    Token token;                           /* 현재 처리중인 토큰 (interperter) */
    CodeSet code;                          /* 현재 코드 셋 (InitExec, Executer) */
    vector<CodeSet> dbgCode;               /* 디버그를 위한 기록용 */

    short srcLineNo;                       /* 소스의 행 번호 */
    short Pc;                              /* 프로그램 카운터 (-1이면, 실행중 아님) */
    bool  isUpdatedSymbols;                /* symbol table update 여부 */
    bool  chkSyntaxMode = false;           /* 구문 검사인지, 실행 상태인지 설정 */
    short blkNest;                         /* 블록의 깊이 정보 */

    char codeBuf[LINE_SIZE+1], *codeBuf_p; /* 내부 코드 생성 작업용 */

    static map<string, TknKind> kindMap;   /* 소스 코드와 내부 코드의 키워드 맵 */
    static const TknKind ctyp[256];        /* 문자 종류표 배열 */ 

    // Code
    void  setCode(int cd);
    short setCode(int cd, short nbr);
    void  pushInternalCode();

    short setLITERAL(double d);
    short setLITERAL(string const& s);

    short getLineNo();

    // Symbol Table
    vector<SymTbl>::iterator symTablePt(CodeSet const& cd);
    short getSymTable(string const& name, SymTbl& symb);
 
    string kind2Str(CodeSet const& cd);    /* code set의 종류, 이름 또는 값 */
    static string kind2Str(TknKind kd);
    static string transExpression(CodeSet const& cd) {
        switch (cd.symIdx) {
            case NORMAL_TYPE:
                return to_string(cd.jmpAdrs);
            case CONDITIONAL_TYPE:
                if (cd.jmpAdrs == 0) return "condition";
                else if (cd.jmpAdrs == 1) return "init";
                else return "loop";
            case GET_ARGUMENT_TYPE:
                return "Argument:" + to_string(cd.jmpAdrs + 1);
            case SET_PARAMETER_TYPE:
                return "Parameter:" + to_string(cd.jmpAdrs + 1);
        }
        return kind2Str(cd.kind) + "??";
    }

    /* 디버깅용 */
    static string debugInterCode(char* msg, short lineNo);

    short countExps;
    inline void initDbgCode() {
        cout << endl << "***** reset dbg" << blkNest;
        countExps = 0;
        if (blkNest == 0) dbgCode.clear();
        else dbgCode.erase(dbgCode.begin() + blkNest, dbgCode.end());
        dbgdbg();
    }
    inline void initDbgCode(CodeSet cs) {
        initDbgCode();
        dbgCode.push_back(cs);
        cout << " new:" << kind2Str(cs.kind);
    }
    inline void addDbgCode(CodeSet cs) {
        cout << endl << "***** add dbg"; 
        dbgCode.push_back(cs);
        dbgdbg();
    }
    inline void addDbgCode(TknKind const tk, short const sym, short const jmp) {
        dbgCode.push_back(CodeSet(tk, sym, jmp));
    }
    inline void changeDbgCode(CodeSet cs) {
        cout << endl << "***** chg dbg";
        if (dbgCode.size() > 0) dbgCode.pop_back();
        dbgCode.push_back(cs);
        dbgdbg();
    }
    inline void changeDbgCode(TknKind const tk, short const sym, short const jmp) {
        changeDbgCode(CodeSet(tk, sym, jmp));
    }
    inline void removeDbgCode() {
        cout << endl << "***** del dbg"; 
        if (dbgCode.size() > 0) dbgCode.pop_back();
        else cout << endl << "*******  remove dbg fail why?????  *** ";
        dbgdbg();
    }
    inline string getCodeSetStr(CodeSet& cs) {
        string s1 = kind2Str(cs.kind);
        string s2 = kind2Str(cs);
        if (s1 == s2) return s1;
        else return s1.append("(").append(s2).append(")");
    }
    inline void dbgdbg() {
        cout << " (lvl " << blkNest << ":" << dbgCode.size() << ") ";
        for(CodeSet c : dbgCode){
            cout << kind2Str(c.kind) << " ";
        }
    }

private:
    static map<TknKind, string> kindMapDBG;  /* 소스 코드와 내부 코드의 키워드 맵 */
    static map<TknKind, short>  kindMapNUM;  /* 내부 코드별 숫자정보 */
    static map<string, TknKind> initKindMap(); 
    static map<TknKind, string> initKindDBGMap();
    static map<TknKind, short>  initKindNUMMap();
    static short kind2Skip(TknKind kd);
};

}

#endif