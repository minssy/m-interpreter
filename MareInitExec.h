
#ifndef MARE_INITEXECUTE_H_INCLUDED
#define MARE_INITEXECUTE_H_INCLUDED

#include "MareExecuter.h"

namespace mare_vm {

using namespace std;


class MareInitExec : public MareExecuter {

public:

    MareInitExec(MareUtil &util) : MareExecuter(-1, util) {}

    void verifySyntax();
    void updateCode();

protected:

    void chkSyntax();

private:
    void assignVariable(bool strict=false);
    int  getMemAdrs(CodeSet const& cd, bool& isDataObj );  // override function

    VarObj getExpression_syntax(short kind1=0, short kind2=0);
    void   expression_syntax(short kind1, short kind2);
    void   term_syntax(short n);
    void   factor_syntax();

    void  callFunction_syntax(int fncIdx, bool needReturn);
    void  execSysFunc_syntax(bool needReturn);
    short setParams_syntax(vector<VarObj>& vs, short numOfParams);

    VarObj getInitVar(DtType dt) {
        VarObj obj; 
        obj.init(dt);
        if (dt == INT_T || dt == DBL_T) obj.set(1.0);
        else if (dt == DATETIME_T) obj.set(10);
        else obj.set("ABC");
        return obj;
    }
};

}

#endif