#include "loginfo.h"
#include <iostream>

void logM(int logLevel, string message) {
    if (logLevel & verbLevel)
        switch(logLevel) {
        case LOG_ERR:
            cerr << message;
            break;
        default:
            cout << message;
        }
}

void logMe(int logLevel, string message) {
    if (logLevel & verbLevel)
        switch(logLevel) {
        case LOG_ERR:
            cerr << message << endl;
            break;
        default:
            cout << message << endl;
        }
}
