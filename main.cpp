
#include "MareInterpreter.h"
#include "MareExecuter.h"
#include <time.h>

using namespace mare_vm;

/** 파일 열기 */
vector<string>  
fileOpen(char *fname) 
{
    vector<string> lines;
    char tmpBuf[LINE_SIZE_MAX];                /* 소스의 1line을 읽어 저장할 배열 */
    ifstream fin;                              /* 파일 입력 스트림 */
    fin.open(fname);
    if (!fin) { 
        cout << endl << fname << " 을(를) 열 수 없습니다\n"; 
        exit(1); 
    }
    while (!fin.eof()) {                       /* 파일의 끝인 경우 */
        fin.getline(tmpBuf, LINE_SIZE_MAX);    /* 1행 읽기 */
        string lin(tmpBuf);
        lines.push_back(lin);
    }
    fin.clear();                               /* clear(): 재 오픈에 대비 */ 
    fin.close();
    return lines;
}

void fileSave(Blob const& obj, char const *fname) 
{
    long max =  obj.size();
    char tt[max+1];
    cout << endl << " -file Save: " << fname << " size:" << max;
    long i=0;
    for (i=0; i<max; i++){
        tt[i] =(char) obj.at(i);
    }
    tt[i] = '\0';
    ofstream outputs(fname, ios::out | ios::binary);
    outputs.write(tt, max);
    outputs.close();
}

void fileLoad(Blob& obj, char const *fname) 
{
    cout << endl << " -file Load: " << fname;
    obj.clear();
    ifstream inputs(fname, ios::in | ios::binary);
    //파일의 끝으로 포인터를 이동.
    inputs.seekg(0, ios::end);
    //파일 사이즈 읽음.
    int sz = inputs.tellg();
    char buf[sz];
    cout << " -size:" << sz;
    //포인터를 파일의 처음으로 이동시킴.
    inputs.seekg(0, ios::beg);
    //binary로 파일을 읽음.
    inputs.read(buf, sz);    
    inputs.close();
    for (int i=0; i<sz; i++){
        unsigned char cc = static_cast<unsigned char>(buf[i]);
        obj.push_back(cc);
    }
}

