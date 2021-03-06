
#ifndef BLOB_H_INCLUDED
#define BLOB_H_INCLUDED

#include <map>
#include <vector>

namespace mare_vm {

// Storage for linear binary data
using Blob = std::vector <unsigned char>;

enum TECcodes
{
    /* 에러가 발생한 위치 */
    //tecCONTRACT_PARSE_ERROR = 160,
    tecCONTRACT_SYNTAX_ERROR = 161,
    tecCONTRACT_PREPROCESS_ERROR,
    tecCONTRACT_EXECUTE_ERROR,
    tecCONTRACT_REQUIRE_FAIL,
    tecUNKNOWN_CONTRACT_ERROR,

        /* 에러 종류 */
    tecEXCEED_CODE_LENGTH,
    tecEXCEED_CODE_LINES,
    tecBAD_ALLOCATE_MEMORY,
    tecSTACK_UNDERFLOW,
    tecINVALID_SYSTEM_METHOD,     // MareSystems 관련 에러

    tecINVALID_TYPE,
    tecINVALID_NAME,              // tecINVALID_VARIABLE_NAME, tecINVALID_FUNC_NAME 포함
    tecNO_VARIABLE,               // 변수를 미선언한 상태로 사용
    tecNEED_INIT_VARIABLE,        // 초기화하지 않은 변수 사용
    tecNEED_INITIALIZE,           // 초기화하지 않은 배열 타입 사용
    tecINVALID_VARIABLE_TYPE,     // 변수를 정의한 type과 데입한 값의 타입이 불일치
    tecEXCEED_ARRAY_LENGTH,    
    tecBAD_MULTI_DIMENTION_ARRAY,

    tecNO_FUNC,                   // 함수를 미선언한 상태로 사용
    tecINVALID_FUNC_ARGUMENT,     // 함수 인수 관련 에러 
    tecINVALID_FUNC_RETURN,       // 함수 반환값 관련 에러
    
    //tecINCORRECT_TOKEN,           // parsing중, 키워드 위치 에러
    tecINCORRECT_SYNTAX,          // codeset 간의 관계 (for 다음에는 '('가 있어야됨)
    tecINCORRECT_BLACKET,
    tecINCORRECT_TYPE,            // int, double, etc. (what??)
    tecINCORRECT_EXPRESSION,      // 식 계산 에러 (계산 식의 구성이 불완전한 경우, MareExecuter)
    tecINCORRECT_ADDRESS_FORMAT, 
    tecINCORRECT_AMOUNT_FORMAT,
    tecINCORRECT_TIME_FORMAT,
    tecINCORRECT_LITERAL_FORMAT,
    tecINCORRECT_OBJECTID_FORMAT,
    
    tecDIVIDE_ZERO,               // 0으로 나누려는 경우
    tecINVALID_ASSIGN,            // 값을 변수에 할당 할때, type 불일치 등로 발생하는 에러 
    tecINVALID_CALC_TYPE,         // 계산 에러 (VarObj 에서 연산시, 계산할 수 없는 타입끼리 연산시)
    tecINVALID_VALUE,
    tecINVALID_AMOUNT,

    /* 변수, 리터널의 형식(타입) */
    tecNEED_UNSIGNED_INTEGER,     // array의 index 
    tecNEED_ARRAY_TYPE,           // what?
    tecNEED_NUMBER_TYPE,
    tecNEED_VARIABLE_TYPE,
    tecNEED_LITERAL_TYPE,         // 함수 선언 중 인자 정의시, 인자 초기값 할당관련...
    
    tecNEED_LINE_END,
    tecNEED_OPEN_BLOCK,
    tecNEED_CLOSE_BLOCK,
    tecNEED_CLOSE_LITERAL,

