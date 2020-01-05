
#ifndef MARE_CONVERTER_H_INCLUDED
#define MARE_CONVERTER_H_INCLUDED

#include "MareSet.h"
#include "MareMemory.h"
#include "VarObj.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Blob.h"

namespace mare_vm {

using namespace std;

static inline vector<string> split(string const& str, char const delimiter) {

    vector<string> internal;
    stringstream ss(str);
    string temp; 
    while (getline(ss, temp, delimiter)) {
        internal.push_back(temp);
    } 
    return internal;
}

/** Converting */
static inline vector<string> fromBlob(Blob const& in_){

    std::string str_array( in_.begin(), in_.end() );
    return split(str_array, '\n');
}

/** Converting from sfInterCode to intenalCode */
static bool fromBlob2InterCode(Blob const& in_, vector<char*>& internal) {
    char* buf_p;
    char* buf_no_p;
    int line_size, dataIndex, lineIdx;     
    internal.clear();

    buf_no_p = new char[2];
    buf_no_p[0] = 0x01; buf_no_p[1] = '\0'; 

    line_size = lineIdx = 0;
    size_t length = in_.size();
    for(size_t strIndex = 0; strIndex < length; dataIndex++) {
        unsigned char tmpValue = in_[strIndex++];
        if (line_size == 0){
            dataIndex = 0;

            if (tmpValue == 0) throw tecCONTRACT_PREPROCESS_ERROR;
            if (tmpValue == 1){
                internal.push_back(buf_no_p);
                strIndex++;  
                continue;
            }
            line_size = tmpValue;
            buf_p = new char[line_size+1];            
            buf_p[dataIndex] = tmpValue;
        }
        else if (line_size == 1){
            buf_p[dataIndex] = '\0';      // char array에 end 추가
            internal.push_back(buf_p);
            line_size--;
        }
        else {
            buf_p[dataIndex] = tmpValue;
            line_size--;
        } 
    }
    return true;
}

/** Converting from sfSymTbls to Gtable, Ltable */
static bool fromBlob2SymTbl(Blob const& in_, vector<SymTbl>& global, vector<SymTbl>& local) {
    
    global.clear(); local.clear();
    vector<string> jStrs = fromBlob(in_);
    int max = jStrs.size();
    int cnt = 0;
    SymTbl tmpTb;
    while (cnt < max) {
        string strtype = jStrs[cnt++];
        if (strtype.length() < 2) throw tecCONTRACT_PREPROCESS_ERROR;
        unsigned char typek = strtype.at(0);
        int nums = atoi(strtype.substr(2).c_str());
        for(int i=0; i<nums; i++){
            string str = jStrs[cnt++];
            // check str length
            vector<string> params = split(str, ' ');
            tmpTb.clear();
            tmpTb.symKind = (SymKind)atoi(params[0].c_str());
            tmpTb.dtTyp  = (DtType)atoi(params[1].c_str());
            tmpTb.adrs = atoi(params[2].c_str());
            tmpTb.aryLen = atoi(params[3].c_str());            
            if (tmpTb.symKind == funcId) {
                tmpTb.args = atoi(params[4].c_str());
                tmpTb.frame = atoi(params[5].c_str());
                tmpTb.name = params[6];
            }
            else {
                tmpTb.name = params[4];
            }

            if (typek == Gvar)     
                global.push_back(tmpTb);
            else if (typek == Lvar)
                local.push_back(tmpTb);
            else {
                throw tecCONTRACT_PREPROCESS_ERROR;
            }
        }
    }    
    return false;
}

static inline VarObj toVarObj(string& valStr) {

    DtType cc = (DtType)(valStr.at(0) - 48);  // char to int

    if (cc == STR_T) {
        if (valStr.length() == 2) {
            VarObj obj; obj.init(STR_T);
            return obj;
        }
        else {
            VarObj obj(cc, valStr.substr(2));
            return obj;
        }
    }

    vector<string> strs = split(valStr, ' ');
    if (cc == NON_T || strs.size() == 1) {
        VarObj obj; obj.init(NON_T);
        return obj;
    }
    else if (cc == INT_T || cc == DBL_T || cc == DATETIME_T){
        double dbl = atof(strs[1].c_str());
        VarObj obj(cc, dbl);
        return obj;
    }

    VarObj obj(cc, strs[1]);
    return obj;
}

static inline void setMemory(int idx, string& valStr, MareMemory& mem) {

    if (mem.size() <= idx)
        mem.autoResize(idx);

    mem.set(idx, toVarObj(valStr));
}

/** Converting from sfStorages to DynamicMem */
static bool fromBlob2Memory(Blob const& private_, vector<SymTbl> const& global, MareMemory& mem) {

    vector<string> jStrsPri = fromBlob(private_);
    int max = jStrsPri.size();
    int sz = global.size();
    mem.resize(max);
    string valStr;
    int m = 0;
    int n = 0;
    for (int k=0; k<sz; k++) {
        if (global[k].symKind == varId) {
            if (global[k].aryLen > 1) {
                for (int j=0; j<global[k].aryLen; j++){
                    valStr = jStrsPri[n++];
                    
                    setMemory(global[k].adrs + j, valStr, mem);
                }
            }
            else {
                valStr = jStrsPri[n++];
                
                setMemory(global[k].adrs, valStr, mem);
            }
        }
    }
    return true;
}

/** Converting from sfLiterals to nbrLITERAL, strLITERAL */
static bool fromBlob2Literal(Blob const& in_, vector<double>& dbls_, vector<string>& strs_) {

    dbls_.clear(); strs_.clear();
    vector<string> jStrs = fromBlob(in_);
    int max = jStrs.size();
    int cnt = 0;
    while (cnt < max) {
        string str = jStrs[cnt++];
        unsigned char typek = str.at(0);
        int nums = atoi(str.substr(2).c_str());

        for(int i=0; i<nums; i++){
            if (typek == VarStr)
                strs_.push_back(jStrs[cnt++]);
            else// if (typek == VarDbl)
                dbls_.push_back(atof(jStrs[cnt++].c_str()));
            //else throw "error:from Blob to Literal";
        }
    }
    return true;
}

/*** To Blob ***/

static inline void appendBlob(string const& in_, Blob& obj) {
    for(unsigned char cc : in_){
        obj.push_back(cc);
    }
    obj.push_back('\n');
}

static inline void appendBlob(TknKind const& type_, int const& nums_, Blob& obj) {
    obj.push_back(type_);
    obj.push_back(' ');
    appendBlob(to_string(nums_), obj);
}

/** Converting from intenalCode to sfInterCode */
static Blob toBlob(vector<char*> const& internalCode_){

    Blob obj;
    int max = internalCode_.size();
    for (int i=0; i<max; i++){

        char* cp = internalCode_[i];        
        unsigned short cnt = cp[0];
        for(int idx=0; idx<cnt; idx++){
            unsigned char code = cp[idx]; 

            obj.push_back(code);
        }
        obj.push_back('\n');
    }
    return obj;
}

/** Converting from DynamicMem, Gtable to sfStorages */
static Blob toBlob(MareMemory& in_, vector<SymTbl> const& global) {

    Blob outPri;
    outPri.clear();
    int max = global.size();
    for (int i=0; i<max; i++){
        SymTbl sym = global[i];
        if (sym.symKind != funcId) {
            if (sym.aryLen > 1) { // 배열인 경우, 배열크기만큼 추가
                for (short k=0; k<sym.aryLen;k++){
                    appendBlob(in_.get(sym.adrs + k).toFullString(), outPri);
                }
            }
            else {
                appendBlob(in_.get(sym.adrs).toFullString(), outPri);
            }
        }
    }
    return outPri;
}

/** Converting from Gtable, Ltable to sfSymTbls */
static Blob toBlob(vector<SymTbl> const& global, vector<SymTbl> const& local) {

    Blob obj;
    int max = global.size();
    if (max > 0) {
        appendBlob(Gvar, max, obj);
        for (int i=0; i<max; i++){
            SymTbl sym = global[i];
            appendBlob(sym.toFullString(), obj);
        }
    }
    max = local.size();
    if (max > 0) {
        appendBlob(Lvar, max, obj);
        for (int i=0; i<max; i++){
            SymTbl sym = local[i];
            appendBlob(sym.toFullString(), obj);
        }
    }
    return obj;
}

/** Converting from nbrLITERAL, strLITERAL to sfLiterals */
static Blob toBlob(vector<double> const& dbls_, vector<string> const& strs_) {
    Blob obj;
    int max = dbls_.size();
    if (max > 0) {
        string objStr;
        appendBlob(VarDbl, max, obj);
        for (int i=0; i<max; i++){
            // 소숫점 정리
            objStr = to_string_with_precision(dbls_[i]);
            to_string_simple(objStr);
            appendBlob(objStr, obj);
        }
    }
    max = strs_.size();
    if (max > 0) {
        appendBlob(VarStr, max, obj);
        for (int i=0; i<max; i++){
            appendBlob(strs_[i], obj);
        }
    }
    return obj;
}

/** Convrting from strings (funcInfos, etc.) */
static Blob toBlob(vector<string> const& strs_) {
    Blob obj;
    int max = strs_.size();
    if (max > 0) {
        for (int i=0; i<max; i++){
            appendBlob(strs_[i], obj);
        }
    }
    return obj;
}

/** Convrting from string (logs, etc.) */
static Blob toBlob(string const& in_) {
    Blob obj;
    appendBlob(in_, obj);
    return obj;
}

}

#endif