int main(int argc, char *argv[])
{
    Blob obj;
    try {
        //cout << " argc:" << argc << endl;
        
        if (argc == 1) {
            cout << "usage : mare-interpreter" << endl;
            cout << " ./m-interpreter <mare-script-filename> : script load, verify(syntax), run(init), save(next run)" << endl;
            cout << " ./m-interpreter create <mare-script-filename> : script load, verify(syntax), run(init), save(next run)" << endl;
            cout << " ./m-interpreter create <mare-script-filename> <func name & arges...> : script load, verify(syntax), run(init), run(func), save(next run)" << endl;
            cout << " ./m-interpreter run <func name & arges...> : load(from saved file), run(func), save(updated memory)" << endl;
            cout << " ./m-interpreter runself <func name & arges...> : load(from saved file), run(func), save(updated memory)" << endl;
            return 0;
        }

        // 로그 레벨 지정
        int logLevel = 0; // trace=0, debug=1, info=2, warn=3, ...

        string opt(argv[1]);
        string execfunc = "";
        vector<string> strs;
        if (argc == 2 || opt.compare("create") == 0) 
        {
            auto mareUtil = MareUtil(1);
            mareUtil.j_.set(logLevel);

            JLOG(mareUtil.j_.info()) << " * execute option:" <<  opt << endl;
            auto pars = MareInterpreter(mareUtil);
            pars.debugging("===== convert_to_internalCode =====", 1);
            if (argc == 2){
                auto sources = fileOpen(argv[1]);
                pars.convertToInterCode(sources);
            }
            else {
                auto sources = fileOpen(argv[2]);
                pars.convertToInterCode(sources);
            }

            pars.debugging("===== saving =====", 1);
            pars.printInfos();
            
            obj = pars.getInterCode();
            fileSave(obj, "./temp/dataCode.bin");
            obj = pars.getFuncInfos();
            fileSave(obj, "./temp/dataFuncs.bin");
            obj = pars.getLiterals();
            fileSave(obj, "./temp/dataLiterals.bin");
            obj = pars.getSymbolTable();
            fileSave(obj, "./temp/dataSymbols.bin");

            // 사용자가 시작 함수명과 인수를 입력했는지 확인
            // 사용자가 따로 입력한 것이 없을 경우, main함수가 있는지 확인
            if (argc > 3){
                pars.debugging("===== execute ready =====", 1);
                for (int k=3; k<argc; k++) {
                    string funcinfoStr(argv[k]);
                    strs.push_back(funcinfoStr);
                }
                pars.prepareExecute(strs);
                pars.debugging("===== saving =====", 1);
                //pars.print(); 
            }

            obj = pars.getMemories();
            fileSave(obj, "./temp/dataMemoryPri.bin");

            pars.debugging("===== log =====", 0);
            Blob bb = pars.getLogs();
            string strLog {bb.begin(), bb.end()};
            JLOG(mareUtil.j_.info()) << strLog;

            pars.debugging("===== done =====", 0);
        } // end create
        else if (opt.compare("run") == 0 || opt.compare("runself") == 0) 
        {            
            auto mareUtil = MareUtil(1);
            mareUtil.j_.set(logLevel);

            JLOG(mareUtil.j_.info()) << " * execute option:" <<  opt << endl;
            strs.clear();
            for (int cnt = 2; cnt < argc; cnt++){
                string param(argv[cnt]);
                if (!param.empty()){
                    strs.push_back(param);
                }
            }

            auto exec = MareExecuter(mareUtil);
            exec.debugging("===== loading =====", 1);
            //exec.loadProcess();
            size_t datas_sz;
            fileLoad(obj, "./temp/dataCode.bin");
            datas_sz = obj.size();
            exec.setInterCode(obj);
            datas_sz += obj.size();
            fileLoad(obj, "./temp/dataLiterals.bin");
            exec.setLiterals(obj);
            datas_sz += obj.size();
            fileLoad(obj, "./temp/dataSymbols.bin");
            exec.setSymbolTable(obj);
            datas_sz += obj.size();
            fileLoad(obj, "./temp/dataMemoryPri.bin");
            exec.setMemories(obj);

            exec.debugging("===== loaded =====", 1);
            exec.printInfos();

            exec.debugging("===== execute =====", 1);
            exec.prepareExecute(strs);

            exec.debugging("===== saving =====", 1);
            if (exec.isUpdatedSymbolTable()) {
                obj = exec.getSymbolTable();
                fileSave(obj, "./temp/dataSymbols.bin");
            }
            obj = exec.getMemories();
            fileSave(obj, "./temp/dataMemoryPri.bin");
            
            exec.debugging("===== log =====", 0);
            Blob bb = exec.getLogs();
            string strLog {bb.begin(), bb.end()};
            JLOG(mareUtil.j_.info()) << strLog;

            exec.debugging("===== done =====", 0);
        }
        else {
            throw "wrong mare argument";
        }
    }
    catch(int ex) {
        cout << endl << "[ERROR int ex] " << ex << endl;
    }
    catch(ErrObj ex) {
        cout << endl << "[ERROR ErrObj ex] " << ex.whatStr() << endl;
    }
    catch(const char* ex) {
        cout << endl << "[ERROR string ex] " << ex << endl;
    }
    catch(exception ex) {
        cout << endl << "[ERROR exception ex] " << ex.what() << endl;
    }
    catch(TECcodes ex) {
        cout << endl << "[ERROR TECcodes ex] " << transToken(ex) << endl;
    }
    catch(...) {
        cout << endl << "[ERROR Unknown ex] ??? " << endl;
    }

    return 0;
}


