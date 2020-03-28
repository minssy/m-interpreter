#ifndef MARE_VAROBJ_H_INCLUDED
#define MARE_VAROBJ_H_INCLUDED

#include "MareSet.h"

namespace mare_vm {

using namespace std;

#ifdef __APPLE__
    #define __INT64_C(c)	c ## L
#endif
#define INT48_MIN		(-__INT64_C(140737488355327)-1)
#define INT48_MAX		(__INT64_C(140737488355327))

static inline double Rounding( long double x )
{
    return static_cast<double>( floor( (x) * CAL_DECIMAL_POINTS + 0.5 ) / CAL_DECIMAL_POINTS );
}

static inline void chkINT64(double num) {
    if (num > INT48_MAX || num < INT48_MIN) throw tecINVALID_VALUE;
}

static inline void chkDateTimeValue(double num) {
    if (num > INT32_MAX || num < 0) throw tecINVALID_VALUE;
}

/** 객체가 숫자형 타입(int, double)인지 확인 */
static inline bool isNumericType(DtType _type) {
    if (_type == INT_T || _type == DBL_T)
        return true;
    return false;
}

/** precision (소숫점 조정) */
static string to_string_with_precision(double const a_value, int n = LEN_DECIMAL_POINTS) {

    if (n == 6) return to_string(a_value);
    
    string svalue = to_string(a_value);
    int m = svalue.length() + (n - 7);  // 소숫점 지정된 자릿수로 나오도록 설정 (n = 7)
    //int m = svalue.length();            // 소수점 7자리로 나오도록
    if (m > 16) { m = 16; }             // double형 부동소숫점 오차 때문에
    else if (a_value < 1) --m;          // 소숫점 8자리로 나오는 현상 수정

    ostringstream out;
    //out << std::ios_base::fixed << setprecision(7) << a_value; // 잘못된 값 나옴
    out << setprecision(m) << a_value;
    return out.str();
}

/** 소숫점 정리 */
static void to_string_simple(string& s_value) {
    if (s_value.find(".") != std::string::npos) {
        size_t lth = s_value.length();
        bool edit = false;
        while (s_value.at(--lth) == '0'){ edit = true; }
        if (edit) { 
            if (s_value.at(lth) == '.') --lth;
            s_value.erase(++lth);
        }
    }
}

/** 변수 계산 등의 처리를 위한 메모리 object 정의 */
class VarObj
{
private:
    DtType type;
    double val;
    string txt;

    /** 
     * type이 설정된 경우에는 type의 값에 따라, update 가능 여부 확인 후, 값 갱신 작업을 수행
     * type 설정이 안된 경우에는 type까지 적용
     */
    void set(VarObj const& nobj) {
        //cout << endl << "set:" << nobj.type << "-" << nobj.txt << nobj.val;
        //cout << endl << "ori:" << type << "-" << txt << val;
        if (nobj.type == NON_T) { 
            type = NON_T; txt=""; val=0;
            return;       
        }
        if (type != nobj.type) {
            if (type == NON_T) type = nobj.type; 
            else if (isNumericType(type)) {
                if (!isNumericType(nobj.type)) {
                    throw tecINVALID_ASSIGN;
                }
                set(nobj.val);
                return;
            }
            else if (type != nobj.type) {
                throw tecINVALID_ASSIGN;
            }
        }
        val = nobj.val;
        txt = nobj.txt;
    }

public:
    /** 생성자 */
    VarObj() { type=NON_T; val=0.0; txt=""; }
    
    /** 생성자 */
    VarObj(DtType _type, double dt) {
        
        type = _type; txt="";
        set(dt);
    }

    /** 생성자 */
    VarObj(DtType _type, string const& st) {
        
        type = _type; val=0;
        set(st);
    }

    /** 생성자 */
    VarObj(DtType _type, VarObj& nobj) {
        type = _type;
        set(nobj);
    }

    /** 지정된 타입으로 초기화 (내부 값도 초기화됨) */
    void init(DtType _type) {
        type = _type;
        val=0.0; txt="";
    }

