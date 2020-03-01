
#include "MareSet.h"
#include "MareInitExec.h"

namespace mare_vm {

/* 구문 검사 */
void 
MareInitExec::verifySyntax() 
{
    try {
        chkSyntax();
    }
    catch(TECcodes err_code) {
        errorExit(err_code, "check Syntax error");
    }
}

/* 구문 검사 */
void 
MareInitExec::chkSyntax() 
{
    chkSyntaxMode = true;
    readedCodeCnt = 0;
    short sz = intenalCode.size();
    DtType dt;

    for (Pc=1; Pc<sz; Pc++) {

        code = firstCode(Pc);
        blkNest = 0;
        initDbgCode(code);

        switch (code.kind) 
        {
        case EofLine: case Do: case Close: 
            break;                                            /* 이미 검사 완료됨 */
        case Else: 
        case End: 
            code = nextCode(); chkEofLine();                  /* line이 끝났는지 검사 */
            break;
        case Func: 
            code = nextCode(); code = chkNextCode(code, '(');
            if (code.kind != ')'){
                short cnt = 0;
                do {
                    addDbgCode(Expression, SET_PARAMETER_TYPE, cnt);
                    addDbgCode(code);
                    if (code.kind != Lvar) errorExit(tecINCORRECT_SYNTAX, "잘못된 함수 형식");
                    dt = symTablePt(code)->dtTyp;             /* 파라미터 타입 */
                    if (symTablePt(code)->aryLen != 0) errorExit(tecNEED_VARIABLE_TYPE, "Array not available.");
                    getTopAdrs(code);
                    code = nextCode();
                    if (code.kind == '=') {
                        addDbgCode(code);
                        returnValue.init(dt);
                        returnValue = getExpression_syntax('=', 0);
                        removeDbgCode();
                    }
                    removeDbgCode();
                    if (code.kind == ')') break;
                    else if (code.kind == ',') code = nextCode();
                    removeDbgCode(); cnt++;
                } while (true);
            }
            code = nextCode(); chkEofLine();
            break;
        case DeclareArr:
        case DeclareVar: 
            do {
                bool isArray = false;
                if (code.kind == DeclareArr) isArray = true;
                code = nextCode();
                addDbgCode(code);
                dt = symTablePt(code)->dtTyp;
                getTopAdrs(code);
                code = nextCode();
                if (code.kind == '=') {
                    if (isArray) errorExit(tecINCORRECT_SYNTAX, "배열 선언시, 초기값 할당 불가");
                    else { 
                        addDbgCode(code);
                        returnValue.init(dt);
                        returnValue = getExpression_syntax('=', 0);
                        removeDbgCode();
                    }
                }
                if (code.kind == EofLine) break;
                else if (code.kind == ',') code = nextCode(); 
                else errorExit(tecINCORRECT_SYNTAX, "Wrong declear syntax");

                if (code.kind != DeclareArr && code.kind != DeclareVar)
                    errorExit(tecINCORRECT_SYNTAX, "Need declear syntax"); 
                removeDbgCode();
            } while (true);
            break;
        case VarStruct:
            break;
        case Gvar: 
        case Lvar:                                            /* 대입문 처리 */
            assignVariable();
            chkEofLine();
            break;
        case If: case Elif: 
        case While:
            code = nextCode();
            getExpression_syntax('(', ')').getDbl();          /* 조건식 값 */
            chkEofLine();
            break;
        case Foreach:
            code = nextCode();
            code = chkNextCode(code, '(');
            //dt = symTablePt(code)->dtTyp;                     /* 임시 변수 타입 */
            getMemAdrs(code, varSymType, dt);                   /* 임시 변수 주소 */
            if (varSymType != varId) errorExit(tecNEED_VARIABLE_TYPE);
            code = chkNextCode(code, ':');
            if (symTablePt(code)->dtTyp != dt) errorExit(tecINCORRECT_TYPE);
            getMemAdrs(code, varSymType, dt);
            if (varSymType == varId) errorExit(tecNEED_ARRAY_TYPE);
            code = chkNextCode(code, ')');
            chkEofLine();
            break;
        case For:
            code = nextCode();
            code = chkNextCode(code, '(');
            addDbgCode(Expression, CONDITIONAL_TYPE, 1);
            getMemAdrs(code, varSymType, dt);                 /* 제어 변수 주소 */
            if (varSymType != varId) errorExit(tecNEED_VARIABLE_TYPE);
            getExpression_syntax('=', ';');                   /* 초깃값 */            
            changeDbgCode(Expression, CONDITIONAL_TYPE, 0);
            if (code.kind == ';') code = nextCode();          /* 증분값 없음 */
            else if (code.kind == Gvar || code.kind == Lvar) {/* 증분값 처리 */
                assignVariable(true);
                code = chkNextCode(code, ';');
            }
            else getExpression_syntax(0, ';');                /* 증분값 */
            changeDbgCode(Expression, CONDITIONAL_TYPE, 2);
            if (code.kind == ')') code = nextCode();          /* 최종값 검증 없음*/
            else getExpression_syntax(0, ')').getDbl();       /* 최종값 검증 */
            chkEofLine();
            break;
        case Break:
        case Continue:
            code = nextCode();
            if (code.kind == '?') getExpression_syntax('?', 0).getDbl(); // true or false only
            chkEofLine();
            break;
        case Return:
            code = nextCode();
            if (code.kind != '?' && code.kind != EofLine) getExpression_syntax();
            if (code.kind == '?') getExpression_syntax('?', 0).getDbl(); // true or false only
            chkEofLine();
            break;
        case Fcall:                                           /* 대입 없는 함수 호출 */
            callFunction_syntax(code.symIdx, false);
            chkEofLine();
            break;
        case Require:
            code = nextCode(); 
            getExpression_syntax('(', ')').getDbl();
            chkEofLine();
        case Throws:
            code = nextCode(); 
            chkEofLine();
            break;
        case Exit:
        case Log:
        case ToArray:
            execSysFunc_syntax(false);
            break;            
        case DBPlus:
        case DBMinus:
            code = nextCode(); 
            if (!isNumericType(symTablePt(code)->dtTyp))
                errorExit(tecNEED_NUMBER_TYPE, "Numeric Type only (++, --)");
            getMemAdrs(code, varSymType, dt);                 /* 좌변 주소 확인 */
            if (varSymType != varId) errorExit(tecNEED_VARIABLE_TYPE, "Leading operator (++, --)");
            if (dt != INT_T && dt != DBL_T) errorExit(tecNEED_NUMBER_TYPE, "Leading operator (++, --)");
            chkEofLine();                                     /* 라인의 끝인지 확인 */
            break;
        default:
            errorExit(tecINCORRECT_SYNTAX, "Not use the first of line");
        }
    }

    chkSyntaxMode = false;
}

/** 불필요한 코드를 제거함 */
void
MareInitExec::updateCode() {

    char* buf_no_p = new char[2];
    buf_no_p[0] = 0x01; buf_no_p[1] = '\0'; 

    Pc = 0;
    readedCodeCnt = 0;
    maxCodeLine = intenalCode.size() - 1;
    while (++Pc<=maxCodeLine) {
        code = firstCode(Pc);
        switch (code.kind) {
            case Func:
                Pc =  code.jmpAdrs;
            case EofLine:
            //case Do:
            //case Close:            
                break;
            default:
                intenalCode[Pc] = buf_no_p;
                break;
        }
    }
    /* 불필요한 빈 라인을 끝에서부터 제거 */
    short rIdx = maxCodeLine;
    bool hasBlankLine = false;
    if (firstCode(rIdx--).kind == EofLine) {
        hasBlankLine = true;
    }
    while (true)
    {
        if (firstCode(rIdx--).kind == EofLine) {
            intenalCode.pop_back();
        }
        else break;
    }    
}


void 
MareInitExec::assignVariable(bool strict)
{
    CodeSet save = code;
    SymKind varSymType;
    DtType dt;
    getMemAdrs(code, varSymType, dt);                        /* 좌변 주소 확인 */
    if (code.kind == '.') {
        if (strict) errorExit(tecNEED_VARIABLE_TYPE);
        return setProperty_syntax(save, varSymType);
    }
    cout << endl << " var:" << varSymType << " " << dt;
    // if (varSymType == objectId && code.kind != '=') 
    //     errorExit(tecINVALID_ASSIGN, "assign(=) only -type:", to_string((int)varSymType));
    // else if (varSymType != varId) 
    //     errorExit(tecINVALID_ASSIGN, "type:", to_string((int)varSymType));

    if (varSymType == objectId && code.kind == '=') {
        // struct가 같은지 확인...
        code = nextCode(); 
        short obj_type = symTablePt(save)->ref;
        if (obj_type != symTablePt(code)->ref)
            errorExit(tecINVALID_ASSIGN, "different struct object");
        getMemAdrs(code, varSymType, dt);
        if (varSymType != objectId) errorExit(tecINVALID_ASSIGN, "need struct object");
        if (code.kind != EofLine) errorExit(tecNEED_LINE_END, "assign struct object");
    }
    else if (varSymType == varId) {
        cout << endl << " assignType:" << kind2Str(code);
        //DtType dt = symTablePt(save)->dtTyp;  /* struct와 같은 경우 에러발생 */
        returnValue.init(dt);
        returnValue = getInitVar(dt);
        addDbgCode(code);
        if (code.kind == '=')
            returnValue = getExpression_syntax(code.kind, 0);
        else if (code.kind == PlusAssign)
            returnValue += getExpression_syntax(code.kind, 0);
        else if (code.kind == MinusAssign)
            returnValue -= getExpression_syntax(code.kind, 0);
        else if (code.kind == MultiAssign)
            returnValue *= getExpression_syntax(code.kind, 0);
        else if (code.kind == DiviAssign)
            returnValue /= getExpression_syntax(code.kind, 0);
        else if (code.kind == DBPlusR || code.kind == DBMinusR) {
            if (!isNumericType(dt)) errorExit(tecNEED_NUMBER_TYPE);
            code = nextCode();  
        }
        else {
            errorExit(tecINCORRECT_SYNTAX, "wrong assign");
        }
    }
    else {
        errorExit(tecINVALID_ASSIGN, "type:", to_string((int)varSymType));
    }
}

void
MareInitExec::setProperty_syntax(CodeSet const& varCode, SymKind sk)
{
    DtType objtp = symTablePt(varCode)->dtTyp;
    code = nextCode(); 
    addDbgCode(code);
    if (code.kind != SetProperty)
        errorExit(tecINCORRECT_SYNTAX, "set property only");
 
    if (varCode.kind != Gvar) 
        errorExit(tecINCORRECT_SYNTAX, "global variable only");

    if (objtp == OBJECT_T) 
        errorExit(tecINCORRECT_SYNTAX, "under construction for struct object!!");
        

    if (code.symIdx == Resize) {
        if (sk != arrayId) errorExit(tecINCORRECT_SYNTAX, "array only");
        code = nextCode();
        double nsz = getExpression_syntax('(', ')').getDbl();
        if (nsz < 1) errorExit(tecNEED_UNSIGNED_INTEGER);
        if (nsz > MAX_ARRAY) errorExit(tecEXCEED_ARRAY_LENGTH);
        return;
    }

    if (sk != arrListId) errorExit(tecINCORRECT_SYNTAX, "arrayList only");
    short arraySetPropertyType = code.symIdx;
    if (arraySetPropertyType == Clear) {
        
        code = nextCode();
        code = chkNextCode(code, '(');
        code = chkNextCode(code, ')');
    }
    else if (arraySetPropertyType == Remove) {

        code = nextCode();
        getExpression_syntax('(', 0).getDbl();
        if (code.kind == ',') {
            getExpression_syntax(',', 0).getDbl();
        }
        code = chkNextCode(code, ')');
    }
    else { 

        code = nextCode();
        if (objtp == OBJECT_T) {
            SymKind varSymType1;
            DtType dt1;
            getMemAdrs(code, varSymType1, dt1);
            if (varSymType1 != objectId)
                errorExit(tecINCORRECT_SYNTAX, "need object");
        }
        else {
            returnValue.init(objtp);
            returnValue = getExpression_syntax('(', 0);
        }

        if (arraySetPropertyType == Add) {
            if (code.kind == ',') {
                getExpression_syntax(',', 0).getDbl();
            }
            code = chkNextCode(code, ')');
        }
        else if (arraySetPropertyType == Insert) {
            getExpression_syntax(',', 0).getDbl();
            if (code.kind == ',') {
                getExpression_syntax(',', 0).getDbl();
            }
            code = chkNextCode(code, ')');
        }
        else errorExit(tecINCORRECT_SYNTAX, "undifined property type");
    }
}

/** 
 * 단순 변수 또는 배열 요소의 주소를 반환. 
 * 배열인지 확인하기 위해 내부적으로 nextCode()를 호출함 */
int 
MareInitExec::getMemAdrs(CodeSet const& cd, SymKind& objType, DtType& varTp)
{
    int adr=0;
    short symTbIdx = cd.symIdx;
    adr = getTopAdrs(cd);
    auto varSymPt = symTablePt(cd);
    objType = varSymPt->symKind;
    varTp = varSymPt->dtTyp;
    if (objType == paraId) objType = varId;
    cout << endl << "getMemAdrs:" << varSymPt->name << " " << objType;
    code = nextCode();
    if (objType == varId) return adr;       /* 변수가 배열이 아닌 경우 */

    if (code.kind == '[') {
        if (!(objType == arrayId || objType == arrListId))
            errorExit(tecINVALID_VARIABLE_TYPE, "not array");
        
        double d;
        d = getExpression_syntax('[', ']').getDbl(); 
        if ((int)d != d) errorExit(tecNEED_UNSIGNED_INTEGER, "The index of array must be a positive integer.");

        if (varSymPt->dtTyp == OBJECT_T) {
            objType = objectId;
            // symTbIdx 업데이트??
            symTbIdx = varSymPt-> ref;
        }
        else objType = varId;               /* 배열등의 요소임 */
    }

    if (code.kind == StructItem) {
        if (objType != objectId) errorExit(tecINCORRECT_SYNTAX, "wrong StructItem");
        objType = varId;                    /* Struct의 요소임 */
        bool finded = false;
        for(ItemTbl it : Itable) {
            cout << endl << it.toFullString(true);
            if (it.symId == symTbIdx && it.offset == code.symIdx) {
                varTp = it.dtTyp;
                finded = true;
                break;
            }
        }
        if (!finded) errorExit(tecINVALID_NAME, "why not found?");

        code = nextCode();
    }
    return adr;                             /* 구문 검사인 경우 */

    // if (code.kind != '[') {                       /* 배열의 첨자가 없을 경우(첫항목을 임시로) */
    //      return adr;
    // }
    // objType = varId;                              /* 배열등의 요소임 */
    // double d;
    // d = getExpression_syntax('[', ']').getDbl(); 
    // if ((int)d != d) errorExit(tecNEED_UNSIGNED_INTEGER, "The index of array must be a positive integer.");
    // return adr;               /* 구문 검사인 경우 */
}

/** 
 * expression 계산 후, 결과를 반환
 * -kind1이 0이 아니면, kind1으로 시작하는지 확인 
 * -kind2가 0이 아니면, 조건식이 끝난 다음에 kind2가 나오는지 확인 
 */
VarObj 
MareInitExec::getExpression_syntax(short kind1, short kind2)
{
    expression_syntax(kind1, kind2); 
    return mstk.pop();
}

/** 
 * 조건부 일반식처리 
 * -kind1이 0이 아니면, kind1으로 시작하는지 확인 
 * -kind2가 0이 아니면, 조건식이 끝난 다음에 kind2가 나오는지 확인 
 */
void 
MareInitExec::expression_syntax(short kind1, short kind2) 
{
    if (kind1 != 0) code = chkNextCode(code, kind1);   /* 코드 확인 */
    addDbgCode(Expression, NORMAL_TYPE, ++countExps);  /* Expression이 시작함 */
    term_syntax(1);                                    /* 이항연산 모듈 처리 */
    removeDbgCode();                                   /* Expression이 종료함 */
    if (kind2 != 0) code = chkNextCode(code, kind2);   /* 코드 확인 */
}

/** n은 우선 순위 */
void 
MareInitExec::term_syntax(short n) 
{
    TknKind op;
    if (n == 8) {
        factor_syntax();
        return; 
    }
    term_syntax(n+1);
    while (n == opOrder(code.kind)) {         /* 우선 순위가 같은 연산자가 연속되도록 */
        op = code.kind;
        code = nextCode(); 
        term_syntax(n+1);

        binaryExpr(op);
    }
}

/** 식의 인자 처리(문법 검사) */
void 
MareInitExec::factor_syntax() 
{
    TknKind kd = code.kind;    
    JLOG(mutil.j_.trace()) << " *** factor:'" << kind2Str(kd) << "':" << kind2Str(code);
    addDbgCode(code);
    
    switch (kd) {
    case Not: case Minus: case Plus:
        code = nextCode(); factor_syntax();
        removeDbgCode();
        mstk.pop(); mstk.push(DBL_T, 1.0);
        break;
    case DBPlus: case DBMinus:
        code = nextCode(); factor_syntax();
        break;
    case Lparen:
        expression_syntax('(', ')');
        break;
    case True:
        mstk.push(DBL_T, 1); code = nextCode();
        break;
    case False:
        mstk.push(DBL_T, 0); code = nextCode();
        break;
    case IntNum: 
        mstk.push(INT_T, code.numVal); code = nextCode();
        break;
    case DblNum:
        mstk.push(DBL_T, code.numVal); code = nextCode();
        break;
    case String:
        mstk.push(STR_T, code.text); code = nextCode();
        break;
    case Gvar: case Lvar:
        {
        //DtType tmpTp = symTablePt(code)->dtTyp;
        DtType tmpTp;
        SymKind varSymTypeTmp;
        getMemAdrs(code, varSymTypeTmp, tmpTp); 
        if (code.kind == '.') {
            code = nextCode();
            if (code.kind != GetProperty)
                errorExit(tecINCORRECT_SYNTAX, "get property only");
            addDbgCode(code);
            if (varSymTypeTmp != varId) {
                if (code.symIdx == Size) tmpTp = INT_T;
                else if (code.symIdx == IndexOf) { 
                    code = nextCode(); code = chkNextCode(code, '(');
                    DtType tmpTp2 = getExpression_syntax(0, 0).getType();
                    if ((tmpTp != tmpTp2) && (!isNumericType(tmpTp) || !isNumericType(tmpTp2)))
                        errorExit(tecINVALID_VARIABLE_TYPE, "mismatch type");
                    if (code.kind == ',') { // 검색 시작 위치
                        code = nextCode();
                        tmpTp2 = getExpression_syntax(0, 0).getType();
                        if (!isNumericType(tmpTp2)) errorExit(tecNEED_NUMBER_TYPE, "start_index");
                        if (code.kind == ',') {
                            code = nextCode();
                            tmpTp2 = getExpression_syntax(0, ')').getType();
                            if (!isNumericType(tmpTp2)) errorExit(tecNEED_NUMBER_TYPE, "search_range");
                        }
                        else code = chkNextCode(code, ')');
                    }
                    else code = chkNextCode(code, ')');
                    mstk.push(INT_T, 1);
                    break;
                }
                else errorExit(tecINVALID_SYSTEM_METHOD, "wrong code (array's property function)");
            }
            else {
                if (code.symIdx == ToString && tmpTp != STR_T)
                    tmpTp = STR_T;
                else if (code.symIdx == Size && tmpTp == STR_T)
                    tmpTp = INT_T;
                else 
                    errorExit(tecINVALID_SYSTEM_METHOD, "wrong code (variable's property function)");
            }
            code = nextCode();
            code = chkNextCode(code, '(');
            code = chkNextCode(code, ')');
            removeDbgCode();
        }
        else if (varSymTypeTmp != varId) {
            errorExit(tecNEED_VARIABLE_TYPE, "Invalid array type");
        }
        else {
            if (code.kind == DBPlusR || code.kind == DBMinusR) {
                if (!isNumericType(tmpTp)) errorExit(tecNEED_NUMBER_TYPE, "Numeric Type only (++/--)");
                code = nextCode(); 
            }
        }
        mstk.push(getInitVar(tmpTp));
        break; 
        }
    case System:
        mstk.push(mutil.getObj(code.symIdx));
        code = nextCode();
        break;
    case Math:
        execSysFunc_syntax(true);
        break;
    case Fcall:
        callFunction_syntax(code.symIdx, true);
        break;
    case EofLine:
        errorExit(tecINCORRECT_EXPRESSION, "The expression did not end normally.");
        break;
    default:
        errorExit(tecINCORRECT_EXPRESSION, "Not a valid expression");
    }
    removeDbgCode();
}

/** 함수 호출 문법 검사 */
void 
MareInitExec::callFunction_syntax(int fncIdx, bool needReturn) 
{
    if (needReturn && (Gtable[fncIdx].dtTyp == NON_T)) 
        errorExit(tecINVALID_FUNC_RETURN, Gtable[fncIdx].name, " function has no return value defined.");
    
    JLOG(mutil.j_.trace()) << " * callFunction (" << fncIdx << ") " << Gtable[fncIdx].name;
    vector<DtType> paramTypes;
    int argCt = 0;
    code = nextCode(); 
    code = chkNextCode(code, '(');
    if (code.kind != ')') {                                /* ')'가 아니면, 인수가 존재함. */
        for (;; code=nextCode()) {
            addDbgCode(Expression, GET_ARGUMENT_TYPE, argCt);
            DtType dtp = getExpression_syntax().getType(); /* 인수식 처리 */
            paramTypes.push_back(dtp); 
            ++argCt;                                       /* 인수식 처리 후, 인수 개수를 하나 증가 */
            removeDbgCode();
            if (code.kind != ',') break;                   /* ',' 이면, 인수가 계속됨 */
        }
    }
    JLOG(mutil.j_.trace()) << " * callFunction stacks: " << kind2Str(code.kind);
    code = chkNextCode(code, ')');                         /* ')'가 있는지 확인 */

    if (argCt > Gtable[fncIdx].args || argCt < Gtable[fncIdx].aryLen)    /* 인수 개수 검사 */
        errorExit(tecINVALID_FUNC_ARGUMENT, Gtable[fncIdx].name, " Invalid parameter count");

    if (paramTypes.size() > 0) {
        JLOG(mutil.j_.trace()) << " * check function param type";
        char* funcCode = intenalCode[Gtable[fncIdx].adrs];
        funcCode += 5;   /* skip code */
        TknKind kd_;
        int idx = 0;
        do {
            addDbgCode(Expression, SET_PARAMETER_TYPE, idx);
            kd_ = (TknKind)*UCHAR_P(funcCode++);
            if (kd_ == Lvar) {
                short tblIdx = *SHORT_P(funcCode); funcCode += SHORT_SIZE;
                CodeSet cs(kd_, tblIdx, -1);
                DtType pty = symTablePt(cs)->dtTyp;
                if (isNumericType(pty)){
                    if (pty == DBL_T && paramTypes[idx] == INT_T){
                        JLOG(mutil.j_.warn()) << Gtable[fncIdx].name
                             <<  ": double형 값을 int형 parameter에 할당합니다.";
                    }
                    if (!isNumericType(paramTypes[idx++]))
                        errorExit(tecINVALID_FUNC_ARGUMENT, "Invalid parameter type");
                }
                else {
                    if (pty != paramTypes[idx++])
                        errorExit(tecINVALID_FUNC_ARGUMENT, "Invalid parameter type");
                }
                --argCt;
            }
            else {
                // 에러??
                errorExit(tecINVALID_FUNC_ARGUMENT, "Wrong type of param");
            }
            kd_ = (TknKind)*UCHAR_P(funcCode++);
            if (kd_ == '=') {
                // Literals 할당 skip
                funcCode++; funcCode += SHORT_SIZE;
                kd_ = (TknKind)*UCHAR_P(funcCode++);
            }
            removeDbgCode();

        } while (kd_ == ',' && argCt > 0);
    }
    
    if (needReturn) {
        mstk.push(getInitVar(Gtable[fncIdx].dtTyp));       /* 적당한 임시 반환 값 */
    }
}

/** 내장함수 검사 */
void 
MareInitExec::execSysFunc_syntax(bool needReturn) 
{
    CodeSet cs = code;
    JLOG(mutil.j_.trace()) << " execSysFunc_syntax: " << kind2Str(cs);
    vector<VarObj> vs;
    short p;

    switch (cs.kind) {
    case ToArray:
    {
        code = nextCode();
        code = chkNextCode(code, '(');
        auto symPt = symTablePt(code);             /* 변수 속성: 순서에 주의(getMemAdrs보다 먼저) */
        if (symPt->symKind == varId) errorExit(tecINVALID_TYPE, "Need a variable of array");
        DtType varType;
        int varAdrs = getMemAdrs(code, varSymType, varType);
        if (varSymType != varId) errorExit(tecINVALID_TYPE, "Required a variable of array");
        VarObj oo; oo.init(varType);
        code = chkNextCode(code, ',');
        oo = getExpression_syntax(0, 0);           /* type 확인 */
        code = chkNextCode(code, ',');
        oo = getExpression_syntax(0, 0);           /* type 확인 */
        ++varAdrs;                                 /* 첫번째 인수 +0, 두번째 인수 +1 */
        while (code.kind == ',') {                 /* ',' 라면 인수가 계속됨 */
            //if (++varAdrs > varEndAdrs) errorExit(tecEXCEED_ARRAY_LENGTH, "Exceed index of array");
            code = nextCode();
            oo = getExpression_syntax(0, 0);       /* type 확인 */
        } 
        code = chkNextCode(code, ')');
        chkEofLine();
    }
        break;
    case Exit: 
    case Log:
        code = nextCode();
        code = chkNextCode(code, '(');
        JLOG(mutil.j_.trace()) << " Log start: " << kind2Str(code.kind);
        while (true) {
            getExpression_syntax(0, 0);            /* 값 출력 확인 */
            if (code.kind != ',') break;           /* ',' 라면 인수가 계속됨 */
            code = nextCode();
        }                
        JLOG(mutil.j_.trace()) << " Log end: " << kind2Str(code.kind);
        code = chkNextCode(code, ')');
        chkEofLine();
        break;
    case Math: 
        if (!needReturn) errorExit(tecINVALID_FUNC_RETURN, "Invalid return value handling");
        p = setParams_syntax(vs, 2);
        if (p == 0) errorExit(tecINVALID_FUNC_ARGUMENT, "Invalid parameter count");
        if (!isNumericType(vs[0].getType())) errorExit(tecNEED_NUMBER_TYPE, "Invalid 1st parameter type");

        if (cs.symIdx == Ceil || cs.symIdx == Floor || cs.symIdx == Abs
             || cs.symIdx == Round || cs.symIdx == Sqrt) {
            if (p != 1) errorExit(tecINVALID_FUNC_ARGUMENT, "Invalid parameter count");
            mstk.push(DBL_T, 1);
        }
        else if (cs.symIdx == Pow) {
            if (p != 2) errorExit(tecINVALID_FUNC_ARGUMENT, "Invalid parameter count");
            if (!isNumericType(vs[1].getType())) errorExit(tecNEED_NUMBER_TYPE, "Invalid 2nd parameter type");
            mstk.push(DBL_T, 1);
        }
        else {
            errorExit(tecNO_FUNC, "function is not defined");
        }
        break;
    default: // MARE
        JLOG(mutil.j_.error()) << " [execSysFunc_syntax] skip: " << kind2Str(cs.kind);
        errorExit(tecNO_FUNC, "function is not defined");
    }
}

/** 함수의 파라미터 처리 (params가 numOfParams보다 많으면 에러) */
short 
MareInitExec::setParams_syntax(vector<VarObj>& vs, short numOfParams) {
    
    --numOfParams;                                   /* 마지막 parameter는 따로 처리 */
    vs.clear();
    code = nextCode();
    code = chkNextCode(code, '(');
    short i;
    for (i=0; i<numOfParams; i++) {
        addDbgCode(Expression, GET_ARGUMENT_TYPE, i);
        vs.push_back(getExpression_syntax(0, 0));    /* Params 값 저장 */
        removeDbgCode();
        if (code.kind == ')') { 
            code = nextCode();
            return vs.size(); 
        }
        code = chkNextCode(code, ',');
    }
    addDbgCode(Expression, GET_ARGUMENT_TYPE, i);
    vs.push_back(getExpression_syntax(0, ')'));      /* 마지막 값 저장 (Params 최대 개수 확인) */
    removeDbgCode();
    return vs.size();
}

}