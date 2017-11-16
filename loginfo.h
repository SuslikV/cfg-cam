#ifndef LOGINFO_H
#define LOGINFO_H
#include <string>

using namespace std;

extern int verbLevel; //declare and define it at 'main.cpp' but also use in 'loginfo.cpp'

enum {
    LOG_DBG  = 1, //debug info
    LOG_INFO = 2, //simple info
    LOG_MSG  = 4, //dialog messages
    LOG_WRN  = 8, //warnings
    LOG_ERR  = 16 //errors
};

enum {
    VERB_FULL    = LOG_ERR + LOG_WRN + LOG_MSG + LOG_INFO + LOG_DBG, //maximum verbosity, 31
    VERB_NORMAL  = LOG_ERR + LOG_WRN + LOG_MSG + LOG_INFO, //no debug info, 30
    VERB_SILENT  = 0  //minimum verbosity
};

void logM(int logLevel, string message);
void logMe(int logLevel, string message);

#endif // LOGINFO_H