    tecSRC_IS_DST,
    tecNOT_DEFINED_ERROR,
};

static std::map<int, std::string> setVector() {
    std::map<int, std::string> str;

    str.insert(std::make_pair(tecCONTRACT_SYNTAX_ERROR, "tecSYNTAX_ERROR"));
    str.insert(std::make_pair(tecCONTRACT_PREPROCESS_ERROR, "tecPREPROCESS_ERROR"));
    str.insert(std::make_pair(tecCONTRACT_EXECUTE_ERROR, "tecEXECUTE_ERROR"));
    str.insert(std::make_pair(tecCONTRACT_REQUIRE_FAIL, "tecREQUIRE_FAIL"));
    str.insert(std::make_pair(tecUNKNOWN_CONTRACT_ERROR, "tecUNKNOWN_ERROR"));

    str.insert(std::make_pair(tecEXCEED_CODE_LENGTH, "tecEXCEED_CODE_LENGTH"));
    str.insert(std::make_pair(tecEXCEED_CODE_LINES, "tecEXCEED_CODE_LINES"));
    str.insert(std::make_pair(tecBAD_ALLOCATE_MEMORY, "tecBAD_ALLOCATE_MEMORY"));
    str.insert(std::make_pair(tecSTACK_UNDERFLOW, "tecSTACK_UNDERFLOW"));
    str.insert(std::make_pair(tecINVALID_SYSTEM_METHOD, "tecINVALID_SYSTEM_METHOD"));

    str.insert(std::make_pair(tecINVALID_TYPE, "tecINVALID_TYPE"));
    str.insert(std::make_pair(tecINVALID_NAME, "tecINVALID_NAME"));
    str.insert(std::make_pair(tecNO_VARIABLE, "tecNO_VARIABLE"));

    str.insert(std::make_pair(tecNEED_INIT_VARIABLE, "tecNEED_INIT_VARIABLE"));
    str.insert(std::make_pair(tecNEED_INITIALIZE, "tecNEED_INITIALIZE"));
    str.insert(std::make_pair(tecINVALID_VARIABLE_TYPE, "tecINVALID_VARIABLE_TYPE"));
    str.insert(std::make_pair(tecEXCEED_ARRAY_LENGTH, "tecEXCEED_ARRAY_LENGTH"));
    //str.insert(std::make_pair(tecBAD_MULTI_DIMENTION_ARRAY, "tecBAD_MULTI_DIMENTION_ARRAY"));

    str.insert(std::make_pair(tecNO_FUNC, "tecNO_FUNC"));
    str.insert(std::make_pair(tecINVALID_FUNC_ARGUMENT, "tecINVALID_FUNC_ARGUMENT"));
    str.insert(std::make_pair(tecINVALID_FUNC_RETURN, "tecINVALID_FUNC_RETURN"));

    str.insert(std::make_pair(tecINCORRECT_SYNTAX, "tecINCORRECT_SYNTAX"));
    str.insert(std::make_pair(tecINCORRECT_BLACKET, "tecINCORRECT_BLACKET"));
    str.insert(std::make_pair(tecINCORRECT_TYPE, "tecINCORRECT_TYPE"));

    str.insert(std::make_pair(tecINCORRECT_EXPRESSION, "tecINCORRECT_EXPRESSION"));
    str.insert(std::make_pair(tecINCORRECT_ADDRESS_FORMAT, "tecINCORRECT_ADDRESS_FORMAT"));
    str.insert(std::make_pair(tecINCORRECT_AMOUNT_FORMAT, "tecINCORRECT_AMOUNT_FORMAT"));
    str.insert(std::make_pair(tecINCORRECT_TIME_FORMAT, "tecINCORRECT_TIME_FORMAT"));
    str.insert(std::make_pair(tecINCORRECT_LITERAL_FORMAT, "tecINCORRECT_LITERAL_FORMAT"));
    str.insert(std::make_pair(tecINCORRECT_OBJECTID_FORMAT, "tecINCORRECT_OBJECTID_FORMAT"));
    
    str.insert(std::make_pair(tecDIVIDE_ZERO, "tecDIVIDE_ZERO"));
    str.insert(std::make_pair(tecINVALID_ASSIGN, "tecINVALID_ASSIGN"));
    str.insert(std::make_pair(tecINVALID_CALC_TYPE, "tecINVALID_CALC_TYPE"));
    str.insert(std::make_pair(tecINVALID_VALUE, "tecINVALID_VALUE"));
    str.insert(std::make_pair(tecINVALID_AMOUNT, "tecINVALID_AMOUNT"));

    str.insert(std::make_pair(tecNEED_UNSIGNED_INTEGER, "tecNEED_UNSIGNED_INTEGER"));
    str.insert(std::make_pair(tecNEED_ARRAY_TYPE, "tecNEED_ARRAY_TYPE"));
    str.insert(std::make_pair(tecNEED_NUMBER_TYPE, "tecNEED_NUMBER_TYPE"));
    str.insert(std::make_pair(tecNEED_VARIABLE_TYPE, "tecNEED_VARIABLE_TYPE"));
    str.insert(std::make_pair(tecNEED_LITERAL_TYPE, "tecNEED_LITERAL_TYPE"));
    
    str.insert(std::make_pair(tecNEED_LINE_END, "tecNEED_LINE_END"));
    str.insert(std::make_pair(tecNEED_OPEN_BLOCK, "tecNEED_OPEN_BLOCK"));
    str.insert(std::make_pair(tecNEED_CLOSE_BLOCK, "tecNEED_CLOSE_BLOCK"));
    str.insert(std::make_pair(tecNEED_CLOSE_LITERAL, "tecNEED_CLOSE_LITERAL"));

    str.insert(std::make_pair(tecSRC_IS_DST, "tecSRC_IS_DST"));
    str.insert(std::make_pair(tecNOT_DEFINED_ERROR, "tecNOT_DEFINED_ERROR"));

    return str;
}

static std::map<int, std::string> TECcodesStr = setVector();

static std::string transToken(int errCode) {

    auto m_iter = TECcodesStr.find((int)errCode);
    if (m_iter != TECcodesStr.end())
        return m_iter->second;
    
    return "tecUSER_DEFINED_ERROR";
}

static TECcodes transCode(std::string errCode) {
    std::map<int, std::string>::iterator iter;
    for (iter = TECcodesStr.begin(); iter != TECcodesStr.end(); ++iter){
        if ((*iter).second == errCode){
            return (TECcodes)(*iter).first;
        }
    }
    throw tecNOT_DEFINED_ERROR;
}

}

#endif
