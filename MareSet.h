#ifndef MARE_SET_H_INCLUDED
#define MARE_SET_H_INCLUDED

#include <iostream>
#include <fstream>  // 파일처리용
#include <sstream>  // 문자열 스트림
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <iomanip>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cctype>

#include <algorithm> 
#include <functional>
#include <locale>
#include <regex>

#include "Blob.h"

namespace mare_vm {

#define SHORT_SIZE sizeof(short int)           /* short int 형의 크기 */
#define SHORT_P(p) (short int *)(p)            /* short int 형 포인터로 변환 */
#define UCHAR_P(p) (unsigned char *)(p)        /* unsigned char 형 포인터로 변환 */

const static int MAJOR_VERSION = 1;
const static int MINER_VERSION = 0;

const static int LINE_SIZE               = 250;         /* 1 line에 가능한 최대 문자수 */
const static int LINE_SIZE_MAX           = 256;         /* 1 line을 읽기위한 버퍼 크기 */
const static int MAX_LINE                = 5000;        /* 프로그램에서 혀용되는 최대 행수 */
const static int MAX_EXE_LINE            = 102400;      /* 프로그램 실행시, 혀용되는 최대 행수 */
const static int MAX_MEMORY              = 60000;       /* 최대 메모리 주소 */
const static int MAX_ARRAY               = 10000;       /* 배열 등의 최대 크기 */
const static int NOT_DEFINED_ARRAY       = 65535;       /* 배열 등의 크기가 미지정된 경우 */
const static int MEMORY_BACK_RESIZE      = 20;
const static int VAR_LITERAL_OFFSET      = 20;          /* 변수타입과 리터럴 타입의 코드값 Offset */
const static int MEMORY_RESIZE           = 32;          /* 기본 메모리 할당 크기 */
const static int LEN_DECIMAL_POINTS      = 8;           /* double형의 소수점 자릿수 */
const static double CAL_DECIMAL_POINTS   = pow(10, LEN_DECIMAL_POINTS); /* double형의 소수점 계산값 */
const static int MEMORY_GLOBAL_MAX       = 100000;      /* global 메모리 최대 크기 (local 메모리 시작) */

/** 코드 (토큰) 종류 정의 */
enum TknKind {

  Void=0,
  VarDbl, 
  VarInt, 
  VarStr, 
  VarDateTime, 
  VarStruct,
  ArrayList,
  
  // 09 = 'HT(Horizontal Tab)', 10 = 'LF(Line Feed)', 13 = 'CR(Carriage Return)'
  
  DblNum=21,      /* double type */
  IntNum,         /* int type */
  String,         /* string type */

  DeclareVar=30,
  DeclareArr,
  DeclareObj,
  Not='!',  DblQ='"', Mod='%', SglQ='\'', // 33, 34, 37, 39
  Lparen='(', Rparen=')',  // 40, 41
  Multi='*', Plus='+', Comma=',', Minus='-', Dot='.', Divi='/', // 42, 43, 44, 45, 46, 47
  // 48~57: Number (0~9)
  Colon=':', Semicolon=';', Assign='=', Ifsub='?', // 58, 59, 61, 63
  // 65 ~ 90: Alphabet (A~Z)
  Lbracket='[', Rbracket=']', // 91, 93
  // 97 ~ 122: Alphabet (a~z)
  Do='{', Close=125, // 123, 125('}')
  PlusAssign=127, MinusAssign, MultiAssign, DiviAssign, DBPlus, DBMinus, DBPlusR, DBMinusR,
  Equal, NotEq, Less, LessEq, Great, GreatEq, And, Or,

  Func=145, 
  If, Elif, Else, While, For, Foreach,
  End, Break, Return, Continue,
  // 단독형 함수
  Require=160, Exit, Log, Throws, 
  ToArray,

  /* 디버깅용 */
  Expression=231, // 일반식
  Math=235,
  System,
  GetProperty,
  SetProperty,
  StructItem, 
  /* 여기부터 고정 값 */
  Version=242,
  True=243,
  False,
  Ident,     // 일반식별자 (변수명이나 함수명)
  Doll,      // $ 문자로 시작하는 변수명
  Digit,     // 숫자 (1 char)
  Letter,    // 문자 (1 char)
  Fcall, 
  Gvar, 
  Lvar, 
  EofProg, EofLine, 
  Other,     // Unknown Type ??
  END_AllKey // 255 
};

/** 토큰 정의 */
struct Token {
    TknKind     kind;     /* 토큰 종류 */
    std::string text;     /* 토큰 문자열 */
    double      numVal;   /* 토큰 상수 값 */

