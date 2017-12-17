//#include <dshow.h>
#include <string>
#include "loginfo.h"
#include "camset.h"

int verbLevel = VERB_NORMAL; //display messages
//int verbLevel = VERB_FULL; //display debug messages
bool ignoreFriendlyName = false;

void helpme() {
    logMe(LOG_INFO, "");
    logMe(LOG_INFO, "CamCfg.Date.2017.version.1.0");
    logMe(LOG_INFO, "To save and apply DirectShow webcam settings");
    logMe(LOG_INFO, "");
    logMe(LOG_INFO, "Usage: WebCameraConfig.exe [options]");
    logMe(LOG_INFO, "");
    logMe(LOG_INFO, "Options:");
    logMe(LOG_INFO, "--readdev          : Read and print all devices short info.");
    logMe(LOG_INFO, "--savedev          : Save devices current settings into .cfg file.");
    logMe(LOG_INFO, "--profile [string] : Uses string as filename to save/load settings.");
    logMe(LOG_INFO, "--ignorefn         : Ignore FriendlyName when looking for diveces.");
    logMe(LOG_INFO, "--help             : Display this help info.");
    logMe(LOG_INFO, "");
    logMe(LOG_INFO, "Without [options], it reads existing cam_sett.cfg file and applies settings");
    logMe(LOG_INFO, "to all available devices.");
    logMe(LOG_INFO, "It doesn't build new graph, only uses existing one.");
}

int main(int argc, char *argv[])
{
    //test
    //argc = 2;
    //argv[0] = (char *)"WebCameraConfig.exe";
    //argv[1] = (char *)"--readdev";
    //argv[1] = (char *)"--savedev";
    //argv[1] = (char *)"--ignorefn";

    string ProfStr;
    bool readVideoDevices, saveVideoDevices = false;
    int i = 1;
    for(; i < argc; i++) {
        string arg(argv[i]);
        if (arg == "--readdev") {
            readVideoDevices = true;
        } else if (arg == "--savedev") {
            saveVideoDevices = true;
        } else if (arg == "--profile") {
            if (++i < argc) ProfStr = argv[i];
        } else if (arg == "--ignorefn") {
            ignoreFriendlyName = true;
        } else if (arg == "--help") {
            helpme();
            return 0;
        } else
            break; //for
    } //for

    //by default no keys required
    if (argc != i) {
        helpme();
        return -1;
    }

    if (ProfStr == "")
        ProfStr = "cam_sett"; //default file name

    CamSetAll camupd;
    try {

        if (readVideoDevices) {
            camupd.displayFoundDevices();
            return 0; //all OK
        }

        if (saveVideoDevices) {
            camupd.saveSett(ProfStr + ".cfg");
            return 0; //all OK
        }

        //default behavior is to load/apply settings from the .cfg file
        camupd.loadSett(ProfStr + ".cfg");

    } catch(string e) {
        logMe(LOG_ERR, e);
        return -1;
    }

    return 0;
}