    /** 설정된 값을 업데이트 */
    void set(double dt) {

        if (type == DBL_T) { 
            chkINT64(dt);
            //val = dt;
            val = Rounding(dt); 
            return; 
        }
        if (type == INT_T) { 
            chkINT64(dt);
            val = floor(dt);
            return; 
        }
        if (type == DATETIME_T) { 
            chkDateTimeValue(dt);
            val = floor(dt);
            return;
        }
        if (type == OBJECT_T) { 
            chkDateTimeValue(dt);
            val = floor(dt);
            return;
        }
        else throw tecINVALID_ASSIGN;
    }

    /** 설정된 값을 업데이트 */
    void set(string st) {
        txt = st;
        if (type == STR_T) return;
        else throw tecINVALID_ASSIGN;
    }

    /** 객체의 타입정보 반환 */
    const DtType getType() {
        return type;
    }    

    /** 
     * 객체의 값 반환 (Numeric Value)
     *  - 결과 확인용 (True / False)
     */
    double getDbl() {
        if (!isNumericType(type))
            throw tecNEED_NUMBER_TYPE;
        return val;
    }

    int getObjAdrs() {
        if (type != OBJECT_T)
            throw tecNEED_NUMBER_TYPE;
        return (int)val;
    }

    /** 객체의 값을 문자열로 반환 */
    string toStr(bool forClient=false) {
        switch(type){
            case INT_T:
            case DATETIME_T:
            case OBJECT_T:
                return to_string((long)val);
            case DBL_T:
            {
                string newStr = to_string_with_precision(val);
                to_string_simple(newStr);
                return newStr;
            }
            default:
                return txt;
        }
    }
    
    /** 객체의 모든 정보를 문자열로 반환 (convert blob or debugging) */
    string toFullString(bool isHumanReadable=false) {
        string str = isHumanReadable ? 
            getDataTypeStr(type) : to_string((unsigned short)type);
        str.append(" ").append(toStr(isHumanReadable));
        return str;
    }

    VarObj& operator=(VarObj const &ref) {
        set(ref);
        return *this;
    }

    VarObj operator+(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T)
                    return VarObj(DBL_T, val + ref.val);
                else if (type == DATETIME_T)
                    return VarObj(DATETIME_T, val + ref.val);

                throw tecINVALID_CALC_TYPE;

            case STR_T:
                if (type == STR_T) 
                    return VarObj(STR_T, txt + ref.txt);

                throw tecINVALID_CALC_TYPE;

