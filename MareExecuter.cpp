
#include "MareExecuter.h"

namespace mare_vm {

/** 함수가 있을 경우, 함소 호출을 위한 코드를 추가함. */
short
MareExecuter::prepareExecute(string const& fname, vector<string> const& params) {

    blkNest = 0;
    readedCodeCnt = 0;
    initDbgCode(CodeSet(String, fname.c_str()));
    code = CodeSet(EofLine);
    // 함수 이름이 없는 경우, 에러 
    if (fname == "" || fname.length() == 0) 
        errorExit(tecCONTRACT_PREPROCESS_ERROR, "prepare: function name is not found.");

    SymTbl fu;
    short funcLocation = getSymTable(fname, fu);
    
    JLOG(mutil.j_.debug()) << " prepare: '" << fname << "', symbol idx: " << funcLocation;

    if (funcLocation == -1) {
        errorExit(tecCONTRACT_PREPROCESS_ERROR, "prepare: function name is not found.");
    }
    initDbgCode(CodeSet(Fcall, funcLocation, funcLocation));

    codeBuf_p = codeBuf;                                   /* 초기화 */

    short OffsetArgs = Gtable[funcLocation].args;
    Pc = Gtable[funcLocation].adrs;
    code = firstCode(Pc);

    code = nextCode(); // '(' skip
    code = nextCode(); // first parameter or ')'

    short startPc = intenalCode.size();
    size_t size = params.size();
    if (OffsetArgs < size) {
        errorExit(tecINVALID_FUNC_ARGUMENT, "prepare: Invalid parameter count");
    }

    if (OffsetArgs == 0) {
        if (size > 0) errorExit(tecINVALID_FUNC_ARGUMENT, "prepare: This function is argumentless.");
        setCode(Fcall, funcLocation); setCode('('); setCode(')');
    }
    else if (size == 0) {
        if (Gtable[funcLocation].aryLen > 0) 
            errorExit(tecINVALID_FUNC_ARGUMENT, "prepare: This function has arguments.");
        setCode(Fcall, funcLocation); setCode('('); setCode(')');
    }
    else {
        OffsetArgs -= size;  
        if (size < Gtable[funcLocation].aryLen) 
            errorExit(tecINVALID_FUNC_ARGUMENT, "prepare: Need more arguments.");

        setCode(Fcall, funcLocation); 
        setCode('('); 
        //JLOG(mutil.j_.trace()) << size << " ?? " << kind2Str(code.kind);
        for (int k=0; k<size; ) {
            if (code.kind == ')') errorExit(tecINVALID_FUNC_ARGUMENT, "error: Invalid parameter count");

            DtType dt = symTablePt(code)->dtTyp;           /* 인수 값이 존재한다면 추가.(상수값) */
            TknKind tk = (TknKind)(dt + VAR_LITERAL_OFFSET);

            string txt = params[k];
            JLOG(mutil.j_.trace()) << " set param -value:'" << txt << "' -type:" << kind2Str(tk);
            if (dt == DATETIME_T) {
                isUnsignedIntStr(txt);
                setCode(tk, setLITERAL(atol(params[k].c_str())));
            }
            else if (dt == INT_T) {
                if (txt == "true") setCode(tk, setLITERAL(1));
                else if (txt == "false") setCode(tk, setLITERAL(0));
                else { isIntStr(txt); setCode(tk, setLITERAL(atol(txt.c_str()))); }
            }
            else if (dt == DBL_T) {
                if (txt == "true") setCode(tk, setLITERAL(1));
                else if (txt == "false") setCode(tk, setLITERAL(0));
                else { isNumberStr(txt); setCode(tk, setLITERAL(atof(txt.c_str()))); }
            }
            else {                
                setCode(tk, setLITERAL(txt));
            }
            code = nextCode();

            if (++k != size) {                               /* 마지막 인수값 다음에는 ','가 없어야 함 */
                if (code.kind == ',') { setCode(','); code = nextCode(); }
                else if (code.kind == '=') { 
                    do { code = nextCode(); } while (code.kind != ',' && code.kind != ')');
                    setCode(','); code = nextCode(); 
                }
                JLOG(mutil.j_.trace()) << " " << kind2Str(code);
            }
        }
        
        setCode(')');
    }
    // 함수호출 코드를 추가
    pushInternalCode();

    return startPc;
}

/** 함수가 있을 경우, 함소 호출을 위한 코드를 추가함. */
void
MareExecuter::prepareExecute(vector<string> const& params) {

    if (params.size() == 0) 
        errorExit(tecCONTRACT_PREPROCESS_ERROR, "Need to function name.");

    string funcname = params[0];
    vector<string> args;
    if (params.size() > 1){
        for (size_t i=1; i<params.size(); i++)
            args.push_back(params[i]);
    }

    execute(funcname, args);
}

void
MareExecuter::execute(string const& fname, vector<string> const& params)
{
    string errTypeStr = "preapre to execute error";
    try {
        short startpc = prepareExecute(fname, params);
        errTypeStr = "execute error";
        execute(startpc);
    }
    catch(ErrObj err) {
        throw err;
    }
    catch(TECcodes err_code) {
        errorExit(err_code, errTypeStr, "(tec code)");
    }
    catch(int err_code) {
        errorExit(err_code, errTypeStr, "(Err code)");
    }
    catch(const char* err_msg) {
        errorExit(tecUNKNOWN_CONTRACT_ERROR, err_msg);
    }
    catch(std::runtime_error err){
        errorExit(tecUNKNOWN_CONTRACT_ERROR, err.what());
    }
    catch(std::exception err) {
        errorExit(tecUNKNOWN_CONTRACT_ERROR, err.what());
    }
    catch(...){
        errorExit(tecUNKNOWN_CONTRACT_ERROR, "Undefined error");
    }
}

/** 코드 실행 (startPc가 코드의 시작 line) */
void 
MareExecuter::execute(short startPc)
{
    JLOG(mutil.j_.trace()) << "execute -start line: " << startPc;
    blkNest = 0;
    readedCodeCnt = 0;
    baseReg = 0;                                            /* 베이스 레지스터 초깃값 */
    stpReg = DynamicMem.size();                             /* 스택 포인터 초깃값 */
    DynamicMem.resize(stpReg+MEMORY_RESIZE);                /* 메모리 영역 초기 확보 */
    breakFlag = conFlag = returnFlag = exitFlag = false;

    Pc = startPc;
    maxCodeLine = intenalCode.size() - 1;
    while (Pc<=maxCodeLine && !exitFlag) {
        statement();
    }

    // 비실행 상태
    Pc = -1;
    // 결과 데이터 보여주기 (MareStack에는 데이터가 없어야 정상임)
    // short sz = mstk.size();
    // if (sz > 0) {
    //     JLOG(mutil.j_.error()) << " ======= Mare Stack (size:" << sz << ")=======";
    //     for (int kk=0; kk<sz; kk++) {
    //         VarObj oo = mstk.pop();
    //         JLOG(mutil.j_.error()) << kk << ": " << oo.toFullString(true);
    //     }
    // }
    // 디버깅용 
    // JLOG(mutil.j_.trace()) << " ======= Mare Memory ===============";
    // JLOG(mutil.j_.trace()) << DynamicMem.to_string();
}

/** 문 실행 (현재 line의 시작 부분) */
void 
MareExecuter::statement() 
{
    CodeSet save;
    short top_line, end_line;
    int varAdrs;
    VarObj wkVal;
    bool isArray;

    if (Pc>maxCodeLine || exitFlag) return;                 /* 프로그램 종료 */

    code = save = firstCode(Pc);                            /* 현재 line의 시작 코드 */
    if (code.kind != EofLine) initDbgCode(code);
    else { initDbgCode(); ++Pc; return; }
    top_line = Pc; end_line = code.jmpAdrs;                 /* 코드 제어 범위의 시작과 끝 설정 */

    JLOG(mutil.j_.trace()) << " ===== Start Next Line ( No. "<< Pc << ", Count:" << readedCodeCnt << ") =====";
    JLOG(mutil.j_.debug()) << debugCodeSet(code);

    switch (code.kind) 
    {
    case DeclareVar:
    case DeclareArr:                                        /* 변수(배열) 선언 처리 */
        {
        bool isArray = false;
        do {
            if (code.kind == DeclareArr) isArray = true;
            else isArray = false;
            code = save = nextCode();
            if (isArray) { setInitArray(); code = nextCode(); }
            else assignVariable(save, true);

            if (code.kind == EofLine) break;
            else if (code.kind == ',') code = nextCode(); 
            else errorExit(tecINCORRECT_SYNTAX, "잘못된 구문.");
        } while (code.kind == DeclareArr || code.kind == DeclareVar);
        }
        ++Pc;
        break;
    case Gvar: 
    case Lvar:                                              /* 대입문 처리 */
        removeDbgCode();
        assignVariable(save);
        ++Pc;
        break;
    case If:
        end_line = getEndLineOfIf(Pc);                      /* if 문의 마지막 line 위치 */
        // if
        if (getExpression(If, 0).getDbl() == 1.0) {         /* 조건이 참(TRUE)인지? */
            ++Pc; block(); Pc = end_line + 1;               /* block을 실행하고, 종료 */
            return;
        }
        Pc = save.jmpAdrs;                                  /* 다음 조건문 위치 */
        if (lookCode(Pc) == Close) Pc++;
        // elif
        while (lookCode(Pc) == Elif) {
            save = firstCode(Pc); code = nextCode();
            if (getExpression().getDbl() == 1.0) {          /* 조건이 참(TRUE)인지? */
                ++Pc; block(); Pc = end_line + 1;           /* block을 실행하고, 종료 */
                return;
            }
            Pc = save.jmpAdrs;                              /* 다음 조건문 위치 */
            if (lookCode(Pc) == Close) Pc++;
        }
        // else
        if (lookCode(Pc) == Else) {                         /* else인지 확인 */
            ++Pc; block(); Pc = end_line + 1;               /* block을 실행하고, 종료 */
            return;
        }
        // end
        ++Pc;
        break;
    case While:
        for (;;) {
            if (!getExpression(While, 0).getDbl()) break;   /* 조건이 거짓(False)이면 종료 */                
            ++Pc; block();                                  /* block을 실행 */
            if (breakFlag || returnFlag || exitFlag) {      /* 플래그 상태에 따른 분기 처리 */
                breakFlag = false; break;                   /* 현재 블록 실행 중단 */
            }
            else if (conFlag) conFlag = false;
            Pc = top_line; code = firstCode(Pc);            /* 조건 학인을 위해, while 시작 위치로 이동 */
        }
        Pc = end_line + 1;                                  /* while 블록의 끝(밖)으로 이동 */
        break;
    case Foreach:									        /* foreach : 제어변수, 배열 */
    {
        code = nextCode(); code = nextCode();
        wkVal.init(symTablePt(code)->dtTyp);                /* 제어변수 타입 설정 */
        varAdrs = getMemAdrs(code, isArray);                /* 제어 변수의 주소 구하기 */
        code = nextCode();
        int idxAdr = getTopAdrs(code);                      /* 배열 변수의 시작주소 구하기 */
        unsigned short idxLen = symTablePt(code)->aryLen;
        idxLen += idxAdr;
        for (short i=idxAdr; i<idxLen; i++) {
            wkVal = DynamicMem.get(i);                      /* 제어변수에 할당할 초기값 가져오기 */
            DynamicMem.set(varAdrs, wkVal);                 /* 제어변수 값 업데이트 */

            Pc=(top_line + 1);                              /* for문 데이터의 시작 위치로  */
            block();                                        /* block을 실행 */
            if (breakFlag || returnFlag || exitFlag) {      /* 플래그 상태에 따른 분기 처리 */
                breakFlag = false; break;                   /* 현재 블록 실행 중단 */
            }
            else if (conFlag) conFlag = false;
        }
        Pc = end_line + 1;                                  /* foreach 블록의 끝(밖)으로 이동 */
        break;
    }
    case For:										        /* for : 제어변수, 초깃값, 최종값, 증분식 */
        code = nextCode(); save = nextCode();
        varAdrs = getMemAdrs(save, isArray);                /* 제어 변수의 주소 구하기 */
        expression('=', ';');                               /* 제어 변수의 초기값 계산 */

        if (!isNumericType(mstk.topType())) 
            errorExit(tecNEED_NUMBER_TYPE, "wrong control variable type");

        DynamicMem.set(varAdrs, mstk.pop());                /* 초기값을 제어 변수에 할당 */
        while ( code.kind != ';') { code = nextCode(); }    /* 증분값 SKIP */
        code = nextCode();                                  /* ; 건너뜀 */

        for ( ; ; ) {
            if (code.kind != ')') {                         /* 실행조건이 있는지 확인 */
                if (getExpression(0, ')').getDbl() != 1.0)  /* 실행 조건 확인 */
                    break;
            }
            Pc=(top_line + 1);                              /* for문 데이터의 시작 위치로  */
            block();                                        /* block을 실행 */
            if (breakFlag || returnFlag || exitFlag) {      /* 플래그 상태에 따른 분기 처리 */
                breakFlag = false; break;                   /* 현재 블록 실행 중단 */
            }
            else if (conFlag) conFlag = false;

            Pc=top_line;                                    /* for문의 시작 위치부터 시작하도록 */
            code = firstCode(Pc);
            do { code = nextCode(); } while ( code.kind != ';');

            code = save = nextCode();                       /* 초기값까지 건너뜀 */
            if (code.kind == ';') code = nextCode();        /* 증분값 없음 */
            else {
                if (code.kind == Gvar || code.kind == Lvar) /* 증분값 처리 */
                    { assignVariable(save); code = nextCode(); }
                else (void)getExpression(0, ';');           /* ++i, --i와 같은 형태로 추정됨 */
            }
        }
        Pc = end_line + 1;                                  /* for 블록의 끝(밖)으로 이동 */
        break;
    case Break:
        code = nextCode(); 
        setEndSubIf(breakFlag);                            /* '?'가 있으면 break의 실행 조건 처리 */
        if (!breakFlag) ++Pc;
        break;
    case Continue:
        code = nextCode(); 
        setEndSubIf(conFlag);                              /* '?'가 있으면 continue 실행 조건 처리 */
        if (!conFlag) ++Pc;
        break;
    case Return:
        wkVal.init(returnValue.getType());                  /* 현재 함수의 return type 저장 */
        wkVal = returnValue;                                /* return 값을 임시로 백업 */
        code = nextCode();
        if (code.kind != '?' && code.kind != EofLine)       /* '식'이 있으면 반환 값을 계산 */
            wkVal = getExpression();
        setEndSubIf(returnFlag);                            /* '?'가 있으면 return의 실행 조건 처리 */
        if (returnFlag) returnValue = wkVal;
        else ++Pc;                                          /* return 조건이 false일 경우 */
        break;
    case Fcall:                                             /* 대입이 없는 함수 호출 */
        callFunction(code.symIdx);                          /* 함수 호출 수행 */
        (void)mstk.pop();                                   /* stack에 있는 (불필요한) 반환 값 처리 */
        ++Pc;
        break;
    case Func:                                              /* 함수 정의는 건너뀜 */
        Pc = end_line + 1;
        break;
    case Require:
        code = nextCode();
        if (getExpression().getDbl() != 1.0) {
            errorExit(tecCONTRACT_REQUIRE_FAIL, "Does not meet the requirements.");
        }
        ++Pc;
        break;
    case Throws:
        errorExit(code.symIdx, "throw : exception by user");
    case Exit:
    case Log:
    case ToArray:
        execSysFunc();
        ++Pc;
        break;
    case DBPlus:
    {
        code = nextCode(); 
        int varAdrs = getMemAdrs(code, isArray);       /* 저장할 변수 주소 */
        VarObj old = DynamicMem.get(varAdrs);          /* 일치하지 않을 경우, 타입 확인하여 값을 저장 */
        DynamicMem.set(varAdrs, ++old);
        ++Pc;
        break;
    }
    case DBMinus:
    {
        code = nextCode(); 
        int varAdrs = getMemAdrs(code, isArray);       /* 저장할 변수 주소 */
        VarObj old = DynamicMem.get(varAdrs);          /* 일치하지 않을 경우, 타입 확인하여 값을 저장 */
        DynamicMem.set(varAdrs, --old);
        ++Pc;
        break;
    }
    case Do:
    case Close:
    //case EofLine:
        /* 실행 시는 무시 */
        ++Pc;
        break;
    default:
        errorExit(tecINCORRECT_SYNTAX, "잘못된 기술입니다.");
    }
}

/** 할당 연산자 처리 (=, +=, -=, *=, /=, var++, var--)
 * save: 현재 변수 정보를 담고있는 코드
 * code: 현재 변수의 다음 코드 (연산자 정보)
 */
void 
MareExecuter::assignVariable(CodeSet const save, bool declare) 
{
    addDbgCode(save);
    bool isArray;
    int varAdrs = getMemAdrs(code, isArray);       /* 저장할 변수 주소 */
    if (isArray) errorExit(tecNEED_VARIABLE_TYPE, "Can't assign value to array type");
    VarObj old = DynamicMem.get(varAdrs);          /* 일치하지 않을 경우, 타입 확인하여 값을 저장 */
    JLOG(mutil.j_.trace()) << " * type:" << kind2Str(save.kind)
            << " varAdrs:" << varAdrs << " " << old.toFullString(true);
    
    if (code.kind == Assign) {
        addDbgCode(code);
        if (old.getType() == NON_T) {                   /* 초기화가 필요한 경우 */
            DtType tmpTp = symTablePt(save)->dtTyp;     /* 변수 타입  */
            old.init(tmpTp);
        }
        expression('=', 0);                            /* 변수에 할당할 값 계산 */
        old = mstk.pop();                              /* = 대입 작업 수행 */ 
        removeDbgCode();
    }
    else if (declare && (code.kind == ',' || code.kind == EofLine)) {
        old.init(NON_T);                               /* 변수 선언 상태로 변경 */
    }
    else {
        addDbgCode(code);
        if (old.getType() == NON_T){
            errorExit(tecNEED_INIT_VARIABLE, "Attempt to use an uninitialized variable.");
        }

        if (code.kind == PlusAssign){
            expression(PlusAssign, 0);                 /* 변수에 더할 값을 계산 */
            old += mstk.pop();                         /* += 대입 작업 수행 */                
        }
        else if (code.kind == MinusAssign){
            expression(MinusAssign, 0);                /* 변수에서 뺄 값을 계산 */
            old -= mstk.pop();                         /* -= 대입 작업 수행 */
        }
        else if (code.kind == MultiAssign){
            expression(MultiAssign, 0);                /* 변수에서 곱할 값을 계산 */
            old *= mstk.pop();                         /* *= 대입 작업 수행 */
        }
        else if (code.kind == DiviAssign){
            expression(DiviAssign, 0);                 /* 변수에서 나눌 값을 계산 */
            old /= mstk.pop();                         /* /= 대입 작업 수행 */
        }
        else if (code.kind == DBPlusR){
            ++old;                                     /* ++ 연산 작업 수행 */
            code = nextCode();
        }
        else if (code.kind == DBMinusR){
            --old;                                     /* -- 연산 작업 수행 */
            code = nextCode();
        }
        else {
            errorExit(tecINCORRECT_EXPRESSION, "잘못된 구문입니다.");
        }
        removeDbgCode();
    }
    JLOG(mutil.j_.trace()) << " ** updated varAdrs:" << varAdrs << " " << old.toFullString(true);
    DynamicMem.set(varAdrs, old);                 /* 최종 변수 값으로 메모리 갱신 */
    removeDbgCode(); 
}

/** 블록 끝까지 문을 실행 */
void 
MareExecuter::block()  
{
    blkNest++; cout << endl << " ========== 업 : " << blkNest;
    TknKind k;
    while (!breakFlag && !conFlag && !returnFlag && !exitFlag) { /* break, return 등으로 인한 종료 플래그 확인 */
        k = lookCode(Pc);                               /* 다음 line의 시작 코드 확인 */
        if (k==End || k==Else || k==Elif) break;        /* 다음 line이 블록의 정상 종료 조건인지 확인 */
        statement();        
    }
    blkNest--; cout << endl << " ========== 다운 : " << blkNest;
}

/** 
 * expression 계산 후, 결과를 반환
 * -kind1이 0이 아니면, kind1으로 시작하는지 확인 
 * -kind2가 0이 아니면, 조건식이 끝난 다음에 kind2가 나오는지 확인 
 */
VarObj 
MareExecuter::getExpression(short kind1, short kind2)
{
    JLOG(mutil.j_.trace()) << " * getExpression(" << kind2Str((TknKind)kind1) << ", " << kind2Str((TknKind)kind2) << ")";
    expression(kind1, kind2); 
    return mstk.pop();
}

/** 
 * 조건부 일반식처리 
 * -kind1이 0이 아니면, kind1으로 시작하는지 확인 
 * -kind2가 0이 아니면, 조건식이 끝난 다음에 kind2가 나오는지 확인 
 */
void 
MareExecuter::expression(short kind1, short kind2) 
{
    if (kind1 != 0) code = chkNextCode(code, kind1);
    addDbgCode(Expression, NORMAL_TYPE, ++countExps);  /* Expression이 시작함 */
    term(1);                                           /* 이항연산 모듈 처리 */
    removeDbgCode();                                   /* Expression이 종료함 */
    if (kind2 != 0) code = chkNextCode(code, kind2);
}

/** n은 우선 순위 */
void 
MareExecuter::term(short n) 
{
    TknKind op;
    if (n == 8) {
        factor(); 
        return;
    }
    term(n+1);
    while (n == opOrder(code.kind)) {         /* 우선 순위가 같은 연산자가 연속되도록 */
        op = code.kind;
        code = nextCode(); 
        term(n+1);

        binaryExpr(op);
    }
}

/** 식의 인자 처리 */
void 
MareExecuter::factor() 
{
    addDbgCode(code);
    TknKind kd = code.kind;    
    JLOG(mutil.j_.trace()) << " *** factor:'" << kind2Str(kd) << "':" << kind2Str(code);

    switch (kd) {                                  /* 실행시 */
    case Not: 
    case Minus: 
        code = nextCode(); factor();               /* 다음 값을 획득 */
        {
            VarObj o = mstk.pop();
            if (!isNumericType(o.getType())) errorExit(tecNEED_NUMBER_TYPE, "숫자형 값이 아닙니다.");
            double res = o.getDbl();
            if (kd == Not) res = !res;             /* 단항 ! 처리 수행 */
            else res = -res;                       /* 단항 - 처리 수행 */
            o.set(res);
            mstk.push(o);
        }
        break;
    case Plus:                                     /* 단항 +는 특별히 처리할 것이 없음 */
        code = nextCode(); factor();               /* 다음 값을 획득 */
        break;
    case DBPlus:               /* 원래 값(DynamicMem의 값)을 1 증가시켜 저장한 후, stack에 추가(반환) */
    case DBMinus:              /* 원래 값(DynamicMem의 값)을 1 감소시켜 저장한 후, stack에 추가(반환) */
        code = nextCode();
        {
            // DtType tmpTp = symTablePt(code)->dtTyp; /* 변수 타입 : 순서에 주의 (getMemAdrs보다 먼저) */
            bool isArray;
            addDbgCode(code);
            int varAdrs = getMemAdrs(code, isArray);
            VarObj oo = DynamicMem.get(varAdrs);
            if (oo.getType() == NON_T) errorExit(tecNEED_INIT_VARIABLE, "Attempt to use an uninitialized variable.");
            if (kd == DBPlus) ++oo;
            else              --oo;
            mstk.push(oo); DynamicMem.set(varAdrs, oo);
            removeDbgCode();
        }
        break;
    case Lparen:
        expression('(', ')');
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
    case Gvar: 
    case Lvar:
        {
        DtType tmpTp = symTablePt(code)->dtTyp;    /* 변수 타입 : 순서에 주의 (getMemAdrs보다 먼저) */
        unsigned short tmpSz = symTablePt(code)->aryLen;
        bool isArray;
        int varAdrs = getMemAdrs(code, isArray);
        VarObj oo = DynamicMem.get(varAdrs);
        JLOG(mutil.j_.trace()) << " ** " << kind2Str((TknKind)tmpTp) << " -> " << oo.toFullString(true);
        if (oo.getType() == NON_T) errorExit(tecNEED_INIT_VARIABLE, "Attempt to use an uninitialized variable.");
        
        if (code.kind == '.') {                    /* 변수의 값이 아닌 속성(함수)일 경우 */
            code = nextCode();
            if (isArray) {
                if (code.kind == Size)
                    mstk.push(INT_T, tmpSz);
                else if (code.kind == Find) {
                    code = nextCode(); 
                    expression('(', 0);
                    VarObj compObj = mstk.pop();
                    int startIdx = 0;
                    if (code.kind == ',') {
                        code = nextCode(); 
                        expression(0, ')');
                        startIdx = mstk.pop().getDbl();
                        if (startIdx >= tmpSz) errorExit(tecEXCEED_ARRAY_LENGTH);
                    }
                    else code = nextCode();        /* ')' skip */
                    bool isFind = false;
                    for (int adr=startIdx; adr<tmpSz; adr++) {
                        if ((compObj == DynamicMem.get(adr + varAdrs)).getDbl()){
                            isFind = true;
                            mstk.push(INT_T, adr);
                            break;
                        }
                    }
                    if (!isFind) mstk.push(INT_T, -1);
                    break;
                }
                else 
                    errorExit(tecINVALID_SYSTEM_METHOD, "wrong code (array's property function)");
            }
            else {
                if (code.kind == ToString)
                    mstk.push(STR_T, oo.toStr());
                else if (code.kind == Size)
                    mstk.push(INT_T, oo.toStr().length());
                else 
                    errorExit(tecINVALID_SYSTEM_METHOD, "wrong code (variable's property function)");
            }
            code = nextCode(); code = nextCode(); code = nextCode();
        }
        else {
            if (oo.getType() == tmpTp) mstk.push(oo); 
            else { VarObj onw(tmpTp, oo); mstk.push(onw); }
        
            if (code.kind == DBPlusR) {            /* 원래 값(DynamicMem의 값)은 1 증가, 증가 전의 값을 반환(stack 추가) */
                DynamicMem.set(varAdrs, ++oo);
                code = nextCode();
            }
            else if (code.kind == DBMinusR) {      /* 원래 값(DynamicMem의 값)은 1 감소, 감소 전의 값을 반환(stack 추가) */
                DynamicMem.set(varAdrs, --oo);
                code = nextCode();
            }
        }
        break;
        }
    case System:
    {
        VarObj oo = mutil.getObj(code.symIdx);
        mstk.push(oo);
        JLOG(mutil.j_.trace()) << " ** " << oo.toFullString(true);
        code = nextCode();
        break;
    }
    case Math:
        execSysFunc();
        break;
    case Fcall:
        callFunction(code.symIdx);
        break;
    default: // MARE (factor_syntax함수에서 이미 에러로 처리)
        JLOG(mutil.j_.error()) << " ** what code? " << kind2Str(code.kind) << ":" << kind2Str(code);
    }
    removeDbgCode();
}

/** 이항 연산자의 우선 순위 */
short 
MareExecuter::opOrder(TknKind kd) 
{
    switch (kd) {
    case DBPlus: case DBPlusR:
    case DBMinus:case DBMinusR:        return 7; /* ++  --     */
    case Multi:  case Divi:  case Mod: return 6; /* *  /  % \  */
    case Plus:   case Minus:           return 5; /* +  -       */
    case Less:   case LessEq:
    case Great:  case GreatEq:         return 4; /* <  <= > >= */
    case Equal:  case NotEq:           return 3; /* == !=      */
    case And:                          return 2; /* &&         */
    case Or:                           return 1; /* ||         */
    default:                           return 0; /* 해당 없음   */
    }
}

/* 이항 연산 */
void 
MareExecuter::binaryExpr(TknKind op) 
{
    VarObj d2 = mstk.pop();
    VarObj d1 = mstk.pop();
    VarObj d;

    JLOG(mutil.j_.trace()) << " binaryExpr() : " << d1.toFullString(true)
                    << " " << kind2Str(op) << " " << d2.toFullString(true);

    addDbgCode(CodeSet(op));

    switch (op) {
    case Plus:    d = (d1 + d2); break;
    case Minus:   d = (d1 - d2); break;
    case Multi:   d = d1 * d2;   break;
    case Divi:    d = d1 / d2;   break;
    case Mod:     d = d1 % d2;   break;
    case Less:    d = d1 <  d2;  break;
    case LessEq:  d = d1 <= d2;  break;
    case Great:   d = d1 >  d2;  break;
    case GreatEq: d = d1 >= d2;  break;
    case Equal:   d = d1 == d2;  break;
    case NotEq:   d = d1 != d2;  break;
    case And:     d = d1 && d2;  break;
    case Or:      d = d1 || d2;  break;
    default:
        errorExit(tecINCORRECT_EXPRESSION, "Binary operation is invalid. Operator:", kind2Str(op));
        break;
    }
    if (chkSyntaxMode) {
        /* 연산 결과가 0일 경우, 다음 연산의 결과에 0으로 나누는 버그가 발생할 수 있기 때문 */
        if (isNumericType(d.getType()) && d.getDbl() == 0) {
            mstk.push(d.getType(), 1); 
            return;
        }
    }
    mstk.push(d);
    removeDbgCode();
    JLOG(mutil.j_.trace()) << " => " << d.toFullString(true);
}

/** return, break 문 이후의 '?' 식 처리 */
void 
MareExecuter::setEndSubIf(bool& flg) 
{
    if (code.kind == EofLine) { flg = true; return; }       /* '?'가 없으면 플래그를 TRUE로 설정하고 처리를 끝냄 */
    if (getExpression('?', 0).getDbl() == 1.0) flg = true;  /* 조건식 처리 후, 결과에 따라 플래그를 설정 */
}

/** 함수 호출 처리 */
void 
MareExecuter::callFunction(short fncIdx) 
{
    vector<VarObj> vc;
    short n, argCt = 0;
    JLOG(mutil.j_.debug()) << " * callFunction [" << fncIdx << "] -name:" << Gtable[fncIdx].name;

    // 실인수 저장
    nextCode(); code = nextCode();                         /* 함수명과 '('는 건너뜀 */
    if (code.kind != ')') {                                /* ')'가 아니면, 인수가 존재함. */
        short i=0;
        for (;; code=nextCode()) {
            addDbgCode(Expression, GET_ARGUMENT_TYPE, i++);
            expression(0, 0); ++argCt;                     /* 인수식 처리 후, 인수 개수를 하나 증가 */
            removeDbgCode();
            if (code.kind != ',') break;                   /* ',' 이면, 인수가 계속됨 */
        }
    }
    code = nextCode();                                     /* ')' 건너뜀 */
    JLOG(mutil.j_.trace()) << "   ** function parameter stack size: " << argCt;

    for (n=0; n<argCt; n++) { vc.push_back(mstk.pop()); }  /* 뒤에서부터 인수를 저장하도록 수정 */
    for (n=0; n<argCt; n++) { mstk.push(vc[n]); }

    execFunction(fncIdx, argCt);                            /* 함수 실행 */
}

/** 함수 실행 */
void 
MareExecuter::execFunction(short fncIdx, short argCnt) 
{
    // 함수입구처리 (이전 상태 저장)
    short save_Pc = Pc;                                    /* 현재 실행행을 저장 */
    int save_baseReg = baseReg;                            /* 현재 baseReg를 저장 */
    int save_spReg = stpReg;                               /* 현재 spReg를 저장 */
    char *save_code_ptr = code_ptr;                        /* 현재 실행행 분석용 포인터를 저장 */
    CodeSet save_code = code;                              /* 현재 code를 저장 */
    DtType save_returnType = returnValue.getType();        /* 이전 함수 리턴 값의 타입 저장 */

    // 함수입구처리 (실행 준비)
    Pc = Gtable[fncIdx].adrs;                              /* 새로운 Pc 설정 */
    baseReg = stpReg;                                      /* 새로운 베이스 레지스터 설정 */
    stpReg += Gtable[fncIdx].frame;                        /* 함수의 프레임 영역 확보 */
    DynamicMem.autoResize(stpReg);                         /* 메모리 유효 영역 확보 */
    returnValue.init(Gtable[fncIdx].dtTyp);                /* 함수 반환 값의 기본값 설정 (void도 가능) */

    JLOG(mutil.j_.trace()) << " * execFunction: Pc:" << Pc;
    JLOG(mutil.j_.trace()) << " ** Func variable memory range: " << baseReg << " ~ " << stpReg;
    code = firstCode(Pc);                                  /* func 시작 코드 획득 */
    short OffsetArgs = Gtable[fncIdx].args - argCnt;

    short save_blkNest = blkNest;
    blkNest = dbgCode.size();
    cout << endl << " ========== 저장 : " << save_blkNest << " 변경 : " << blkNest;
    addDbgCode(code);                                      /* func의 시작임 */

    // 함수의 인수에 값을 저장 처리
    nextCode(); code = nextCode();                         /* Func의 End 위치와 '(' 건너뜀 */
    if (code.kind != ')') {                                /* ')'가 아니면, 인수가 존재함. */
        short i = 0;
        for (;; code=nextCode()) {
            JLOG(mutil.j_.trace()) << " ** set value of param -name: " << kind2Str(code);
            DtType tmpTp = symTablePt(code)->dtTyp;        /* 변수 타입 : 순서에 주의 (getMemAdrs보다 먼저) */
            JLOG(mutil.j_.trace()) << " ** param type: " << kind2Str((TknKind)tmpTp);
            addDbgCode(Expression, SET_PARAMETER_TYPE, i++);
            int varArgs;
            bool isArray;
            if (argCnt > 0) {
                argCnt--;
                varArgs = getMemAdrs(code, isArray);
                // if (isArray) errorExit(tecNEED_VARIABLE_TYPE, "배열을 인자값으로 할당 불가");
                VarObj oo = mstk.pop();
                JLOG(mutil.j_.trace()) << " ** param assign value: "
                     << oo.toFullString(true) << " args:" << varArgs;
                if (tmpTp == oo.getType()) {
                    DynamicMem.set(varArgs, oo);
                }
                else {
                    DynamicMem.set(varArgs, VarObj(tmpTp, oo));
                }
                if (code.kind == '=') {code = nextCode(); code = nextCode();}   /* 할당값이 있다면 skip */
            }
            else {
                JLOG(mutil.j_.trace()) << " ** param assign default value. -remain default args:" << (OffsetArgs - 1);
                assignVariable(code);
            }
            removeDbgCode();
            if (code.kind != ',') break;                   /* ',' 이면 인수가 계속됨 */
        }
    }
    removeDbgCode();
    code = nextCode();                                     /* ')' 건너뜀 */
    JLOG(mutil.j_.trace()) << " * execFunction body ";
    // 함수 본체 처리
    ++Pc; block(); returnFlag = false;                     /* 함수 본체 처리 */

    for (int i=baseReg; i<stpReg; i++)                     /* 함수에서 사용된 메모리 초기화 */
        DynamicMem.reset(i);

    // 함수 출구 처리
    mstk.push(returnValue);                                /* 함수 반환 값 stack에 저장 */

    Pc       = save_Pc;                                    /* 함수 호출 전의 환경으로 복원 */
    baseReg  = save_baseReg;
    stpReg    = save_spReg;
    code_ptr = save_code_ptr;
    code     = save_code;
    returnValue.init(save_returnType);

    blkNest = save_blkNest;
    cout << endl << " ========== 복구 : " << blkNest;
}

/** 내장함수 실행 */
void 
MareExecuter::execSysFunc() 
{
    CodeSet cs = code;
    short p;
    string s;
    VarObj v;
    vector<VarObj> vs;

    switch (cs.kind) {
    case ToArray:
        {
        code = nextCode();
        code = nextCode();                              /* toArray, '(' 건너뜀 */
        DtType tmpTp = symTablePt(code)->dtTyp;         /* 변수 타입 : 순서에 주의 (getMemAdrs보다 먼저) */
        bool isArray;
        int varAdrs = getMemAdrs(code, isArray);
        v.init(tmpTp);
        while (code.kind == ','){                       /* ',' 라면 인수가 계속됨 */  
            code = nextCode();
            v = getExpression(0, 0);
            DynamicMem.set(varAdrs++, v);
        }
        code = nextCode();
        }
        break;
    case Exit:
        JLOG(mutil.j_.trace()) << " ** exit func start ";
        exitFlag = true;                                /* exit 플래그를 셋팅 */
        mutil.setLedgerLog("Exit");                     /* exit 메세지를 log에 저장 */
    case Log:
        JLOG(mutil.j_.trace()) << " ** log message - start ";
        s.clear();
        code = nextCode();                              /* print, '(' 건너뜀 */
        do {
            code = nextCode();
            if (code.kind == String) {
                s.append(code.text);
                code = nextCode();
            }
            else {
                v.init(NON_T);                          /* 반복 사용될 경우, 타입 충돌 문제 방지 */
                v = getExpression();                    /* 함수 내에서 exit 가능성이 있음 */
                if (!exitFlag) { s.append(v.toStr(true)); }
            }
        } while (code.kind == ',');                     /* ',' 라면 인수가 계속됨 */
        
        JLOG(mutil.j_.debug()) << "   " << s;
        s.append("\n");
        mutil.appendLedgerLog(s);
        JLOG(mutil.j_.trace()) << " ** log message - end ";
        break;
    case Math: 
        p = setParams(vs, 2);
        if (cs.symIdx == Ceil) {
            mstk.push(vs[0].getType(), ceil(vs[0].getDbl()));
        }
        else if (cs.symIdx == Floor) {
            mstk.push(vs[0].getType(), floor(vs[0].getDbl()));
        }
        else if (cs.symIdx == Abs) {
            mstk.push(vs[0].getType(), abs(vs[0].getDbl()));
        }
        else if (cs.symIdx == Pow) {
            mstk.push(vs[0].getType(), pow(vs[0].getDbl(), vs[1].getDbl()));
        }
        else if (cs.symIdx == Sqrt) {
            mstk.push(vs[0].getType(), sqrt(vs[0].getDbl()));
        }
        else if (cs.symIdx == Round) {
            mstk.push(vs[0].getType(), round(vs[0].getDbl()));
        }
        else {
            errorExit(tecNO_FUNC, "function is not defined");
        }
        break;
    default: // MARE
        JLOG(mutil.j_.error()) << " [execSysFunc] skip: " << kind2Str(cs.kind);
        return;
    }
}

/** 함수의 파라미터 처리 (params가 numOfParams보다 많으면 에러) */
short 
MareExecuter::setParams(vector<VarObj>& vs, short numOfParams) {
    
    --numOfParams;                            /* 마지막 parameter는 따로 처리 */
    vs.clear();
    code = nextCode();
    code = chkNextCode(code, '(');
    short i;
    for (i=0; i<numOfParams; i++) {
        addDbgCode(Expression, GET_ARGUMENT_TYPE, i);
        vs.push_back(getExpression(0, 0));    /* Params 값 저장 */
        removeDbgCode();
        if (code.kind == ')') { 
            code = nextCode();
            return vs.size(); 
        }
        code = chkNextCode(code, ',');
    }
    addDbgCode(Expression, GET_ARGUMENT_TYPE, i);
    vs.push_back(getExpression(0, ')'));      /* 마지막 값 저장 (Params 최대 개수 확인) */
    removeDbgCode();
    return vs.size();
}

/** 
 * 단순 변수 또는 배열 요소의 주소를 반환. 
 * 배열인지 확인하기 위해 내부적으로 nextCode()를 호출함 */
int 
MareExecuter::getMemAdrs(CodeSet const& cd, bool& isDataObj)
{
    int adr=0, len;
    isDataObj = false;
    adr = getTopAdrs(cd);
    len = symTablePt(cd)->aryLen;
    code = nextCode();
    if (len == 0) return adr;                     /* 변수가 배열이 아닌 경우 */

    if (code.kind != '[') {                       /* 배열의 첨자가 없을 경우(첫항목을 임시로) */
        if (len > 0) isDataObj = true;            /* 배열 자체를 지정 */
         return adr;
    }

    double d = getExpression('[', ']').getDbl(); 
    if ((int)d != d) errorExit(tecNEED_UNSIGNED_INTEGER, "The index value of array must be a positive integer only.");

    int index = (int) d;
    if (index < 0 || len <= index)
        errorExit(tecEXCEED_ARRAY_LENGTH, "The index value of the array is out of range.");

    return adr + index;		                     /* 배열의 첨자만큼 더함 */
}

/** 변수의 시작 주소를 반환 (배열인 경우는 0번째) */
int 
MareExecuter::getTopAdrs(CodeSet const& cd)
{
    switch (cd.kind) {
        case Gvar: return symTablePt(cd)->adrs;		      /* 글로벌 변수 */
        case Lvar: return symTablePt(cd)->adrs + baseReg; /* 로컬 변수   */ 
        default: 
            errorExit(tecNEED_VARIABLE_TYPE, "Variable is required.");
            return 0;
    }
}

/** if 문에 대응하는 end의 위치 반환 */
short 
MareExecuter::getEndLineOfIf(short line_) 
{
    CodeSet cd;
    char *save = code_ptr;

    cd = firstCode(line_);
    for (;;) {
        line_ = cd.jmpAdrs;

        cd = firstCode(line_);
        if (cd.kind == Close) cd = firstCode(line_+1); /* Close 다음 라인의 시작 값을 확인해야 함. */

        if (cd.kind ==Elif || cd.kind==Else) continue; /* 시작코드가 else if거나 else인지 확인 */
        if (cd.kind == End) break;
    }
    code_ptr = save;
    return line_;
}

/** 코드가 line의 끝인지(다른 코드가 더이상 없는지) 여부를 확인 */
void 
MareExecuter::chkEofLine() 
{
    if (code.kind != EofLine) 
        errorExit(tecNEED_LINE_END, "The line must end.");
}

/** line_행의 시작 코드 정보를 확인 */
TknKind 
MareExecuter::lookCode(short line_) 
{
    // 1st : INTERCODE_LEN
    return (TknKind)(unsigned char)intenalCode[line_][1];
}

/** 
 * 확인부 코드 획득
 * 현재 코드 type이 예상되는 값인지를 확인 후, 다음 코드 반환함 */
CodeSet 
MareExecuter::chkNextCode(CodeSet const& cd, short kind2) 
{
    if (cd.kind != kind2) {
        if (cd.kind == Do || cd.kind == Close){
            CodeSet cd1 = nextCode();
            return chkNextCode(cd1, kind2);
        }
        if (kind2   == EofLine) errorExit(tecNEED_LINE_END, "The line must end.");
        if (cd.kind == EofLine) errorExit(tecINCORRECT_SYNTAX, kind2Str((TknKind)kind2), " code is required.");
        errorExit(tecINCORRECT_SYNTAX, kind2Str((TknKind)kind2) + " must be in position ", kind2Str(cd.kind));
    }
    return nextCode();
}

/** 시작 코드 획득 (line의 첫 코드를 반환) */
CodeSet 
MareExecuter::firstCode(short line_)   			
{
    if (++readedCodeCnt > MAX_EXE_LINE) 
        errorExit(tecEXCEED_CODE_LINES, "too much code lines:", to_string(readedCodeCnt));

    code_ptr = intenalCode[line_];      /* 분석용 포인터를 행의 시작으로 설정 */
    JLOG(mutil.j_.debug()) << debugInterCode(code_ptr, line_);
    code_ptr++;                         /* skip INTERCODE_LEN */
    return nextCode();
}

/** 코드 획득 (현재 line의 다음 코드를 반환) */
CodeSet 
MareExecuter::nextCode() 						
{
    TknKind kd;
    short int jmpAdrs, tblIdx;

    if (*code_ptr == '\0') {                     /* 현재 line에서 코드가 끝난 경우 */
        return CodeSet(EofLine);
    }
    kd = (TknKind)*UCHAR_P(code_ptr++);          /* 분석용 포인터를 다음 위치로 설정해서 코드 값을 읽음 */

    JLOG(mutil.j_.trace()) << " * nextCode: " << kind2Str(kd);
    switch (kd) {
    case Func:
    case While: 
    case For:
    case Foreach:
    case If: 
    case Elif: 
    case Else:
        jmpAdrs = *SHORT_P(code_ptr); code_ptr += SHORT_SIZE;
        return CodeSet(kd, -1, jmpAdrs);                   /* 수행후, 점프할 주소를 추가 */
    case String:
        tblIdx = *SHORT_P(code_ptr); code_ptr += SHORT_SIZE;
        return CodeSet(kd, strLITERAL[tblIdx].c_str());    /* 문자열 리터럴에서 값을 가져와 추가 */ 
    case IntNum: 
    case DblNum:						
        tblIdx = *SHORT_P(code_ptr); code_ptr += SHORT_SIZE;
        return CodeSet(kd, nbrLITERAL[tblIdx]);            /* 수치 리터럴에서 값을 가져와 추가 */           
    case Fcall: 
    case Gvar: 
    case Lvar:
    case System:
    case Math:
    case Throws:
        tblIdx = *SHORT_P(code_ptr); code_ptr += SHORT_SIZE;
        return CodeSet(kd, tblIdx, -1);                    /* 호출 대상이 위치한 주소를 추가 */
    default:
        return CodeSet(kd);                                /* 부속 정보가 없는 코드 */
    }
}

/** 배열 초기화 */
void 
MareExecuter::setInitArray() 
{
    int memAdrs = getTopAdrs(code);
    vector<SymTbl>::iterator p = symTablePt(code);
    if (p->aryLen == 0) errorExit(tecNEED_ARRAY_TYPE);
    DtType typ = p->dtTyp;

    // 배열인 경우, 초기화 수행
    if (typ == INT_T || typ == DBL_T) {             // 숫자형 배열이면 내용을 0으로 초기화
        for (int n=0; n < p->aryLen; n++)
            DynamicMem.set(memAdrs+n, VarObj(typ, 0));
    }
    else {
        string initVal;
        if (typ == STR_T) initVal = "";
        else errorExit(tecINVALID_TYPE, "The type cannot be used in an array.");

        for (int n=0; n < p->aryLen; n++)
            DynamicMem.set(memAdrs+n, VarObj(typ, initVal));
    }
}

}