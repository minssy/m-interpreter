#build for Mac/Ubuntu

#g++ -Wall -std=c++11 -o m-interpreter MareUtil.cpp MareBase.cpp MareExecuter.cpp MareInitExec.cpp MareInterpreter.cpp main.cpp
g++ -std=c++14 -o m-interpreter MareUtil.cpp MareBase.cpp MareExecuter.cpp MareInitExec.cpp MareInterpreter.cpp main.cpp
