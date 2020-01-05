
#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <iostream>

namespace mare_vm {

#define JLOG(x) if (!x) { } else std::cout

class log {

private:
    int level = 2;
    // 5:fatal, 4:error, 3:warn, 2:info, 1:debug, 0:trace
    
public:
    log() : level(2) {}
    log(int level_) : level(level_) {}
    /** 5:fatal, 4:error, 3:warn, 2:info, 1:debug, 0:trace */
    void set(int level_) { level = level_; }

    /** trace는 주의 (mared에 추가X) */
    bool trace() const
    {
        if (level == 0) {
            std::cout << std::endl << "[T] ";
            return true;
        }
        return false;
    }
    bool debug() const
    {
        if (level < 2) {
            std::cout << std::endl << "[D] ";
            return true;
        }
        return false;
    }
    bool info() const
    {
        if (level < 3) {
            std::cout << std::endl << "[I] ";
            return true;
        }
        return false;
    }
    bool warn() const
    {
        if (level < 4) {
            std::cout << std::endl << "[W] ";
            return true;
        }
        return false;
    }
    bool error() const
    {
        if (level < 5) {
            std::cout << std::endl << "[E] ";
            return true;
        }
        return false;
    }
};

}

#endif
