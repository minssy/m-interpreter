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
const static int VAR_LITERAL_OFFSET      = 20;          /* 변수타입과 리터럴 타입의 코드값 Offset */
const static int MEMORY_RESIZE           = 32;          /* 기본 메모리 할당 크기 */
const static int LEN_DECIMAL_POINTS      = 8;           /* double형의 소수점 자릿수 */
const static double CAL_DECIMAL_POINTS   = pow(10, LEN_DECIMAL_POINTS); /* double형의 소수점 계산값 */

/** 코드 (토큰) 종류 정의 */
enum TknKind {

  Void=0,
  VarDbl, 
  VarInt, 
  VarStr, 
  VarDateTime, 

  // 09 = 'HT(Horizontal Tab)', 10 = 'LF(Line Feed)', 13 = 'CR(Carriage Return)'
  
  DblNum=21,      /* double type */
  IntNum,         /* int type */
  String,         /* string type */

  DeclareVar=31,
  DeclareArr,
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
 
  // 변수/배열 귀속형 함수
  //ToString=180, 
  /*Length,*/ //Size, Find,  

  /* 디버깅용 */
  Expression=231, // 일반식
  Math=235,
  System,
  GetProperty,
  SetProperty,
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
    funcId,
    varId,      
    paraId,
};  

/** 심볼 테이블 구성 */
struct SymTbl
{
    std::string      name;    /* 변수나 함수의 이름 */
    SymKind          symKind; /* 종류 (funcId, varId,...) */
    DtType           dtTyp;   /* 타입 (NON_T, DBL_T,...) 변수타입, 함수의 리턴타입 */

    unsigned short   adrs;    /* 변수/함수의 주소 */
    unsigned short   aryLen;  /* 배열길이, 0=단순변수 (func일 경우, 최소 인수개수) */
    unsigned short   args;    /* 함수의 인수 개수 */
    unsigned short   frame;   /* 함수의 프래임 크기 */

    SymTbl() { clear(); }
    void clear() {
        name = ""; symKind=noId; dtTyp=NON_T; 
        aryLen=0; args=0; adrs=0; frame=0; 
    }

    std::string toFullString(bool isHumanReadable=false) {
        std::string str("");
        if (isHumanReadable) {
            str.reserve(45);      // 메모리 확보

            if (symKind == funcId) { str.append("Function "); }
            else if (symKind == varId) { str.append("Variable "); }            
            else if (symKind == paraId) { str.append("Parameter "); }
            else if (symKind == noId) { str.append("none-symbol "); }
            else { str.append("Wrong-symble-type:").append(std::to_string((int)symKind)); return str; }

            if (dtTyp == DBL_T) { str.append("double "); }
            else if (dtTyp == INT_T) { str.append("int "); }
            else if (dtTyp == STR_T) { str.append("string "); }
            else if (dtTyp == DATETIME_T) { str.append("datetime "); }
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

        if (!isHumanReadable && symKind == funcId) { // func에서만 필요
            str.append(" ").append(std::to_string(args));
            str.append(" ").append(std::to_string(frame));
        }
        str.append(" ").append(name);
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

/** trim (copying) */
static inline std::string trimCopy(std::string s) {
    // left trim
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        std::not1(std::ptr_fun<int, int>(std::isspace))));
    // right trim
    s.erase(std::find_if(s.rbegin(), s.rend(),
        std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

static const std::regex INT_TYPE("[+-]?[0-9]+");
static const std::regex DOUBLE_TYPE("[+-]?[0-9]+[.]?[0-9]+");
static const std::regex UNSIGNED_INT_TYPE("[+]?[0-9]+");
static const std::regex NUM_ENG_TYPE("[a-zA-Z0-9]+");

static void isNumberStr(std::string const& str) {
    //str.find_first_not_of("0123456789.") == std::string::npos;

    if (str.find('.') == std::string::npos){
        if (!std::regex_match(str, INT_TYPE)){
            std::cout << std::endl << " int text:" << str;
            throw tecINVALID_VALUE;
        }
        return;
    }

    if (!std::regex_match(str, DOUBLE_TYPE)){
        std::cout << std::endl << " double text:" << str;
        throw tecINVALID_VALUE;
    }
}

static void isIntStr(std::string const& str) {

    if (!std::regex_match(str, INT_TYPE)){
        std::cout << std::endl << " int text:" << str;
        throw tecINVALID_VALUE;
    }
}

static void isUnsignedIntStr(std::string const& str) {

    if (!std::regex_match(str, UNSIGNED_INT_TYPE)){
        std::cout << std::endl << " datetime text:" << str;
        throw tecINVALID_VALUE;
    }
}

}

#endif