            case DATETIME_T:
                if (type == INT_T || type == DBL_T)
                    return VarObj(DATETIME_T, val + ref.val);
                
            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator-(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T){
                    return VarObj(DBL_T, val - ref.val);
                }
                else if (type == DATETIME_T){
                    return VarObj(DATETIME_T, val - ref.val);
                }
                throw tecINVALID_CALC_TYPE;

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator*(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T){
                    return VarObj(DBL_T, val * ref.val);
                }
                throw tecINVALID_CALC_TYPE;

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator/(VarObj const &ref) {

        if (ref.val == 0) throw tecDIVIDE_ZERO;

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T){
                    return VarObj(DBL_T, val/ref.val);
                }
                throw tecINVALID_CALC_TYPE;

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator%(VarObj const &ref) {

        if (((long)ref.val) == 0) throw tecDIVIDE_ZERO;

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T){
                    return VarObj(INT_T, (long)val % (long)ref.val);
                }

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj& operator++() {

        switch(type){
            case INT_T:
            case DBL_T:
                val += 1;
                return *this;

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    // VarObj const operator++(int) {
    //     switch(type){
    //         case INT_T: case DBL_T:
    //         {
    //             VarObj const obj(type, val);
    //             val += 1; return obj;
    //         }
    //         default:
    //             throw "error: mismatch type (++)";
    //     }
    // }

    VarObj& operator--() {

        switch(type){
            case INT_T:
            case DBL_T:
                val -= 1;
                return *this;

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    // VarObj const operator--(int) {
    //     switch(type){
    //         case INT_T: case DBL_T:
    //         {
    //             VarObj const obj(type, val);
    //             val -= 1; return obj;
    //         }
    //         default:
    //             throw "error: mismatch type (++)";
    //     }
    // }

    VarObj& operator+=(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T)
                    val = floor(val + ref.val);
                else if (type == DBL_T)
                    val += ref.val;
                else if (type == DATETIME_T)
                    val = floor(val + ref.val);
                else 
                    throw tecINVALID_CALC_TYPE;
                
                chkINT64(val);
                return *this;

            case STR_T:
                if (type == STR_T) {
                    txt += ref.txt;
                    return *this;
                }

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj& operator-=(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T)
                    val = floor(val - ref.val);
                else if (type == DBL_T)
                    val -= ref.val;
                else if (type == DATETIME_T)
                    val = floor(val - ref.val);
                else 
                    throw tecINVALID_CALC_TYPE;

                chkINT64(val);
                return *this;

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj& operator*=(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T)
                    val = floor(val * ref.val);
                else if (type == DBL_T)
                    val *= ref.val;
                else 
                    throw tecINVALID_CALC_TYPE;
                
                chkINT64(val);
                return *this;

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj& operator/=(VarObj const &ref) {

        if (ref.val == 0) throw tecDIVIDE_ZERO;

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T)
                    val = floor(val / ref.val);
                else if (type == DBL_T)
                    val /= ref.val;
                else 
                    throw tecINVALID_CALC_TYPE;

                chkINT64(val);
                return *this;

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator<(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T)
                    return VarObj(DBL_T, val < ref.val);
                
                throw tecINVALID_CALC_TYPE;

            case DATETIME_T:
                if (type == DATETIME_T) 
                    return VarObj(DBL_T, val < ref.val);

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator<=(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T)
                    return VarObj(DBL_T, val <= ref.val);
                
                throw tecINVALID_CALC_TYPE;

            case DATETIME_T:
                if (type == DATETIME_T) 
                    return VarObj(DBL_T, val <= ref.val);

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator>(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T)
                    return VarObj(DBL_T, val > ref.val);

                throw tecINVALID_CALC_TYPE;

            case DATETIME_T:
                if (type == DATETIME_T) 
                    return VarObj(DBL_T, val > ref.val);

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator>=(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T)
                    return VarObj(DBL_T, val >= ref.val);
                
                throw tecINVALID_CALC_TYPE;

            case DATETIME_T:
                if (type == DATETIME_T) 
                    return VarObj(DBL_T, val >= ref.val);

            default:
                throw tecINVALID_CALC_TYPE;
        }
    }


    VarObj operator==(VarObj const &ref) {

        if (type != ref.type) {
            if ((type != INT_T && type != DBL_T)
                 || (ref.type != INT_T && ref.type != DBL_T) )
                throw tecINVALID_CALC_TYPE;
        }

        switch(ref.type){
            case INT_T:
            case DBL_T:
            case DATETIME_T:
                return VarObj(DBL_T, val == ref.val);

            case STR_T:
            {
                short res = txt.compare(ref.txt) == 0 ? 1 : 0;
                return VarObj(DBL_T, res);
            }
            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator!=(VarObj const &ref) {

        if (type != ref.type) {
            if ((type != INT_T && type != DBL_T)
                 || (ref.type != INT_T && ref.type != DBL_T) )
                throw tecINVALID_CALC_TYPE;
        }

        switch(ref.type){
            case INT_T:
            case DBL_T:
            case DATETIME_T:
                return VarObj(DBL_T, val != ref.val);

            case STR_T:
            {
                short res = txt.compare(ref.txt) == 0 ? 0 : 1;
                return VarObj(DBL_T, res);
            }
            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator&&(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T)
                    return VarObj(DBL_T, val && ref.val);
                
            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    VarObj operator||(VarObj const &ref) {

        switch(ref.type){
            case INT_T:
            case DBL_T:
                if (type == INT_T || type == DBL_T)
                    return VarObj(DBL_T, val || ref.val);
                
            default:
                throw tecINVALID_CALC_TYPE;
        }
    }

    /** convert type to string */
    static string getDataTypeStr(DtType _type) {
        switch(_type){
            case NON_T:
                return "NON_T";
            case INT_T:
                return "INT_T";
            case DBL_T:
                return "DBL_T";
            case STR_T:
                return "STR_T";
            case DATETIME_T:
                return "DATETIME_T";
            case OBJECT_T:
                return "OBJECT_T";
            default:
                throw tecINVALID_TYPE;
        }
    }
}; 

}

#endif