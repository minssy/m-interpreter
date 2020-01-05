
#include <sstream>
#include "MareUtil.h"

namespace mare_vm {


map<string, short> MareUtil::subCmdlist = initSubCmdList();
map<short, string> MareUtil::subCmdDBG = initSubCmdDBG();

map<string, short> 
MareUtil::initSubCmdList() 
{
    map<string, short> itemlist_;

    itemlist_.insert(make_pair("now", 1));
    itemlist_.insert(make_pair("today", 2));

    itemlist_.insert(make_pair("ceil", Ceil));
    itemlist_.insert(make_pair("floor", Floor));
    itemlist_.insert(make_pair("abs", Abs));
    itemlist_.insert(make_pair("pow", Pow));
    itemlist_.insert(make_pair("sqrt", Sqrt));
    itemlist_.insert(make_pair("round", Round));

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
                if (k == 1 || k == 2) return k;
                break;
            case Math:
                if (k >= Ceil && k <= Round) return k;
                break;
        }
    }

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

}
