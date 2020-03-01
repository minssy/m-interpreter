#ifndef MARE_EXECUTE_H_INCLUDED
#define MARE_EXECUTE_H_INCLUDED

#include "MareBase.h"

namespace mare_vm {

using namespace std;


class MareExecuter : public MareBase {

public:
    MareExecuter(MareUtil &util) : MareBase(0, util), returnValue() {}

    void prepareExecute(vector<string> const& params);
    void execute(string const& fname, vector<string> const& params);
    
protected:
    MareExecuter(int startPc, MareUtil &util) : MareBase(startPc, util), returnValue() {}

    VarObj returnValue;                            /* 함수의 반환 값 저장용 */
    SymKind varSymType;                            /* getMemAdrs 함수용 */

    MareStack mstk; 
    unsigned short readedCodeCnt;

    //CodeSet code;                                /* 현재 코드 셋 (MareBase로 이동) */
    int baseReg;                                   /* 베이스 레지스터 */
    int stpReg;                                    /* 스택 포인터 */
    int maxCodeLine;                               /* 프로그램 코드의 끝행 번호 */
    char *code_ptr;                                /* 내부 코드 분석용 포인터 */
    bool breakFlag, conFlag, returnFlag, exitFlag; /* 제어용 플래그 */

    /* 실행 */
    short prepareExecute(string const& fname, vector<string> const& params);
    void  execute(short startPc);

    void statement();
    void block();

    short opOrder(TknKind kd);
    void  binaryExpr(TknKind op);
    
    void setEndSubIf(bool& flg);
    void callFunction(short fncIdx);
    void execFunction(short fncIdx, short argCnt);
    void execSysFunc();

    int   getTopAdrs(CodeSet const& cd);
    short getEndLineOfIf(short line_);
    void  chkEofLine();

    TknKind lookCode(short line_);
    CodeSet firstCode(short line_);
    CodeSet nextCode();
    CodeSet chkNextCode(CodeSet const& cd, short kind2);
    
private:
    void setInitArray();
    void assignVariable(CodeSet const save, bool declare=false);

    int  getMemAdrs(CodeSet const& cd, SymKind& objType, DtType& varTp);
    VarObj getExpression(short kind1=0, short kind2=0);
    void expression(short kind1, short kind2);    
    void term(short n);
    void factor();

    short setParams(vector<VarObj>& vs, short numOfParams);
    
    void setPropertyRun(CodeSet const& varCode, SymKind sk);
    void updateSymTbl(int adrs, int diff, bool isFixedArray=false);   
};

}

#endif