    Token() { kind=Other; text=""; numVal=0.0; }
    Token(TknKind k) { kind=k; text=""; numVal=0.0; }
    Token(TknKind k, double d) { kind=k; text=""; numVal=d; }
    Token(TknKind k, std::string const& s) { kind=k; text=s; numVal=0.0; }
};

/** 변수 타입 */
enum DtType {
    NON_T = 0,
    DBL_T,
    INT_T,
    STR_T,
    DATETIME_T,
    OBJECT_T,
};

enum ExpressionType {
    NORMAL_TYPE = 1,
    CONDITIONAL_TYPE,         /* if, while 등의 조건식 */
    GET_ARGUMENT_TYPE,        /* 전달인자(argument): 함수 호출시, 전달되는 값 */
    SET_PARAMETER_TYPE,       /* 매개변수(parameter): 함수 정의부분에 정의된 변수 */
};

/** 심볼 테이블 등록되는 타입 */
enum SymKind {
    noId = 0,
    /* symbol 등록시 사용 */
    funcId,
    varId,
    paraId,
    /* symbol 등록중, varId, paraId로 부터 변경 */
    arrayId,
    arrListId,
    objectId,
};

/** 심볼 테이블 구성 */
struct SymTbl
{
    std::string      name;    /* 변수나 함수의 이름 */
    SymKind          symKind; /* 종류 (funcId, varId,...) */
    DtType           dtTyp;   /* 타입 (NON_T, DBL_T,...) 변수타입, 함수의 리턴타입 */

    unsigned short   adrs;    /* 변수/함수의 주소 */
    unsigned short   aryLen;  /* 배열길이, 0=단순변수 (func일 경우, 최소 인수개수) */
    unsigned short   args;    /* 함수의 인수 개수 (메모리 버퍼 크기) */
    unsigned short   frame;   /* 함수의 프래임 크기 (struct item 갯수) */

    SymTbl() { clear(); }
    void clear() {
        name = ""; symKind=noId; dtTyp=NON_T; 
        adrs=0; aryLen=0; args=0; frame=1; 
    }

    std::string toFullString(bool isHumanReadable=false) {
        std::string str("");
        if (isHumanReadable) {
            str.reserve(45);      // 메모리 확보

            if (symKind == funcId) { str.append("Function "); }
            else if (symKind == varId) { str.append("Variable "); }
            else if (symKind == paraId) { str.append("Param "); }
            else if (symKind == arrayId) { str.append("Array "); }
            else if (symKind == arrListId) { str.append("ArrayList "); }
            else if (symKind == objectId) { str.append("Struct "); }
            else if (symKind == noId) { str.append("none-symbol "); }
            else { str.append("Wrong-symble-type:").append(std::to_string((int)symKind)); return str; }

            if (dtTyp == DBL_T) { str.append("double "); }
            else if (dtTyp == INT_T) { str.append("int "); }
            else if (dtTyp == STR_T) { str.append("string "); }
            else if (dtTyp == DATETIME_T) { str.append("datetime "); }
            else if (dtTyp == OBJECT_T) { str.append("struct "); }
            else if (dtTyp == NON_T) { str.append("void(none) "); }
            else { str.append("Wrong-data-type:").append(std::to_string((int)dtTyp)); return str; }
        }
        else {
            str.reserve(30);      // 메모리 확보
            str.append(std::to_string((unsigned short)symKind));
            str.append(" ").append(std::to_string((unsigned short)dtTyp));
        }
        str.append(" ").append(std::to_string(adrs));
        str.append(" ").append(std::to_string(aryLen));

        str.append(" ").append(std::to_string(args));
        str.append(" ").append(std::to_string(frame));

        str.append(" ").append(name);
        return str;
    }
};

struct ItemTbl 
{
    unsigned short   symId;   /* struct의 symtbl id */
    std::string      name;    /* Item의 이름 */
    DtType           dtTyp;   /* 타입 (NON_T, DBL_T,...) 변수타입 */
    DtType           initTyp;
    double           initVal; /* 초기값 */
    std::string      initStr; /* 초기값 */

