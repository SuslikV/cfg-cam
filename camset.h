#ifndef CAMSET_H
#define CAMSET_H

#include <string>
#include <dshow.h>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <exception>

using namespace std;

extern bool ignoreFriendlyName;

enum {
    GET_SETT = 1, //get settings
    SET_SETT = 2, //set settings
    DIS_SETT = 4, //display settings
    DIS_INFO = 8  //display info
};

void ListMyDevicesInfo(); //list of video device info
void MyDevicesSettings(int gsd); //device settings manipulation
void DisplayDeviceInformation(IEnumMoniker *pEnum); //display properties of found devices
void DisplayDeviceSettings(); //from RAM
void SetDeviceSettings(IEnumMoniker *pEnum); //set settings to devices
void GetDeviceSettings(IEnumMoniker *pEnum); //get settings from devices
HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum); //Enumerate devices of given category to get devices monikers
string ConvertBSTRToMBS(BSTR bstr); //general variant string to standard string conversion
string ConvertWCSToMBS(const wchar_t* pstr, long wslen); //intermediate string conversion

class CamSetAll {
public:
    CamSetAll();
    ~CamSetAll();
    void loadSett(string cfgfilename); //load settings from .cfg file
    void saveSett(string cfgfilename); //save current settings to .cfg file
    void displayFoundDevices(); //list all available video capture devices
};

#endif // CAMSET_H
