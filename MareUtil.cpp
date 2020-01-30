
#include <sstream>
#include "MareUtil.h"

namespace mare_vm {

map<string, short> MareUtil::subCmdlist = initSubCmdList();
map<short, string> MareUtil::subCmdDBG = initSubCmdDBG();

map<string, short> 
MareUtil::initSubCmdList() 
{
    map<string, short> itemlist_;
    // ----- math function -----
    itemlist_.insert(make_pair("ceil", Ceil));
    itemlist_.insert(make_pair("floor", Floor));
    itemlist_.insert(make_pair("abs", Abs));
    itemlist_.insert(make_pair("pow", Pow));
    itemlist_.insert(make_pair("sqrt", Sqrt));
    itemlist_.insert(make_pair("round", Round));

    // ----- Get Property -----
    itemlist_.insert(make_pair("now", Now));
    itemlist_.insert(make_pair("today", Today));

    itemlist_.insert(make_pair("size", Size));
    itemlist_.insert(make_pair("length", Size));

    itemlist_.insert(make_pair("toString", ToString));

    itemlist_.insert(make_pair("find", Find));

    // ----- Set Property -----
    itemlist_.insert(make_pair("resize", Resize));

    itemlist_.insert(make_pair("push", Push));
    itemlist_.insert(make_pair("pop", Pop));

    return itemlist_;
}

map<short, string> 
MareUtil::initSubCmdDBG() 
{
    map<short, string> itemlist_;
    map<string, short>::iterator iter;
    for (iter = subCmdlist.begin(); iter != subCmdlist.end(); ++iter){
        itemlist_.insert(make_pair((*iter).second, (*iter).first));
    }
    return itemlist_;
}

short 
MareUtil::getIdx(TknKind const& tk, string const& itemName)
{
    auto m_iter = subCmdlist.find(itemName);
    if (m_iter != subCmdlist.end()){
        short k = m_iter->second;
        switch (tk) {
            case System:
                if (k == Now || k == Today) return k;
                break;
            case GetProperty:
                if (k >= Size && k <= Find) return k;
                break;
            case SetProperty:
                if (k >= Resize && k <= Pop) return k;
                break;
            case Math:
                if (k >= Ceil && k <= Round) return k;
                break;
        }
    }
    /* Not found or mismatched */
    throw tecINVALID_SYSTEM_METHOD;
}

short 
MareUtil::getPropertyIdx(string const& itemName, TknKind& tk)
{
    auto m_iter = subCmdlist.find(itemName);
    if (m_iter != subCmdlist.end()){
        short k = m_iter->second;
        switch (k) {
            case Size:
            case ToString:
            case Find:
                tk = GetProperty;
                return k;
            case Resize:
            case Push:
            case Pop:
                tk = SetProperty;
                return k;
        }
    }
    /* Not found or mismatched */
    throw tecINVALID_SYSTEM_METHOD;
}

string 
MareUtil::getSubCmdStr(short itemType)
{
    auto m_iter = subCmdDBG.find(itemType);
    if (m_iter != subCmdDBG.end()){
        return m_iter->second;
    }
    return "unknown(" + to_string(itemType) + ")";
}

void 
MareUtil::setLedgerLog(string const& title){
    logStrs_.append("[").append(title).append("] ");
}

void 
MareUtil::appendLedgerLog(string const& msg) {
    logStrs_.append(msg);
}

/** 현재 시간 */
long 
MareUtil::getTimeNow() {
    time_t current_time;
    time(&current_time);
    current_time += timeOffset;
    return (long)current_time;
}

/** (UTC) 오늘 0시 기준 시간 */
long 
MareUtil::getTodayTime() {
    std::time_t today_time = std::time(NULL);
    std::tm tmToday;
    tmToday.tm_year = gmtime(&today_time)->tm_year;
    tmToday.tm_mon = gmtime(&today_time)->tm_mon;
    tmToday.tm_mday = gmtime(&today_time)->tm_mday;
    tmToday.tm_hour = 0;
    tmToday.tm_min = 0;
    tmToday.tm_sec = 0;
    time_t current_time = mktime(&tmToday);
    current_time += timeOffset;

    return (long)current_time;
}

/** trim (copying) */
std::string 
MareUtil::trimCopy(std::string s) {
    // left trim
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        std::not1(std::ptr_fun<int, int>(std::isspace))));
    // right trim
    s.erase(std::find_if(s.rbegin(), s.rend(),
        std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

void 
MareUtil::isNumberStr(std::string const& str) {
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

void 
MareUtil::isIntStr(std::string const& str) {

    if (!std::regex_match(str, INT_TYPE)){
        std::cout << std::endl << " int text:" << str;
        throw tecINVALID_VALUE;
    }
}

void 
MareUtil::isUnsignedIntStr(std::string const& str) {

    if (!std::regex_match(str, UNSIGNED_INT_TYPE)){
        std::cout << std::endl << " datetime text:" << str;
        throw tecINVALID_VALUE;
    }
}

}
