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

using namespace std;

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
        case 1: return VarObj(DATETIME_T, getTimeNow());
        case 2: return VarObj(DATETIME_T, getTodayTime());
        
        default:
            throw tecINVALID_SYSTEM_METHOD;
        }

    }

    /** 시스템 변수여부 확인 및 코드에 저장될 값 반환 */
    static short getIdx(TknKind const& tk, string const& itemName);

    static string getSubCmdStr(short itemType);

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
