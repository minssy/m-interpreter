#ifndef MARE_UTIL_H_INCLUDED
#define MARE_UTIL_H_INCLUDED

#include <sstream>
#include <string>
#include <vector>
#include "MareSet.h"
#include "MareMemory.h"
#include "Blob.h"
#include "Log.h"
#include "VarObj.h"
#include "MareConverter.h"

namespace mare_vm {

static const std::regex INT_TYPE("[+-]?[0-9]+");
static const std::regex DOUBLE_TYPE("[+-]?[0-9]+[.]?[0-9]+");
static const std::regex UNSIGNED_INT_TYPE("[+]?[0-9]+");
static const std::regex NUM_ENG_TYPE("[a-zA-Z0-9]+");

using namespace std;

enum MathAPI {
    Ceil = 21,
    Floor,
    Abs,
    Pow,
    Sqrt,
    Round
};

enum VariableGetAPI {

    /* system */
    Now = 31,
    Today,
    /* common (string, array, vector) */
    Size = 41,
    /* int, double */
    ToString,
    /* string */
    // ToInt,
    // ToDbl,

    /* array, vector */
    IndexOf = 51, // find

};

enum VariableSetAPI {

    /* vector */
    Resize = 61,
    Reserve,
    Push,
    Pop,
    Insert,
    Erase,
    Clear,

};

class MareUtil {

public:

    log j_;

    MareUtil(long timeOffset_) : j_(), timeOffset(timeOffset_), logStrs_("log\n") { }

    void setLedgerLog(string const& title);
    void appendLedgerLog(string const& msg);
    string getLedgerLog() { return logStrs_; }


    /** 시스템 변수 값 가져오기 */
    VarObj getObj(short const type) {
        switch (type)
        {
        case Now: return VarObj(DATETIME_T, getTimeNow());
        case Today: return VarObj(DATETIME_T, getTodayTime());
        
        default:
            throw tecINVALID_SYSTEM_METHOD;
        }

    }

    /** 시스템 변수여부 확인 및 코드에 저장될 값 반환 */
    static short getIdx(TknKind const& tk, string const& itemName);
    static short getPropertyIdx(string const& itemName, TknKind& tk);
    /** 디버깅용 */
    static string getSubCmdStr(short itemType);

    /** Utils */
    static std::string trimCopy(std::string s);
    static void isNumberStr(std::string const& str);
    static void isIntStr(std::string const& str);
    static void isUnsignedIntStr(std::string const& str);

private:
    long timeOffset;
    string logStrs_;

    long getTimeNow();
    long getTodayTime();

    static map<string, short> subCmdlist;
    static map<short, string> subCmdDBG;
    static map<string, short> initSubCmdList();
    static map<short, string> initSubCmdDBG();

};

}

#endif
