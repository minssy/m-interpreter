#ifndef MARE_INTERP_H_INCLUDED
#define MARE_INTERP_H_INCLUDED

#include "MareBase.h"
#include "MareInitExec.h"

namespace mare_vm {

using namespace std;

class MareInterpreter : public MareInitExec {

public:
    MareInterpreter(MareUtil& util) : MareInitExec(util) {}

    void convertToInterCode(vector<string>& src);
    Blob getFuncInfos();

private:

    const static short NO_FIX_ADRS = 0;        /* 아직 주소가 미정인 항목 처리용 */

    short srcMaxLineNo;
    vector<string> srcWork;
    vector<string> srcNew;
    vector<string> funcInfos;                  /* 함수 실행 정보 */

    // Parsing
    //Token token;                             /* 현재 처리중인 토큰 정보 (MareBase로 이동) */
    SymTbl tmpTb;                              /* 정보 저장용 임시 심볼 테이블 */
    //int blkNest;                             /* 블록의 깊이 정보 (MareBase로 이동) */
    int localAdrs;                             /* 로컬 변수 주소 관리 */
    
    short loopNest;                            /* 블록의 깊이 정보 (check for 루프 - break,continue) */
    bool  funcDeclFlag;                        /* 함수 정의 처리 중이면 참 */
    bool  chkDo;                               /* 블록 시작 체크용 플래그 */
    //char codeBuf[LINE_SIZE+1], *codeBuf_p;   /* 내부 코드 생성 작업용 (MareBase로 이동) */

    int startLtable;           		           /* 로컬용 테이블 시작 주소 위치 */

    // Token
    char *token_p;                             /* 1문자 획득용 문자위치 */
    char tmpBuf[LINE_SIZE_MAX];                /* 소스의 1line을 읽어 저장할 배열 */
    bool endOfFileFlag;                        /* 파일 종료 플래그 */

    void convertAll(vector<string>& src);
    // Parsing
    void convert();
    void convertForRest();
    void convertVarAssign(bool literalOnly=false);
    void convertBlockSet();
    void convertBlockSetIf();
    void convertBlock();

    void varDeclare(short const varType);
    void chkVarName(Token const& tk);
    void setSymName(short const dtType);
    void setSymAryLen();
    void arrayListDeclare();
    void structDeclare();
    void funcDeclare();
    void backPatch(short line, short endLine);

    void setCodeEnd();
    void setCodeEofLine(bool DoEnable=false);

    bool isLocalScope();
    long getTimeSpanOfStr(string const& d);
    bool chkTimeSpanOfStr(int idx, bool valueIsOne);

    // Token
    Token nextLineTkn();
    Token nextTkn();
    Token chkNextTkn(Token const& tk, int kind2);

    static bool isOperator(char c1, char c2);
    static void chkKinds(Token const& tk);
    
    // Symbol Table
    short enter(SymTbl& tb, SymKind kind);
    void setStartLtable();
    bool isLocalVarName(string const& name, SymKind kind);
    short searchName(string const& sn, int mode);

};

}

#endif