    ItemTbl() { clear(); }
    void clear() {
        name = ""; symId=0; dtTyp=NON_T; 
        initTyp=NON_T; initVal=0; initStr=""; 
    }

    std::string toFullString(bool isHumanReadable=false) {
        std::string str("");
        if (isHumanReadable) {
            str.reserve(40);      // 메모리 확보

            if (dtTyp == DBL_T) { str.append("double "); }
            else if (dtTyp == INT_T) { str.append("int "); }
            else if (dtTyp == STR_T) { str.append("string "); }
            else if (dtTyp == DATETIME_T) { str.append("datetime "); }
            else if (dtTyp == NON_T) { str.append("void(none) "); }
            else { str.append("Wrong-data-type:").append(std::to_string((int)dtTyp)); return str; }
        }
        else {
            str.reserve(25);      // 메모리 확보
            str.append(std::to_string((unsigned short)dtTyp));
        }
        str.append(" ").append(std::to_string(symId));
        str.append(" ").append(name).append(" ");
        str.append(" ").append(std::to_string(initTyp));
        if (initTyp == NON_T) return str;
        if (initTyp == STR_T)
            str.append(" ").append(initStr);
        else {
            //string str = to_string_with_precision(initVal);
            //to_string_simple(str);
            str.append(" ").append(std::to_string(initVal));
        }

        return str;
    }
};

/** 내부 실행 코드 구성 */
struct CodeSet
{
    TknKind     kind;         /* 종류 */
    char const *text;         /* 문자열 리터럴일 때의 위치 */
    double      numVal;       /* 수치 상수일때의 값 */
    short       symIdx;       /* 심볼 테이블에 대한 인덱스 */
    short       jmpAdrs;      /* 점프할 주소 */

    CodeSet() { clear(); kind=Other; }
    CodeSet(TknKind k) { clear(); kind=k; }                         /* code only (+, -, ) */
    CodeSet(TknKind k, double d) { clear(); kind=k; numVal=d; }     /* literal (int, double, datetime) */
    CodeSet(TknKind k, char const *s) { clear(); kind=k; text=s; }  /* literal (string, address, amount) */
    CodeSet(TknKind k, short sym, short jmp) { clear(); kind=k; symIdx=sym; jmpAdrs=jmp; } /* func, for, etc. */
    void clear() {
        text = ""; numVal=0.0; symIdx=0; jmpAdrs=0;
    }
};

enum ExecuteMode {
    PARSING_MODE,
    VERIFY_MODE,
    EXECUTE_MODE
};

/** 에러 처리 (종료) */
struct ErrObj
{
    ExecuteMode mode;
    int         tec;          /* 에러 코드 */
    short       line;         /* 에러 발생 CODE Line 정보 */
    std::string msg;          /* 에러 메세지 */
    std::string code;         /* 실행중인 CODE 정보 */

    ErrObj(ExecuteMode mode_, int tec_, short line_, std::string const& msg_, std::string code_) { 
        mode = mode_; tec = tec_; 
        line = line_; msg=msg_; code=code_; 
    }

    ErrObj(ExecuteMode mode_, int tec_, short line_, std::string const& msg_, std::string token_, std::string value_) { 
        mode = mode_; tec = tec_; 
        line = line_; msg=msg_; 
        if (value_ != "") code=token_ + "(" + value_ + ")"; 
        else code=token_;
    }

    std::string whatStr() {
        std::string err("SCRIPT ERROR: \n-status: ");        
        if (mode == EXECUTE_MODE) err.append("EXECUTE CODE");
        else if (mode == PARSING_MODE) err.append("PARSE SCRIPT");
        else err.append("VERIFY SYNTAX");
        err.append(" \n-error_code: ").append(std::to_string(tec));
        err.append(" \n-error: ").append(transToken(tec));
        err.append(" \n-message: ").append(msg);
        err.append(" \n-line: ").append(std::to_string(line));
        if (!code.empty()) { 
            if (mode == PARSING_MODE) err.append(" \n-token: ");
            else err.append(" \n-code: ");
            err.append(code);
        }
        return err;
    }

};

}

#endif
