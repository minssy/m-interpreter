

#include "MareBase.h"

namespace mare_vm {

using namespace std;

map<string, TknKind> MareBase::kindMap = initKindMap();
map<TknKind, string> MareBase::kindMapDBG = initKindDBGMap();
map<TknKind, short>  MareBase::kindMapNUM = initKindNUMMap();

const TknKind MareBase::ctyp[256] = {
/* 001- */ Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other, 
/* 016- */ Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other, 
/* 032- */ Other,  Not,    DblQ,   Other,  Doll,   Mod,    Other,  SglQ,   Lparen, Rparen, Multi,  Plus,   Comma,  Minus,  Dot,    Divi,
/* 048- */ Digit,  Digit,  Digit,  Digit,  Digit,  Digit,  Digit,  Digit,  Digit,  Digit,  Colon,  Semicolon,Less, Assign, Great,  Ifsub, 
/* 064- */ Other,  Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, 
/* 080- */ Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Lbracket,Other, Rbracket,Other, Letter, 
/* 096- */ Other,  Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, 
/* 112- */ Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter, Do,     Other,  Close,  Other,  Other, 
/* 128- */ Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other, 
/* 144- */ Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other, 
/* 160- */ Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  
/* 176- */ Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other, 
/* 192- */ Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other, 
/* 208- */ Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,
/* 224- */ Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other, 
/* 240- */ Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other };

/** contract code의 parsing관련된 keyword 목록 */
map<string, TknKind> 
MareBase::initKindMap() {

    map<string, TknKind> tmpKinds;       
    tmpKinds.insert(make_pair("\"", DblQ));        tmpKinds.insert(make_pair("'", SglQ));

    tmpKinds.insert(make_pair("!", Not));          tmpKinds.insert(make_pair("?", Ifsub));
    tmpKinds.insert(make_pair(",", Comma));        tmpKinds.insert(make_pair(".", Dot));
    tmpKinds.insert(make_pair(";", Semicolon));    tmpKinds.insert(make_pair(":", Colon));

    tmpKinds.insert(make_pair("(", Lparen));       tmpKinds.insert(make_pair(")", Rparen));
    tmpKinds.insert(make_pair("[", Lbracket));     tmpKinds.insert(make_pair("]", Rbracket));
    tmpKinds.insert(make_pair("{", Do));           tmpKinds.insert(make_pair("}", Close));

    tmpKinds.insert(make_pair("+", Plus));         tmpKinds.insert(make_pair("-", Minus));
    tmpKinds.insert(make_pair("*", Multi));        tmpKinds.insert(make_pair("/", Divi));
    tmpKinds.insert(make_pair("%", Mod));
    tmpKinds.insert(make_pair("=", Assign));
    tmpKinds.insert(make_pair("++", DBPlus));      tmpKinds.insert(make_pair("--", DBMinus));
    tmpKinds.insert(make_pair("+=", PlusAssign));  tmpKinds.insert(make_pair("-=", MinusAssign));
    tmpKinds.insert(make_pair("*=", MultiAssign)); tmpKinds.insert(make_pair("/=", DiviAssign));
    tmpKinds.insert(make_pair("==", Equal));       tmpKinds.insert(make_pair("!=", NotEq));
    tmpKinds.insert(make_pair("<", Less));         tmpKinds.insert(make_pair("<=", LessEq));
    tmpKinds.insert(make_pair(">", Great));        tmpKinds.insert(make_pair(">=", GreatEq));
    tmpKinds.insert(make_pair("&&", And));         tmpKinds.insert(make_pair("||", Or));

    tmpKinds.insert(make_pair("void", Void));
    tmpKinds.insert(make_pair("double", VarDbl)); 
    tmpKinds.insert(make_pair("int", VarInt));
    tmpKinds.insert(make_pair("string", VarStr));
    tmpKinds.insert(make_pair("datetime", VarDateTime));

    tmpKinds.insert(make_pair("arrayList", ArrayList));
    tmpKinds.insert(make_pair("struct", Struct));

    tmpKinds.insert(make_pair("func", Func));      tmpKinds.insert(make_pair("function", Func));

    tmpKinds.insert(make_pair("if", If));
    tmpKinds.insert(make_pair("else", Else));
    tmpKinds.insert(make_pair("while", While));
    tmpKinds.insert(make_pair("for", For));
    tmpKinds.insert(make_pair("foreach", Foreach));
    tmpKinds.insert(make_pair("break", Break));
    tmpKinds.insert(make_pair("continue", Continue));
    tmpKinds.insert(make_pair("return", Return));

    tmpKinds.insert(make_pair("system", System));
    tmpKinds.insert(make_pair("math", Math));

    tmpKinds.insert(make_pair("true", True));
    tmpKinds.insert(make_pair("false", False));

    // 내장 함수 
    tmpKinds.insert(make_pair("throw", Throws));
    tmpKinds.insert(make_pair("log", Log));
    tmpKinds.insert(make_pair("require", Require));    
    tmpKinds.insert(make_pair("exit", Exit));
    tmpKinds.insert(make_pair("toArray", ToArray));

    return tmpKinds;
}

/** keyword 목록 (contract code의 parsing keyword + internal code관련 keyword 추가) */
map<TknKind, string> 
MareBase::initKindDBGMap() {

    map<TknKind, string> kindDBG;
    map<string, TknKind>::iterator iter;
    for (iter = kindMap.begin(); iter != kindMap.end(); ++iter){
        kindDBG.insert(make_pair((*iter).second, (*iter).first));
    }
    // 내부적으로만 사용됨. (디버깅, 오류처리용 추가)
    kindDBG.insert(make_pair(Fcall, "funcCall"));
    kindDBG.insert(make_pair(Gvar, "gVar"));
    kindDBG.insert(make_pair(Lvar, "lVar"));
    kindDBG.insert(make_pair(EofProg, "EndProgram"));
    kindDBG.insert(make_pair(EofLine, "EndLine"));
    
    kindDBG.insert(make_pair(Elif, "elseif"));
    kindDBG.insert(make_pair(End, "endBlock"));

    kindDBG.insert(make_pair(DblNum, "DblNum"));
    kindDBG.insert(make_pair(IntNum, "IntNum"));
    kindDBG.insert(make_pair(String, "String"));

    kindDBG.insert(make_pair(DeclareVar, "declareVar"));
    kindDBG.insert(make_pair(DeclareArr, "declareArray"));

    kindDBG.insert(make_pair(DBPlusR, "++"));
    kindDBG.insert(make_pair(DBMinusR, "--"));

    kindDBG.insert(make_pair(Doll, "Doll"));
    kindDBG.insert(make_pair(Ident, "Ident"));
    kindDBG.insert(make_pair(GetProperty, "GetProperty"));
    kindDBG.insert(make_pair(SetProperty, "SetProperty"));
    kindDBG.insert(make_pair(StructItem, "StructItem"));

    kindDBG.insert(make_pair(Expression, "Expression"));
    kindDBG.insert(make_pair(Version, "Version"));

    return kindDBG;
}

/** keyword 목록 (contract code의 parsing keyword + internal code관련 keyword 추가) */
map<TknKind, short> 
MareBase::initKindNUMMap() {

    map<TknKind, short> kindMapNum;
    kindMapNum.insert(make_pair(Func, 2));
    kindMapNum.insert(make_pair(Fcall, 2));
    kindMapNum.insert(make_pair(Gvar, 2));
    kindMapNum.insert(make_pair(Lvar, 2));

    kindMapNum.insert(make_pair(If, 2));
    kindMapNum.insert(make_pair(Elif, 2));
    kindMapNum.insert(make_pair(Else, 2));
    kindMapNum.insert(make_pair(For, 2));
    kindMapNum.insert(make_pair(Foreach, 2));
    kindMapNum.insert(make_pair(While, 2));

    kindMapNum.insert(make_pair(DblNum, 2));
    kindMapNum.insert(make_pair(IntNum, 2));
    kindMapNum.insert(make_pair(String, 2));

    kindMapNum.insert(make_pair(System, 2));
    kindMapNum.insert(make_pair(GetProperty, 2));
    kindMapNum.insert(make_pair(SetProperty, 2));
    kindMapNum.insert(make_pair(StructItem, 2));
    kindMapNum.insert(make_pair(Math, 2));
    kindMapNum.insert(make_pair(Throws, 2));
    
    kindMapNum.insert(make_pair(Version, 2));

    return kindMapNum;
}
short 
MareBase::kind2Skip(TknKind kd) 
{
    auto m_iter = kindMapNUM.find(kd);
    if (m_iter != kindMapNUM.end()){
        return m_iter->second;
    }
    return 0;
}

/** CodeSet 키워드 → 문자열 */
string 
MareBase::kind2Str(TknKind kd) 
{
    auto m_iter = kindMapDBG.find(kd);
    if (m_iter != kindMapDBG.end()){
        return m_iter->second;
    }
    //std::cout << std::endl << "  ** kind2Str : " << (short)kd;
    return "";
}

/** CodeSet 값 (혹은 키워드) → 문자열 */
string 
MareBase::kind2Str(CodeSet const& cd) 
{
    switch (cd.kind) {
    case Lvar: case Gvar: case Fcall: return symTablePt(cd)->name;
    case IntNum: return to_string((long)cd.numVal);
    case DblNum: return to_string(cd.numVal);
    case String: return string("\"") + cd.text + "\"";
    case System: case GetProperty: case SetProperty: case Math:
        return /*kind2Str(cd.kind) + "." +*/ mutil.getSubCmdStr(cd.symIdx);
    case Throws: 
        return transToken(cd.symIdx);
    case Expression: 
        return transExpression(cd);
    case EofLine: return "";
    default: return kind2Str(cd.kind);
    }
}
/** 
 * 이름으로 SymTbl 정보를 가져오며, 참조를 위한 SymTbl의 id를 반환. 
 * 함수명을 찾을 수 없는 경우 -1 반환 */
short
MareBase::getSymTable(string const& name, SymTbl& symb) {

    short max = Gtable.size();
    short i=0;
    while (i<max){
        SymTbl tb = Gtable[i];
        if (name.compare(tb.name) == 0){
            symb = tb; 
            return i;
        }
        i++;
    }
    return -1;
}

/** (함수, 변수) 반복자 획득 */
vector<SymTbl>::iterator 
MareBase::symTablePt(CodeSet const& cd) 
{
    if (cd.kind == Lvar) 
        return Ltable.begin() + cd.symIdx;    /* Lvar */
    if (cd.kind == Gvar || cd.kind == Fcall)
        return Gtable.begin() + cd.symIdx;    /* Gvar or Fcall */

    errorExit(tecINCORRECT_SYNTAX, "Invalid code.", "Required function or variable only.");
    return Gtable.begin();
}

/** 읽기 or 실행 중 위치 */
short 
MareBase::getLineNo() 
{
    return (Pc == -1) ? srcLineNo : Pc;       /* 분석 중 : 실행 중 */
}

/** 수치 리터럴 추가 */
short 
MareBase::setLITERAL(double d) 
{
    chkINT64(d);
    for (short n=0; n<(short)nbrLITERAL.size(); n++) {
        if (nbrLITERAL[n] == d) return n;     /* 같은 값이 존재하면, 첨자 위치를 반환 */
    }
    nbrLITERAL.push_back(d);                  /* 수치 리터럴을 저장 */
    return nbrLITERAL.size() - 1;             /*저장된 수치 리터럴의 첨자 위치를 반환 */
}

/** 문자열 리터럴 추가 */
short 
MareBase::setLITERAL(string const& s) 
{
    for (int n=0; n<(int)strLITERAL.size(); n++) {
        if (strLITERAL[n] == s) return n;     /* 같은 값이 존재하면, 첨자 위치를 반환 */
    }
    strLITERAL.push_back(s);                  /* 문자열 리터럴을 저장 */
    return strLITERAL.size() - 1;             /* 저장된 문자열 리터럴의 첨자 위치를 반환 */
}

/** 코드 저장 */
void 
MareBase::setCode(int cd) 
{
    *codeBuf_p++ = (char)cd;
}

/** 코드와 short값 저장후, 현재 line번호 반환 */
short 
MareBase::setCode(int cd, short nbr) 
{
    *codeBuf_p++ = (char)cd;
    *SHORT_P(codeBuf_p) = nbr;
    codeBuf_p += SHORT_SIZE;
    return getLineNo();                       /* backPatch 용으로 저장할 행을 반환 */
}

/** 변환된 내부 코드를 저장 */
void 
MareBase::pushInternalCode() 
{
    short len;
    char *p;

    *codeBuf_p++ = '\0';
    if ((len = codeBuf_p-codeBuf) >= LINE_SIZE)
        errorExit(tecEXCEED_CODE_LENGTH, "Internal code after conversion is too long.");

    try {
        unsigned char lth = len;              /* INTERCODE_LEN */
        len++;
        p = new char[len];                    /* 메모리 확보 */
        memset(p, lth, len);
        //memcpy(p, codeBuf, len);
        for(short i=0; i<lth; i++){
            p[i+1] = codeBuf[i];
        }
        intenalCode.push_back(p);

        JLOG(mutil.j_.debug()) << " ***** insert new intenal Code [line:" << intenalCode.size() << "] *****";
        JLOG(mutil.j_.debug()) << debugInterCode(p, intenalCode.size());
    }
    catch (bad_alloc) { 
        errorExit(tecBAD_ALLOCATE_MEMORY, "[std::bad_alloc]", "Not enough memory.");
    }
    codeBuf_p = codeBuf;                      /* 다음 처리를 대비해 저장할 곳 맨 앞으로 위치지정 */
}

/** 오류 표시 및 실행 종료 */
void 
MareBase::errorExit(int err, string a, string b) {

    string msg("");
    if (a != "\1") { msg.append(a);
    if (b != "\1") { msg.append(" ").append(b);
    }}
    
    JLOG(mutil.j_.trace()) << " [Exit by Contract Exception] -line: "
                           << getLineNo() << " " << transToken(err)
                           << " -code: " << to_string(err) << " -message: " << msg;
    
    JLOG(mutil.j_.trace()) << printInfos();
    JLOG(mutil.j_.trace()) << mutil.getLedgerLog();

    //throw err;
    if (Pc == -1) {
        if (token.kind == IntNum)
            throw ErrObj(PARSING_MODE, err, getLineNo(), msg, kind2Str(token.kind), to_string((long)token.numVal));
        if (token.kind == DblNum)
            throw ErrObj(PARSING_MODE, err, getLineNo(), msg, kind2Str(token.kind), to_string(token.numVal));
        
        throw ErrObj(PARSING_MODE, err, getLineNo(), msg, kind2Str(token.kind), token.text);
    }
    else {
        // Syntax or Execute Mode
        string dbgCodeStr = "";
        short i;
        short sz = dbgCode.size();
        if (sz > 0) {
            --sz;
            for (i=0; i<sz; i++) {
                dbgCodeStr.append(getCodeSetStr(dbgCode[i]));
                dbgCodeStr.append(" -> ");
            }
            dbgCodeStr.append(getCodeSetStr(dbgCode[i]));
            if ((code.kind != EofLine && code.kind != Dot) && code.kind != dbgCode[i].kind) {
                dbgCodeStr.append(" -> ");
                dbgCodeStr.append(getCodeSetStr(code));
            }
        }

        if (chkSyntaxMode)
            throw ErrObj(VERIFY_MODE, err, getLineNo(), msg, dbgCodeStr);
        else 
            throw ErrObj(EXECUTE_MODE, err, getLineNo(), msg, dbgCodeStr);
    }
}

/* ============ 디버깅용 ============ */
string 
MareBase::printInfos(bool all) {

    int max = 0;
    int i = 0;
    if (all) {
        max =  intenalCode.size();
        cout << endl << "  ************** intenalCode (" << max << ")************ ";
        for (i=0; i<max; i++){
            char* tt = intenalCode[i];
            cout << endl << debugInterCode(tt, i);
        }

        max =  nbrLITERAL.size();
        cout << endl << "  ************** number LITERAL (" << max << ")********* ";
        for (i=0; i<max; i++){
            cout << endl << " " << i << ": " << nbrLITERAL[i];
        }
        max =  strLITERAL.size();
        cout << endl << "  ************** string LITERAL (" << max << ")********* ";
        for (i=0; i<max; i++){
            cout << endl << " " << i << ": '" << strLITERAL[i] << "'";
        }
    }
    max =  Gtable.size();
    cout << endl << "  ************** Global Table (" << max << ")*********** ";
    cout << endl << "id kind type adr len args frame : name";
    for (i=0; i<max; i++){
        SymTbl tb = Gtable[i];
        cout << endl << " " << i << " : " << tb.symKind << "  " << (int)tb.dtTyp << "   "
             << tb.adrs << "    " << tb.aryLen << "   " << tb.args << "   " << tb.frame << "    :  " << tb.name;
    }
    max =  Ltable.size();
    cout << endl << "  ************** Local Table (" << max << ")************ "; 
    for (i=0; i<max; i++){
        SymTbl tb = Ltable[i];
        cout << endl << " " << i << " : " << tb.symKind << "  " << (int)tb.dtTyp << "   "
             << tb.adrs << "    " << tb.aryLen << "   " << tb.args << "   " << tb.frame << "    :  " << tb.name;
    }

    cout << endl << DynamicMem.to_string();

    cout << endl << "  ******************************************" << endl;
    return "";
}

/* ============ 디버깅용 ============ */
void 
MareBase::debugging(string const& msg, int lvl) {
    
    string spaces = "";
    JLOG(mutil.j_.info()) << " ";

    while (lvl > 0){
        lvl--;
        spaces += "  ";
    }
    JLOG(mutil.j_.info()) << spaces << "'" << msg << "' -line:" << getLineNo();
    JLOG(mutil.j_.info()) << " ";
}

/* ============ 디버깅용 ============ */
string 
MareBase::debugInterCode(char* msg, short lineNo) {
    
    string str(" * line ");
    unsigned char length = msg[0];
    str.append(to_string(lineNo)).append(": ");
    str.append(to_string(length));
    unsigned char cc;
    for (int i=1; i<length;) {
        cc = msg[i++];
        int skip = kind2Skip((TknKind)cc);
        string msgx = kind2Str((TknKind)cc);
        str.append(" ").append(msgx == "" ? to_string(cc) : msgx);
        while (skip > 0){
            cc = msg[i++];  skip--;
            str.append(" ").append(to_string(cc));            
        }
    }
    return str;
}

Blob 
MareBase::getInterCode() {
    return toBlob(intenalCode);
}

Blob 
MareBase::getSymbolTable() {
    return toBlob(Gtable, Ltable);
}

Blob 
MareBase::getLiterals() {
    return toBlob(nbrLITERAL, strLITERAL);
}

Blob 
MareBase::getMemories() {
    return toBlob(DynamicMem, Gtable);    
}

Blob
MareBase::getLogs() {
    return toBlob(mutil.getLedgerLog());
}

bool 
MareBase::setInterCode(Blob const& obj) {
    return fromBlob2InterCode(obj, intenalCode);
}

bool 
MareBase::setSymbolTable(Blob const& obj) {
    return fromBlob2SymTbl(obj, Gtable, Ltable);
}

bool 
MareBase::setLiterals(Blob const& obj) {
    return fromBlob2Literal(obj, nbrLITERAL, strLITERAL);
}

/** setSymbolTable 함수를 먼저 실행*/
bool 
MareBase::setMemories(Blob const& priobj) {
    return fromBlob2Memory(priobj, Gtable, DynamicMem);
}

}