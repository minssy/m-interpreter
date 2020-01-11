
#include "MareInterpreter.h"

namespace mare_vm {

using namespace std;    

Blob
MareInterpreter::getFuncInfos() {
    JLOG(mutil.j_.trace()) << " * funcInfos size: " << funcInfos.size();
    return toBlob(funcInfos);
}

/** 코드 변환 시작 */
void 
MareInterpreter::convertToInterCode(vector<string> &src)
{
    string errTypeStr = "parsing script error";
    try {
        convertAll(src);
        errTypeStr = "verify syntax error";
        chkSyntax();
        errTypeStr = "initial execute error";
        execute(1);
        errTypeStr = "reorganize code error";
        updateCode();
    }
    catch(ErrObj err) {
        throw err;
    }
    catch(TECcodes err_code) {
        errorExit(err_code, errTypeStr, "(tec code)");
    }
    catch(int err_code) {
        errorExit(err_code, errTypeStr, "(Error code)");
    }
    catch(const char* err_msg) {
        errorExit(tecUNKNOWN_CONTRACT_ERROR, err_msg);
    }
    // catch(std::string err_msg) {  // 예외를 catch하지 못함??
    //     JLOG(mutil.j_.error()) << "[Contract Exception String] " << err_msg
    //                            << " -Line: " << getLineNo();
    //     return tecFAILED_PROCESSING;
    // }
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

void 
MareInterpreter::convertAll(vector<string> &src)
{
    Pc = -1;
    endOfFileFlag = false;
    srcMaxLineNo = src.size();
    srcWork = src;

    if (srcMaxLineNo > MAX_LINE)
        errorExit(tecEXCEED_CODE_LINES, "프로그램의 허용 코드수 초과");

    srcNew.clear();
    funcInfos.clear();
    srcLineNo = 0;
    
    blkNest = loopNest = 0;
    funcDeclFlag = false;
    codeBuf_p = codeBuf;

    string strInfo;
    size_t idx;
    // 함수 이름만 먼저 등록 
    while (token=nextLineTkn(), token.kind != EofProg) {

        switch (token.kind) {

            case Func:
                token = nextTkn();                /* return type */
                token = nextTkn();                /* func name */
                setSymName(Void);                 /* 임시값으로 저장 */
                   
                enter(tmpTb, funcId);             /* 함수 인 경우 */

                srcNew.push_back(srcWork[srcLineNo-1]);
                strInfo = trimCopy(srcWork[srcLineNo-1]);

                if (strInfo.at(4) == ' ') strInfo.erase(0, 5); // func
                else strInfo.erase(0, 9);                      // function
                idx = strInfo.find('{', 10);

                if (idx != string::npos)
                    funcInfos.push_back(strInfo.substr(0, idx));
                else funcInfos.push_back(strInfo);

                break;
            case For:     /* for문의 초기값, 검증, 증감조건 순서를 실행시 유리하도록 변경 */
                {

                strInfo = srcWork[srcLineNo-1];
                size_t idx1 = strInfo.find_first_of(';');
                if (idx1 == string::npos) errorExit(tecINCORRECT_SYNTAX, "wrong for statement (semicolon not found)");
                string newInfo = strInfo.substr(0, idx1);
                size_t idx2 = strInfo.find_last_of(';');
                size_t idxx = strInfo.find_first_of(';', idx1+1);
                if (idx1 == idx2) errorExit(tecINCORRECT_SYNTAX, "wrong for statement(one semicolon found)");
                if (idx2 != idxx) errorExit(tecINCORRECT_SYNTAX, "wrong for statement(3 or more semicolons found)");
                idx = strInfo.find_last_of(')');
                newInfo += strInfo.substr(idx2, (idx -idx2));
                newInfo += strInfo.substr(idx1, (idx2-idx1));
                newInfo += strInfo.substr(idx);
                srcNew.push_back(newInfo);
                }
                break;

            default:
                srcNew.push_back(srcWork[srcLineNo-1]);
                break;
        } // switch

    } // while

    printInfos(); // 디버깅용 

    if (funcInfos.size() == 0) {
        JLOG(mutil.j_.warn()) << "호출 가능한 함수가 없습니다.";
    }

    // 내부 코드로 변환 시작
    /* 0번째 행은 필요 없으므로 스크립트 버전으로 채움. (1행부터 시작하도록) */
    setCode(Version, MAJOR_VERSION); setCode(Version, MINER_VERSION);
    pushInternalCode(); 

    srcWork.clear();
    srcWork = srcNew;
    
    srcLineNo = 0;                  /* Ready to convert */
    endOfFileFlag = false;

    token = nextLineTkn();
    while (token.kind != EofProg) {
        convert();                   /* 내부 코드로 변환 */
    }

    pushInternalCode();
}

/** 
 * line의 맨 앞에 출현하는 코드를 처리. 
 * 나머지 부분은 convertForRest()등에서 처리됨 */
void 
MareInterpreter::convert()
{
    JLOG(mutil.j_.trace()) << "* convert: " << kind2Str(token.kind);
    switch (token.kind) {

    case VarInt:
    case VarDbl:
    case VarStr:
    case VarDateTime:
        varDeclare(token.kind);          /* 변수 선언 처리 */
        break;
    case Func:
        chkDo = true;                    /* 블록 시작 체크를 위한 플래그 설정 */
        funcDeclare();                   /* 함수 선언 처리 */
        break;
    case For:
    case Foreach:
    case While:
        ++loopNest;
        chkDo = true;                    /* 블록 시작 체크를 위한 플래그 설정 */
        convertBlockSet();               /* 블록 내부 처리 */
        setCodeEnd();                    /* 블록 종료 처리 */
        --loopNest;
        break;
    case If:
        chkDo = true;                    /* 블록 시작 체크를 위한 플래그 설정 */
        convertBlockSetIf();             /* if 조건식 및 내부 블록 처리 */

        while (token.kind == Else) {     /* else of 거나 else 인 경우가 있을 수 있음 */
            chkDo = true;                /* 블록 시작 체크를 위한 플래그 설정 */
            Token cc = nextTkn();        /* 다음이 if인지 '{'인지 등에 따른 처리를 위해, 임시 변수로 저장 */
            if (cc.kind == If) {         /* else if 인 경우 처리 */
                token = Elif;
                convertBlockSetIf();     /* else if 조건식 및 내부 블록 처리 */
            }
            else if (cc.kind == Do) {    /* '{'로 시작하는 경우 처리 */
                chkDo = false;           /* 블록 시작 체크 완료됨 */
                break;
            }
            else if (cc.kind == EofLine) break;
            else errorExit(tecINCORRECT_SYNTAX, "wrong else if format");
        }
        if (token.kind == Else){         /* else 인 경우 처리 */
            convertBlockSet();           /* 블록 내부 처리 */
            setCodeEnd();                /* 블록 종료 처리 */
        }
        break;
    case Break:
    case Continue:
        if (loopNest <= 0) 
            errorExit(tecINCORRECT_SYNTAX, "잘못된 break/continue입니다.");
        setCode(token.kind); 
        token = nextTkn(); 
        convertForRest();
        break;
    case Return:
        if (!funcDeclFlag) 
            errorExit(tecINCORRECT_SYNTAX, "잘못된 return입니다.");
        setCode(token.kind); 
        token = nextTkn(); 
        convertForRest();
        break;
    case Throws:
        token = nextTkn();
        if (token.kind == IntNum) {
            if (token.numVal < 1) 
                errorExit(tecINCORRECT_SYNTAX, "wrong error code");
            
            setCode(Throws, token.numVal);
        }
        else if (token.kind == Ident && token.text.find("tec") == 0){
            auto x = transCode(token.text);
            setCode(Throws, x);
        }
        else errorExit(tecINCORRECT_SYNTAX, "잘못된 throw 형식입니다."); 
        token = nextTkn();
        convertForRest();
        break;
    case Require:
    case Exit:
    case Log:
    case ToArray:
        setCode(token.kind);
        token = nextTkn();
        convertForRest();
        break;
    case System:
    case Math:
    case Do:
    case Close:
    case End:
    case Size: // case Length: 
    case Find:
    case ToString: 
        // 단독으로 사용할 수 없는 경우
        errorExit(tecINCORRECT_SYNTAX, "잘못된 " + kind2Str(token.kind) + " 위치입니다."); 
        break;
    default: convertForRest(); 
        break;
    }
}

/** 코드 라인의 중간에 올수 없는 키워드인지 확인 */
void 
MareInterpreter::chkKinds(Token const& tk) {

    switch (tk.kind) {
        case VarInt:     case VarDbl:    case VarStr:
        case VarDateTime:

        case Func:       case End:
        case If:         case Elif:      case Else: 
        case For:        case Foreach:   case While:     
        case Break:      case Continue:  case Return:

        case Require:    case Exit:      case Throws:
        case Log:        
        case ToArray:
            /* 이 키워드가 도중에 나타나는 일은 없음 */
            throw tecINCORRECT_SYNTAX;
        default: 
            break;
    }
}

/** 문의 나머지 부분 처리 */
void 
MareInterpreter::convertForRest() 
{
    for (;;) {
        JLOG(mutil.j_.trace()) << " convertForRest() -token:" << kind2Str(token.kind);

        if (token.kind == EofLine) break;
        
        chkKinds(token);

        switch (token.kind) {

        case Ident:                                              /* 함수 호출, 혹은 변수 */
            short tblIdx;
            if ((tblIdx=searchName(token.text, 'G')) != -1) {    /* Global symbol로 등록되어 있는지 확인 */
                if (Gtable[tblIdx].symKind==funcId)
                    setCode(Fcall, tblIdx); 
                else
                    setCode(Gvar, tblIdx);
            }
            else if ((tblIdx=searchName(token.text, 'L')) == -1) /* Local symbol로 등록되어 있는지 확인 */
                errorExit(tecNO_VARIABLE, "This token is not declared.");
            else
                setCode(Lvar, tblIdx);

            token = nextTkn();
            if (token.kind != Dot) continue;
            setCode(Dot);
            token = nextTkn();
            TknKind Property;
            tblIdx = mutil.getPropertyIdx(token.text, Property);
            setCode(Property, tblIdx);
            break;
        case IntNum: 
        case DblNum:
            setCode(token.kind, setLITERAL(token.numVal));       /* 정수도 double형으로 저장 */
            break;
        case String:
            setCode(token.kind, setLITERAL(token.text));
            break;
        case System:
        case Math:
        {
            TknKind tp = token.kind;
            token = nextTkn(); token = chkNextTkn(token, '.');
            setCode(tp, mutil.getIdx(tp, token.text));
            break;
        }
        case Do:
            if (chkDo) { chkDo = false; } /* 블록의 시작이 '{'인 경우 */
            else errorExit(tecINCORRECT_SYNTAX, "wrong curly brackets");
            break;
        default:                                                 /* '+', '-', '<=' 등 처리 */
            setCode(token.kind);
            break;
        }
        token = nextTkn();
    } 
    JLOG(mutil.j_.trace()) << "convertForRest():save";
    pushInternalCode();
    token = nextLineTkn();
}

/** 
 * 변수 선언과 동시에 Assign(=)이 있을 경우, 문의 나머지 처리
 * (함수 파라미터의 초기값이 정의된 경우, literalOnly=true) */
void 
MareInterpreter::convertVarAssign(bool literalOnly) 
{
    bool inBracket = false;
    short bracketLevel = 0;
    for (;;) {
        JLOG(mutil.j_.trace()) << " convertVarAssign() subAssign:" << kind2Str(token.kind);

        if (literalOnly) {
            if (token.kind == Comma || token.kind == ')') return;
        }
        if (token.kind == EofLine) {
            if (inBracket) errorExit(tecINCORRECT_BLACKET);
            return; 
        }
        else if (token.kind == Comma && !inBracket) return;
        else if (token.kind == ')') {
            --bracketLevel;
            if (bracketLevel == 0) inBracket = false;
            if (bracketLevel < 0) errorExit(tecINCORRECT_BLACKET);
        }
        else 
            chkKinds(token);

        switch (token.kind) {

        case Do:         case Close: 
            errorExit(tecINCORRECT_SYNTAX);
            break;
        case Ident:                                                /* 함수 호출, 변수 */
            if (literalOnly) errorExit(tecNEED_LITERAL_TYPE, 
                "Only literal values are allowed to initialize function arguments.");
            short tblIdx;
            if ((tblIdx=searchName(token.text, 'G')) != -1) {      /* Global symbol로 등록되어 있는지 확인 */
                if (Gtable[tblIdx].symKind==funcId)
                    setCode(Fcall, tblIdx); 
                else
                    setCode(Gvar, tblIdx);
            }
            else if ((tblIdx=searchName(token.text, 'L')) == -1)   /* Local symbol로 등록되어 있는지 확인 */
                errorExit(tecNO_VARIABLE, "This token is not declared.");
            else
                setCode(Lvar, tblIdx);

            token = nextTkn();
            if (token.kind != Dot) continue;
            setCode(Dot);
            token = nextTkn();
            TknKind Property;
            tblIdx = mutil.getPropertyIdx(token.text, Property);
            setCode(Property, tblIdx);
            break;
        case IntNum:  case DblNum:                                 /* 정수도 double형으로 저장 */
            setCode(token.kind, setLITERAL(token.numVal));
            break;
        case String:
            setCode(token.kind, setLITERAL(token.text));
            break;
        case System:
        {
            TknKind tp = token.kind;
            token = nextTkn(); token = chkNextTkn(token, '.');
            setCode(tp, mutil.getIdx(tp, token.text));
            break;
        }
        case Math:
        {
            if (literalOnly) errorExit(tecNEED_LITERAL_TYPE, 
                //"함수 인자 초기값에는 리터럴 값만 가능합니다.",
                "Only literal values are allowed to initialize function arguments.");
            TknKind tp = token.kind;
            token = nextTkn(); token = chkNextTkn(token, '.');
            setCode(tp, mutil.getIdx(tp, token.text));
            break;
        }
        case True:  case False:
            setCode(token.kind);
            break;
        default:                                                   /* '+', '-', '<=' 등 처리 */
            if (literalOnly) errorExit(tecNEED_LITERAL_TYPE, 
                "Only literal values are allowed to initialize function arguments.");
            setCode(token.kind);
            if (token.kind == '(') { inBracket = true; ++bracketLevel; }
            break;
        }
        token = nextTkn();
    }
}

/** 블록 처리 관리 (if, else if 제외) */
void 
MareInterpreter::convertBlockSet() 
{
    JLOG(mutil.j_.trace()) << " convertBlockSet() ... " << kind2Str(token.kind);
    short patch_line;
    TknKind kind = token.kind;
    patch_line = setCode(token.kind, NO_FIX_ADRS);   /* 블록의 끝 line 정보를 임시 값으로 할당 */
    token = nextTkn();
    convertForRest();                                /* 조건문 처리 */
    convertBlock();                                  /* 실제 블록 처리 */
    JLOG(mutil.j_.trace()) << " end block:" << kind2Str(kind) << " s_line:" << patch_line
         << " n_line:" << getLineNo();
    backPatch(patch_line, getLineNo());              /* 블록의 끝 line 정보를 수정 (end행 번호) */
}

/** if, else if 블록 처리 관리 (마지막에 else로 종료되지 않으면, End code를 추가) */
void 
MareInterpreter::convertBlockSetIf() 
{

    convertBlockSet();                              /* 조건문 및 블록 처리 */
    token = nextTkn();
    if (token.kind == EofLine){                     /* 블록 끝난 후, 처리를 위한 다음 line의 토큰 확인 */
        token = nextLineTkn();
        if (token.kind != Else) setCode(End);       /* if 관련 종료를 위해서는 블록 끝에 end를 삽입해야 됨. */
        else setCode(Close);                        /* Close처리를 EofLine로 변경가능하나 실행 및 검증에 대한 수정 필요함. */
        
        pushInternalCode();
    } 
    else if (token.kind != Else){
        errorExit(tecINCORRECT_SYNTAX, "잘못된 형식입니다. (else 혹은 라인의 끝이어야 합니다.)", kind2Str(token.kind) );
    }
}

/** 블록 내부 처리 (블록의 끝까지 문을 분석) */
void 
MareInterpreter::convertBlock() 
{
    TknKind k;
    ++blkNest;                                      /* 블록 내부로 들어옴에 따른 블록 레벨값 증가 */

    if (chkDo){                                     /* '{' 로 시작하는지 확인 필요함. */
        if (token.kind == Do){                      /* '{' 로 시작하는 경우 처리 */
            token = nextTkn();                      
            chkDo = false;
            if (token.kind == EofLine){             /* 현재 line에 '{'만 존재하는 경우 처리. */
                //setCode(Do);                      /* 굳이 '{'를 추가할 필요가 없을 듯 */
                convertForRest();                   /* 현재 line 저장 및 다음 line으로 전환 */
            }
        }
        else errorExit(tecNEED_OPEN_BLOCK, "블록 열기 에러: ", token.text);
    }

    while(k=token.kind, k!=Close) {                 /* '}'가 나올때까지 블록 내부 처리. */
        JLOG(mutil.j_.trace()) << " ----- lvl[" << to_string(blkNest) << "] block line start ";
        if (k==EofProg || k==Elif || k==Else || k==End || k==Func){
            errorExit(tecNEED_CLOSE_BLOCK, "블록 닫음 에러: ", token.text); 
        }
        convert();                                  /* 블록 내부의 명령행들을 처리 */
    }
    --blkNest;                                      /* 블록이 끝남에 따른 블록 레벨값 감소 */
}


/** 변수 선언 처리 */
void 
MareInterpreter::varDeclare(short const varType) 
{
    for (;;) {
        token = nextTkn();
        JLOG(mutil.j_.trace()) << " * new variable:" << kind2Str((TknKind)varType) << " " << token.text;
        chkVarName(token);                     /* 이름 검사 */
        setSymName(varType);                   /* 변수 등록에 사용될 SymTbl 셋팅 */
        setSymAryLen();                        /* 배열이면 길이 정보 설정 */
        short tblNb = enter(tmpTb, varId);     /* 변수등록 (주소도 등록) */

        if (tmpTb.aryLen > 1) {
            if (token.kind == '=') errorExit(tecINVALID_ASSIGN, "Array cannot be initialized by assignment.");
            setCode(DeclareArr);
        }
        else setCode(DeclareVar);

        if (isLocalVarName(tmpTb.name, varId))
            setCode(Lvar, tblNb);
        else 
            setCode(Gvar, tblNb);

        if (token.kind == '=') {               /* 선언 후, 초기값이 할당되는지 여부에 따른 처리 */
            setCode('=');                      /* '=' (Assign) 코드 추가 */
            token = nextTkn();
            JLOG(mutil.j_.trace()) << " var Assign(=) " << kind2Str(token.kind) << ":" << token.text;

            convertVarAssign();                /* 할당할 값 관련 내용 추가 */
        }
        if (token.kind != ',') break;          /* ',' 이면, 인수가 계속됨 */
        setCode(',');
    }

    setCodeEofLine();
}


/** 새로운 변수 등록을 위한 이름 확인 */
void 
MareInterpreter::chkVarName(Token const& tk) 
{
    if (tk.kind != Ident) 
        errorExit(tecINVALID_NAME, "Duplicated identifier: ", tk.text);

    //if (tk.text[0] != '$')
    //    errorExit("변수 명은 $로 시작해야 합니다: ", tk.text);

    char chkTp = isLocalScope() ? 'V' : 'A';        
    if (searchName(tk.text, chkTp) != -1)
            errorExit(tecINVALID_NAME, "Duplicated variable name: ", tk.text);
}

/** 
 * 등록할 SymTbl의 이름 및 PermissionType, DataType 설정 (함수일 경우, 리턴 타입) 
 * 현재토큰이 다음토큰으로 변경됨 */
void 
MareInterpreter::setSymName(short const dtType) 
{
    if (token.kind != Ident) 
        errorExit(tecINVALID_NAME, "Duplicated identifier: ", token.text);
    
    if (dtType < Void || dtType > VarDateTime)
        errorExit(tecINCORRECT_SYNTAX, "Duplicated type identifier: ", kind2Str((TknKind)dtType));

    tmpTb.clear(); 
    tmpTb.name = token.text;                   /* 이름 설정 */
    tmpTb.dtTyp = (DtType)dtType;              /* 타입 설정 (함수일 경우, 리턴 타입) */

    token = nextTkn();
}

/** 배열 크기 설정 */
void 
MareInterpreter::setSymAryLen() 
{
    tmpTb.aryLen = 0;
    if (token.kind != '[') return;             /* 배열이 아닌 경우 */

    token = nextTkn();
    if (token.kind != IntNum)
        errorExit(tecNEED_UNSIGNED_INTEGER, "The index value of array must be a positive integer only.: ", token.text);

    tmpTb.aryLen = (int)token.numVal;          /* int a[5]는 첨자 0~4가 유효함 */
    if (MAX_ARRAY < tmpTb.aryLen)
        errorExit(tecEXCEED_ARRAY_LENGTH, "The size of the array has been exceeded.");

    token = chkNextTkn(nextTkn(), ']');
    if (token.kind == '[') 
        errorExit(tecBAD_MULTI_DIMENTION_ARRAY, "Multi-dimensional arrays cannot be declared.");
}

/** 함수 정의 */
void 
MareInterpreter::funcDeclare() 
{
    int tblIdx, patch_line, fncTblIdx;
    
    if (blkNest > 0 || funcDeclFlag) 
        errorExit(tecINCORRECT_SYNTAX, "Incorrect function declaration position.");

    funcDeclFlag = true;                               /* 함수 처리 시작 확인 플래그 */
    localAdrs = 0;                                     /* 로컬 영역 할당 카운터 초기화 */
    setStartLtable();                                  /* 로컬 심볼 테이블의 시작 위치 설정 */
    patch_line = setCode(Func, NO_FIX_ADRS);           /* 나중에 블록의 끝(end)의 행 번호를 넣기 위해, 임시 값 할당으로 공간 확보 */

    token = nextTkn();                                 /* return type */
    short funcRtnTp = token.kind;                      /* return type 정보 보관 */
    token = nextTkn();                                 /* func name */
    fncTblIdx = searchName(token.text, 'F');           /* 함수명은 이미 등록되어 있기 때문에 함수명 검색을 함 */
    
    Gtable[fncTblIdx].dtTyp = (DtType)funcRtnTp;       /* 함수의 리턴 타입 설정 */

    // 주석과 같이 불필요한 코드가 제거된 경우...
    Gtable[fncTblIdx].adrs = getLineNo();              /* 함수의 주소 업데이트 */
    
    bool paramAssign = false;                          /* parameter Assign 순서 검증용 */ 
    // 가인수 분석
    token = nextTkn(); // '('
    token = chkNextTkn(token, '(');                    /* '(' 로 시작하는지 확인 */
    setCode('(');
    if (token.kind != ')') {                           /* 인수가 있는지 여부 확인 */
        short minArgs = 0;
        for (;; token = nextTkn()) {
            int ttk = token.kind;                      /* param type */
            token = nextTkn();                         /* param name */
            setSymName(ttk);
            setSymAryLen();                            /* 배열이면 길이 정보 설정 */
            tblIdx = enter(tmpTb, paraId);             /* 인수 등록 */
            setCode(Lvar, tblIdx);                     /* 인수는 Lvar로서 처리됨 */
            if (tmpTb.aryLen == 0)
                ++Gtable[fncTblIdx].args;              /* 인수 개수를 +1 */
            else 
                errorExit(tecINVALID_FUNC_ARGUMENT, "function param type");
            
            if (token.kind == '=') {                   /* '=' 이면, 함수 파라미터 초기화 처리 */
                setCode('=');                          /* '=' (Assign) 코드 추가 */
                token = nextTkn();
                JLOG(mutil.j_.trace()) << " var Assign(=) " << kind2Str(token.kind) << ":" << token.text;
                convertVarAssign(true);                /* 할당할 값 관련 내용 추가 */
                minArgs++;
                paramAssign = true;
            }
            else if (paramAssign) 
                errorExit(tecINVALID_FUNC_ARGUMENT, "wrong function param Assign(=)");

            if (token.kind != ',') break;              /* ',' 이면, 인수가 계속됨 */
            setCode(',');
        }
        if (paramAssign)                               /* 최소 인수 개수 */
            Gtable[fncTblIdx].aryLen = Gtable[fncTblIdx].args - minArgs;
        else Gtable[fncTblIdx].aryLen = Gtable[fncTblIdx].args;
    }
    token = chkNextTkn(token, ')');                    /* ')' 로 끝나는지 확인 */
    setCode(')'); 
    setCodeEofLine(true);                              /* '{'가 있는 경우도 처리됨 */
    convertBlock();                                    /* 함수 본체 처리 */

    backPatch(patch_line, getLineNo());                /* 블록의 끝으로 jump 할수있도록 블록의 끝(end)의 행 번호로 수정 */
    setCodeEnd();
    Gtable[fncTblIdx].frame = localAdrs;               /* 프레임 크기 (함수에서 선언된 로컬 변수 개수) */

    funcDeclFlag = false;                              /* 함수 처리 종료 */
}

/** line행에 n을 설정 */
void 
MareInterpreter::backPatch(short line, short endLine) 
{
    *SHORT_P(intenalCode[line] + 2) = (short)endLine; // skip INTERCODE_LEN (+1)
}

/** end 인지 확인하고, 저장 처리 */
void 
MareInterpreter::setCodeEnd() 
{
    JLOG(mutil.j_.trace()) << "setCodeEnd() : " << kind2Str(token.kind);
    if (token.kind != End && token.kind != Close) 
        errorExit(tecNEED_CLOSE_BLOCK, "'}' 가 ", token.text + " 앞에 필요합니다.");

    setCode(End); 
    token = nextTkn(); 
    setCodeEofLine();
}

/** 
 * line의 끝인지 확인하고, Code 저장 처리 
 * (DoEnable이 true일 경우, '{'로 끝나는 경우에 대해서도 처리 가능}) */
void 
MareInterpreter::setCodeEofLine(bool DoEnable) 
{
    JLOG(mutil.j_.trace()) << "setCodeEofLine(" << DoEnable << ") : " << kind2Str(token.kind);
    if (DoEnable && token.kind == Do){
        //setCode(Do);  Do는 실제로 사용 안되므로 생략함
        chkDo = false;
        token = nextTkn();
    }
    if (token.kind != EofLine) errorExit(tecNEED_LINE_END, "Require the end of the line.");

    pushInternalCode();
    token = nextLineTkn();                 /* 다음 행으로 진행 */
}

/** 함수 내부 블록의 코드를 처리 중인지 확인 */
bool 
MareInterpreter::isLocalScope() 
{
    return funcDeclFlag;
}

/** 다음 행을 읽고, 다음 행의 첫 토큰을 반환 */
Token 
MareInterpreter::nextLineTkn() 
{
    if (endOfFileFlag) 
        errorExit(tecUNKNOWN_CONTRACT_ERROR, "Code is the end.");

    if (srcMaxLineNo <= srcLineNo) 
        endOfFileFlag = true;
    
    if (!endOfFileFlag) {
        strcpy(tmpBuf, srcWork[srcLineNo++].c_str());

        if (strlen(tmpBuf) > LINE_SIZE)
            errorExit(tecEXCEED_CODE_LENGTH, "Max line length:", to_string(LINE_SIZE));
    
        token_p = tmpBuf;
    }

    JLOG(mutil.j_.debug()) << srcLineNo << " line: " << token_p;

    return nextTkn();
}

#define CH (*token_p)
#define CHC (unsigned char)(*token_p)
#define C2 (*(token_p+1))
#define CX(x) (*(token_p+x))
#define NEXT_CH()  ++token_p

/** 다음 토큰을 반환 */
Token 
MareInterpreter::nextTkn() 
{
    TknKind kd;
    string txt = "";

    if (endOfFileFlag) return Token(EofProg);                  /* 파일 종료 */
    while (isspace(CH)) NEXT_CH();                             /* 공백 스킵 */
    if (CH == '\0')  return Token(EofLine);                    /* 행의 끝   */
    
    JLOG(mutil.j_.trace()) << "  -> nextTkn(): '" << CHC << "' "
             << ((C2 == '\0') ? ' ' : C2);// << "' -> " << to_string(ctyp[CHC]);

    switch (ctyp[CHC]) {
    case DblQ:                                                 /* 문자열 상수 */
        NEXT_CH();
        while (CH!='"' && CH!='\0') { txt += CH; NEXT_CH(); }
        if (CH == '"') NEXT_CH(); else errorExit(tecNEED_CLOSE_LITERAL, "Need to close the string literal.");
        return Token(String, txt);
    case SglQ:
        NEXT_CH();
        while (CH!='\'' && CH!='\0') { txt += CH; NEXT_CH(); }
        if (CH == '\'') NEXT_CH(); else errorExit(tecNEED_CLOSE_LITERAL, "Need to close the string literal.");
        return Token(String, txt);
    case Digit:                                                /* 수치상수 */
        kd = IntNum;
        while (ctyp[CHC] == Digit) { txt += CH; NEXT_CH(); }
        if (CH == '.') { 
            kd = DblNum; txt += CH; NEXT_CH(); 
            if (ctyp[CHC] != Digit) errorExit(tecINCORRECT_LITERAL_FORMAT, "Need numeric only");
            while (ctyp[CHC] == Digit) { txt += CH; NEXT_CH(); } 
        }
        else if (ctyp[CHC] == Letter) {
            long timetmp = getTimeSpanOfStr(txt);
            return Token(kd, timetmp);                         /* IntNum도 double형으로 저장 */
        }
        return Token(kd, atof(txt.c_str()));                   /* IntNum도 double형으로 저장 */
    case Doll: 
    case Letter:
        txt += CH; NEXT_CH();
        while (ctyp[CHC] == Letter || ctyp[CHC] == Digit) { 
            txt += CH; NEXT_CH(); 
        }
        break;
    default:
        if (CH=='/' && C2=='/') return Token(EofLine);         /* 주석 제거 처리 */
        if (isOperator(CH, C2)) { txt += CH; txt += C2; NEXT_CH(); NEXT_CH(); }
        else                    { txt += CH; NEXT_CH(); }
    }

    auto m_iter = kindMap.find(txt);                           /* 키워드인지 확인 */
    if (m_iter != kindMap.end()){
        if (txt == "++" || txt == "--")
            if (token.kind == Ident && (isspace(CH) || (ctyp[CHC] != Letter && ctyp[CHC] != Doll))){
                if (m_iter->second == DBPlus) { return Token(DBPlusR, txt); }
                else if (m_iter->second == DBMinus) { return Token(DBMinusR, txt); }
            }
        return Token(m_iter->second, txt);
    }

    unsigned char chk = (unsigned char)txt[0];
    if (ctyp[chk] == Letter || ctyp[chk] == Doll) return Token(Ident, txt);
    else errorExit(tecINCORRECT_SYNTAX, "wrong token:", txt);
    return Token(EofProg);
}

/** 시간 상수 변환 */
long 
MareInterpreter::getTimeSpanOfStr(string const& d) {

    long val = atol(d.c_str());
    bool numType = (val == 1);

    if (CH == 'y' && C2 == 'e' && CX(2) == 'a' && CX(3) == 'r'){
        if (!chkTimeSpanOfStr(4, numType))
            errorExit(tecINCORRECT_TIME_FORMAT, "wrong years format");
        val *= 31536000; //365 * 24 * 60 * 60;
        return val;
    }
    if (CH == 'm' && C2 == 'o' && CX(2) == 'n' && CX(3) == 't' && CX(4) == 'h'){
        if (!chkTimeSpanOfStr(5, numType))
            errorExit(tecINCORRECT_TIME_FORMAT, "wrong months format");
        val *= 2592000; //30 * 24 * 60 * 60;
        return val;
    }
    if (CH == 'w' && C2 == 'e' && CX(2) == 'e' && CX(3) == 'k'){
        if (!chkTimeSpanOfStr(3, numType))
            errorExit(tecINCORRECT_TIME_FORMAT, "wrong days format");
        val *= 86400; //7 * 24 * 60 * 60;
        return val;
    }
    if (CH == 'd' && C2 == 'a' && CX(2) == 'y'){
        if (!chkTimeSpanOfStr(3, numType))
            errorExit(tecINCORRECT_TIME_FORMAT, "wrong days format");
        val *= 86400; //24 * 60 * 60;
        return val;
    }
    if (CH == 'h' && C2 == 'o' && CX(2) == 'u' && CX(3) == 'r'){
        if (!chkTimeSpanOfStr(4, numType))
            errorExit(tecINCORRECT_TIME_FORMAT, "wrong hours format");
        val *= 3600;
        return val;
    }
    if (CH == 'm' && C2 == 'i' && CX(2) == 'n' && CX(3) == 'u' && CX(4) == 't' && CX(5) == 'e'){
        if (!chkTimeSpanOfStr(6, numType))
            errorExit(tecINCORRECT_TIME_FORMAT, "wrong minutes format");
        val *= 60;
        return val;
    }
    if (CH == 's' && C2 == 'e' && CX(2) == 'c' && CX(3) == 'o' && CX(4) == 'n' && CX(5) == 'd'){
        if (!chkTimeSpanOfStr(6, numType))
            errorExit(tecINCORRECT_TIME_FORMAT, "wrong seconds format");
        return val;
    }
    errorExit(tecINCORRECT_TIME_FORMAT, "wrong time format");
    return -1;
}

bool 
MareInterpreter::chkTimeSpanOfStr(int idx, bool valueIsOne) {

    if (!valueIsOne && CX(idx) == 's') { NEXT_CH(); }
    else if (valueIsOne && CX(idx) != 's') { }
    else return false;
    token_p += idx;  /* 이동 */
    return true;
}

/** 현재 토큰의 종류가 kind2와 일치하는지 확인 후, 다음 토큰 반환 
 * (일치하지 않을 경우 에러 발생) */
Token 
MareInterpreter::chkNextTkn(Token const& tk, int kind2) 
{
    if (tk.kind != kind2) 
        errorExit(tecINCORRECT_SYNTAX, "'"+kind2Str((TknKind)kind2)+", is required instead of " + tk.text);

    return nextTkn();
}

/** 심볼 테이블에 함수/변수를 등록 */
short 
MareInterpreter::enter(SymTbl& tb, SymKind kind) 
{
    short n, mem_size;
    bool isLocal = kind == paraId ? true : false;
    if (kind == varId) {
        isLocal = isLocalScope();
    }
    JLOG(mutil.j_.trace()) << " * try to enter symbol -name:'" << tb.name
                         << "', -type:" << (kind == funcId ? "func" : "var")
                         << ", -isLocal:" << (isLocal ? "true" : "false");

    mem_size = tb.aryLen;
    if (mem_size == 0) mem_size = 1;                           /* 단순 변수일 때 처리 */

    if (kind == funcId && tb.name[0] == '$')                   /* 함수인 경우, '$' 사용불가 */
        errorExit(tecINVALID_NAME, "Don't allow '$' except variable name: ", tb.name);

    tb.symKind = kind;
    n = -1;                                                    /* 중복 여부 확인 */
    if (kind == funcId) n = searchName(tb.name, 'G');
    if (kind == paraId) n = searchName(tb.name, 'L');
    if (n != -1) errorExit(tecINVALID_NAME, "Duplicated name: ", tb.name);

    // 주소 설정
    if (kind == funcId) 
        tb.adrs = getLineNo();                                 /* 함수 시작 행 정보 저장 */
    else {
        if (isLocal) {                                         /* 로컬 변수 처리 */
            tb.adrs = localAdrs; 
            localAdrs += mem_size; 
        }
        else {
            tb.adrs = DynamicMem.size();                       /* 글로벌 변수 주소 */
            DynamicMem.resize(DynamicMem.size() + mem_size);   /* 글로벌 영역 확보 */
            if (DynamicMem.size() > MAX_MEMORY) 
                errorExit(tecBAD_ALLOCATE_MEMORY, "Allowed memory exceeded", tb.name);
        }
    }
    JLOG(mutil.j_.trace()) << " -register to symbol table: " << tb.name;

    if (isLocal) { n = Ltable.size(); Ltable.push_back(tb); }  /* 로컬 변수로 등록 */
    else         { n = Gtable.size(); Gtable.push_back(tb); }  /* 글로벌 변수로 등록 */

    return n;                                                  /* 등록된 위치(table id) 반환 */
}

/** 로컬 심볼 테이블 시작 위치 설정 */
void 
MareInterpreter::setStartLtable() 
{
    startLtable = Ltable.size();
}

/** 로컬로 선언된 이름인지 확인 */
bool 
MareInterpreter::isLocalVarName(string const& name, SymKind kind) 
{
    if (kind == paraId) { return true; }
    else if (kind == varId) {
        if (searchName(name, 'G') == -1) return true;
        //if (isLocalScope()) return true; // 함수안이라도 Global 변수일수도 있음.
    }
    return false;   /* funcId 인 경우 */
}

/** 심볼 테이블에서 함수/변수 이름 검색 */
short 
MareInterpreter::searchName(string const& sn, int mode) 
{
    JLOG(mutil.j_.trace()) << " * searchName name:'" << sn << "' mode:" << (char)mode;
    short n;
    switch (mode) {
        case 'A': 									    /* 모든 심볼 테이블 검색 */
        for (n=0; n<(int)Gtable.size(); n++) {
            if (Gtable[n].name == sn) return n;
        }
        for (n=0; n<(int)Ltable.size(); n++) {
            if (Ltable[n].name == sn) return n;
        }
        break;
    case 'G': 										    /* 글로벌 심볼 테이블 검색 */
        for (n=0; n<(int)Gtable.size(); n++) {
            if (Gtable[n].name == sn) return n;
        }
        break;
    case 'L':  											/* 로컬 심볼 테이블 검색 (글로벌을 먼저 검색) */
        n = searchName(sn, 'G');
        if (n != -1) return n;                          /* 글로벌 심볼 테이블에서 찾은 경우 */
        for (n=startLtable; n<(int)Ltable.size(); n++) {
            if (Ltable[n].name == sn) return n;
        }
        break;
    case 'F':  											/* 함수명 검색 */
        n = searchName(sn, 'G');
        if (n != -1 && Gtable[n].symKind==funcId) return n;
        break;
    case 'V':  											/* 변수명 검색 */
        /*if (searchName(s, 'F') != -1) errorExit("함수명과 중복되었습니다: ", s);
        if (s[0] == '$')     return searchName(s, 'G');*/
        if (isLocalScope()) return searchName(sn, 'L');  /* 로컬 영역 처리중 */
        else                return searchName(sn, 'G');  /* 글로벌 영역 처리중 */
        break;
    default:
        errorExit(tecUNKNOWN_CONTRACT_ERROR, "Wrong Mode(searchName function) - ", to_string(mode));
    }
    return -1;                                          /* 검색되지 않음 */
}

/** 2문자 연산자인 경우, true */
bool 
MareInterpreter::isOperator(char c1, char c2) 
{
    if (c1=='\0' || c2=='\0') return false;
    char s[] = "    ";    
    s[1] = c1; s[2] = c2;
    return strstr(" ++ -- += -= *= /= <= >= == != && || ", s) != NULL;
}

}