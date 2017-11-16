#include "camset.h"
#include "loginfo.h"

#pragma comment(lib, "strmiids") //to get CLSIDs
#pragma comment(lib, "oleaut32") //to get variant routines
#pragma comment(lib, "ole32") //to get CoCreateInstance etc

vector<uint32_t> idxArray;
vector<string> settArray;

CamSetAll::CamSetAll() {
    //some
}

CamSetAll::~CamSetAll() {
    //some
}

string ConvertWCSToMBS(const wchar_t* pstr, long wslen)
{
    int len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL);

    string dblstr(len, '\0');
    len = ::WideCharToMultiByte(CP_ACP, 0 /* no flags */,
                                pstr, wslen /* not necessary NULL-terminated */,
                                &dblstr[0], len,
                                NULL, NULL /* no default char */);

    return dblstr;
}

//std-string stuff to add save current device settings to .cfg file.
//Used for robust string comparison and display.
//DO NOT assign converted string to any device variables - use original BSTR type instead.
string ConvertBSTRToMBS(BSTR bstr)
{
    int wslen = ::SysStringLen(bstr);
    return ConvertWCSToMBS((wchar_t*)bstr, wslen);
}

/*
BSTR ConvertMBSToBSTR(const std::string& str)
{
    int wslen = ::MultiByteToWideChar(CP_ACP, 0 // no flags ,
                                      str.data(), str.length(),
                                      NULL, 0);

    BSTR wsdata = ::SysAllocStringLen(NULL, wslen);
    ::MultiByteToWideChar(CP_ACP, 0 // no flags ,
                          str.data(), str.length(),
                          wsdata, wslen);
    return wsdata;
}
*/

//Enumerate devices of given category to get devices monikers
HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum) {
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
        if (hr == S_FALSE) {
            hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
        }
        pDevEnum->Release();
    }
    return hr;
}

void DisplayDeviceInformation(IEnumMoniker *pEnum) {
    IMoniker *pMoniker = NULL;

    while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
        IPropertyBag *pPropBag;
        HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
        if (FAILED(hr)) {
            pMoniker->Release();
            continue;
        }

        VARIANT var;
        VariantInit(&var);

        logMe(LOG_INFO, "");

        hr = pPropBag->Read(L"DevicePath", &var, 0);
        if (SUCCEEDED(hr)) {
            // The device path is not intended for display.
            logM(LOG_INFO, "Device path: ");
            logMe(LOG_INFO, ConvertBSTRToMBS(var.bstrVal));
            VariantClear(&var);
        }

        // Get friendly name.
        hr = pPropBag->Read(L"FriendlyName", &var, 0);
        if (SUCCEEDED(hr)) {
            logM(LOG_INFO, "FriendlyName: ");
            logMe(LOG_INFO, ConvertBSTRToMBS(var.bstrVal));
            VariantClear(&var);
        }

        pPropBag->Release();
        pMoniker->Release();
    }
}

void DisplayDeviceSettings() {
    for (uint32_t i = 0; i < settArray.size(); i++) {
		logM(LOG_DBG, to_string(i) + " ");
		logMe(LOG_INFO, settArray[i]);
	}
}

//this function includes small string parser to get parameter, its value and flag.
void SetDeviceSettings(IEnumMoniker *pEnum) {
    logMe(LOG_DBG,"Set devices settings...");

    IMoniker *pMoniker = NULL;
    IAMVideoProcAmp *pProcAmp = 0;
    IAMCameraControl *pCamCtrl = 0;
    IPropertyBag *pPropBag;

        while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
            // Get the capture filter pointer for the IPropertyBag interface
            HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
            if (FAILED(hr)) {
                pMoniker->Release();
                continue;
            }

            VARIANT var;
            VariantInit(&var);
            uint32_t j = 0;
			
            // Get device path
            hr = pPropBag->Read(L"DevicePath", &var, 0);
            if (SUCCEEDED(hr)) {
                logMe(LOG_DBG, "Device path SUCCEEDED");
                logM(LOG_DBG, "j = " + to_string(j));
                logMe(LOG_DBG, "; idxArray.size() = " + to_string(idxArray.size()));
                while (j < idxArray.size()) {
                    //There is always 2 strings present per device
                    //Device #N
                    //---end of device #N

					// compare DevicePath to saved one
                    logMe(LOG_DBG, ConvertBSTRToMBS(var.bstrVal));
                    logMe(LOG_DBG, settArray[idxArray[j]]);
                    if (ConvertBSTRToMBS(var.bstrVal) == settArray[idxArray[j]]) {
						VariantClear(&var);
                        logMe(LOG_DBG,"Device path match found");
						// Get friendly name.
						hr = pPropBag->Read(L"FriendlyName", &var, 0);
						if (SUCCEEDED(hr)) {
							// compare device FriendlyName to saved one
                            if ((ignoreFriendlyName) || (ConvertBSTRToMBS(var.bstrVal) == settArray[idxArray[j] +1])) {
                                VariantClear(&var);
                                logMe(LOG_DBG,"Device FriendlyName match found");
                                //match device found

                                bool VideoProcAmpCapable = true,
                                     CameraControlCapable = true;
								
								// Get the capture filter pointer for the IAMVideoProcAmp interface.
								hr = pMoniker->BindToObject(0, 0, IID_IAMVideoProcAmp, (void**)&pProcAmp);
								if (FAILED(hr)) {
									// The device does not support IAMVideoProcAmp, so skip.
									VideoProcAmpCapable = false;
								}
									// Get the capture filter pointer for the IAMCameraControl interface.
								hr = pMoniker->BindToObject(0, 0, IID_IAMCameraControl, (void**)&pCamCtrl);
								if (FAILED(hr)) {
									// The device does not support IAMCameraControl, so skip.
									CameraControlCapable = false;
								}
								
                                string Parameter;
                                long ParValue;
                                size_t fr1;
                                size_t fr2;
                                bool FlagManual;

								//string parser
                                for (uint32_t i = idxArray[j] +2; i<idxArray[j +1]; i++) {
                                        fr1 = settArray[i].find('=');
                                        fr2 = settArray[i].find('[');
										if ((fr1 != string::npos) && (fr2 != string::npos)) {
                                            Parameter = settArray[i].substr(0, fr1);
											try {
												ParValue = stol(settArray[i].substr(fr1 +1, fr2 - fr1 -2)); //-2 is " ["	
                                            } catch(invalid_argument& ia) {
                                                throw string("Exception. Invalid argument. cam_sett.cfg, reading variable at [" + to_string(i) + ";" + to_string(fr1 +1) + "]");
                                            } catch(out_of_range& oor) {
                                                throw string("Exception. Out of range. cam_sett.cfg, reading variable at [" + to_string(i) + ";" + to_string(fr1 +1) + "]");
                                            }
											if (settArray[i].substr(fr2) == "[Manual]") { //get Auto/Manual flag
												FlagManual = true;
											} else {
												FlagManual = false;
											}
											
											//set parameters
                                            if (VideoProcAmpCapable) {
                                                if (Parameter == "VideoProcAmp_BacklightCompensation") {
                                                    hr = pProcAmp->Set(VideoProcAmp_BacklightCompensation, ParValue, FlagManual ? VideoProcAmp_Flags_Manual : VideoProcAmp_Flags_Auto);
                                                } else if (Parameter == "VideoProcAmp_Brightness") {
													hr = pProcAmp->Set(VideoProcAmp_Brightness, ParValue, FlagManual ? VideoProcAmp_Flags_Manual : VideoProcAmp_Flags_Auto);
                                                } else if (Parameter == "VideoProcAmp_ColorEnable") {
													hr = pProcAmp->Set(VideoProcAmp_ColorEnable, ParValue, FlagManual ? VideoProcAmp_Flags_Manual : VideoProcAmp_Flags_Auto);
                                                } else if (Parameter == "VideoProcAmp_Contrast") {
													hr = pProcAmp->Set(VideoProcAmp_Contrast, ParValue, FlagManual ? VideoProcAmp_Flags_Manual : VideoProcAmp_Flags_Auto);
                                                } else if (Parameter == "VideoProcAmp_Gain") {
													hr = pProcAmp->Set(VideoProcAmp_Gain, ParValue, FlagManual ? VideoProcAmp_Flags_Manual : VideoProcAmp_Flags_Auto);
                                                } else if (Parameter == "VideoProcAmp_Gamma") {
													hr = pProcAmp->Set(VideoProcAmp_Gamma, ParValue, FlagManual ? VideoProcAmp_Flags_Manual : VideoProcAmp_Flags_Auto);
                                                } else if (Parameter == "VideoProcAmp_Hue") {
													hr = pProcAmp->Set(VideoProcAmp_Hue, ParValue, FlagManual ? VideoProcAmp_Flags_Manual : VideoProcAmp_Flags_Auto);
                                                } else if (Parameter == "VideoProcAmp_Saturation") {
													hr = pProcAmp->Set(VideoProcAmp_Saturation, ParValue, FlagManual ? VideoProcAmp_Flags_Manual : VideoProcAmp_Flags_Auto);
                                                } else if (Parameter == "VideoProcAmp_Sharpness") {
													hr = pProcAmp->Set(VideoProcAmp_Sharpness, ParValue, FlagManual ? VideoProcAmp_Flags_Manual : VideoProcAmp_Flags_Auto);
                                                } else if (Parameter == "VideoProcAmp_WhiteBalance") {
													hr = pProcAmp->Set(VideoProcAmp_WhiteBalance, ParValue, FlagManual ? VideoProcAmp_Flags_Manual : VideoProcAmp_Flags_Auto);
                                                } else logMe(LOG_DBG, "VideoProcAmp string not found"); //no match found, so skip in silent

												logMe(LOG_DBG, "HRESULT: " + to_string(hr));
                                            } //if VideoProcAmpCapable

                                            if (CameraControlCapable) {
                                                if (Parameter == "CameraControl_Exposure") {
													hr = pCamCtrl->Set(CameraControl_Exposure, ParValue, FlagManual ? CameraControl_Flags_Manual : CameraControl_Flags_Auto);
                                                } else if (Parameter == "CameraControl_Focus") {
													hr = pCamCtrl->Set(CameraControl_Focus, ParValue, FlagManual ? CameraControl_Flags_Manual : CameraControl_Flags_Auto);
                                                } else if (Parameter == "CameraControl_Iris") {
													hr = pCamCtrl->Set(CameraControl_Iris, ParValue, FlagManual ? CameraControl_Flags_Manual : CameraControl_Flags_Auto);
                                                } else if (Parameter == "CameraControl_Pan") {
													hr = pCamCtrl->Set(CameraControl_Pan, ParValue, FlagManual ? CameraControl_Flags_Manual : CameraControl_Flags_Auto);
                                                } else if (Parameter == "CameraControl_Roll") {
													hr = pCamCtrl->Set(CameraControl_Roll, ParValue, FlagManual ? CameraControl_Flags_Manual : CameraControl_Flags_Auto);
                                                } else if (Parameter == "CameraControl_Tilt") {
													hr = pCamCtrl->Set(CameraControl_Tilt, ParValue, FlagManual ? CameraControl_Flags_Manual : CameraControl_Flags_Auto);
                                                } else if (Parameter == "CameraControl_Zoom") {
													hr = pCamCtrl->Set(CameraControl_Zoom, ParValue, FlagManual ? CameraControl_Flags_Manual : CameraControl_Flags_Auto);
                                                } else logMe(LOG_DBG, "CameraControl string not found"); //no match found, so skip in silent

												logMe(LOG_DBG, "HRESULT: " + to_string(hr));
                                            } //if CameraControlCapable
										} //if actual parameter found (fr1,fr2)
								} //for
                                if (VideoProcAmpCapable) pProcAmp->Release();
                                if (CameraControlCapable) pCamCtrl->Release();
                                break; //break while idxArray, try next device (moniker)
							} //if FriendlyName match	
						}
					} //if DevicePath match
						
					// iterate through each second element of idxArray (begin of the device description)
					j += 2;	
					
				} //while idxArray
            } //if device path read succeeded
            pPropBag->Release();
            pMoniker->Release();
        } //while
}

void GetDeviceSettings(IEnumMoniker *pEnum) {
    IMoniker *pMoniker = NULL;
    IAMVideoProcAmp *pProcAmp = 0;
    IAMCameraControl *pCamCtrl = 0;
    IPropertyBag *pPropBag;
    int i = 0;
        while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
            // Get the capture filter pointer for the IPropertyBag interface
            HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
            if (FAILED(hr)) {
                pMoniker->Release();
                continue;
            }

            logMe(LOG_DBG,"Bind to properties storage SUCCEEDED");            

            VARIANT var;
            VariantInit(&var);

            i++;
            // add device number (just for info)
            settArray.push_back("Device #" + to_string(i));
            // add indexes of the device descriptions begin
            idxArray.push_back(settArray.size());

            // Get device path
            hr = pPropBag->Read(L"DevicePath", &var, 0);
            if (SUCCEEDED(hr)) {
				// The device path is not intended for display.
				settArray.push_back(ConvertBSTRToMBS(var.bstrVal));
                VariantClear(&var);
            }

            // Get friendly name.
            hr = pPropBag->Read(L"FriendlyName", &var, 0);
            if (SUCCEEDED(hr)) {
				settArray.push_back(ConvertBSTRToMBS(var.bstrVal));
                VariantClear(&var);
            }

            long Val, Flags;
            bool VideoProcAmpCapable = true,
                 CameraControlCapable = true;

            // Get the capture filter pointer for the IAMVideoProcAmp interface.
            hr = pMoniker->BindToObject(0, 0, IID_IAMVideoProcAmp, (void**)&pProcAmp);
            if (FAILED(hr)) {
                // The device does not support IAMVideoProcAmp.
                logMe(LOG_DBG, "IAMVideoProcAmp not supported by device");
                VideoProcAmpCapable = false;
            }

            // Get the capture filter pointer for the IAMCameraControl interface.
            hr = pMoniker->BindToObject(0, 0, IID_IAMCameraControl, (void**)&pCamCtrl);
            if (FAILED(hr)) {
                // The device does not support IAMCameraControl, so skip.
                // if it is last in the list, then Release moniker+Continue possible.
                logMe(LOG_DBG, "IAMCameraControl not supported by device");
                CameraControlCapable = false;
            }

            if (VideoProcAmpCapable) {
                logMe(LOG_DBG,"Bind to object with IID_IAMVideoProcAmp SUCCEEDED");

                // Get the current value.
                hr = pProcAmp->Get(VideoProcAmp_BacklightCompensation, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("VideoProcAmp_BacklightCompensation=" +
                            to_string(Val) +
                            ((Flags > VideoProcAmp_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pProcAmp->Get(VideoProcAmp_Brightness, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("VideoProcAmp_Brightness=" +
                            to_string(Val) +
                            ((Flags > VideoProcAmp_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pProcAmp->Get(VideoProcAmp_ColorEnable, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("VideoProcAmp_ColorEnable=" +
                            to_string(Val) +
                            ((Flags > VideoProcAmp_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pProcAmp->Get(VideoProcAmp_Contrast, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("VideoProcAmp_Contrast=" +
                            to_string(Val) +
                            ((Flags > VideoProcAmp_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pProcAmp->Get(VideoProcAmp_Gain, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("VideoProcAmp_Gain=" +
                            to_string(Val) +
                            ((Flags > VideoProcAmp_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pProcAmp->Get(VideoProcAmp_Gamma, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("VideoProcAmp_Gamma=" +
                            to_string(Val) +
                            ((Flags > VideoProcAmp_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pProcAmp->Get(VideoProcAmp_Hue, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("VideoProcAmp_Hue=" +
                            to_string(Val) +
                            ((Flags > VideoProcAmp_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pProcAmp->Get(VideoProcAmp_Saturation, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("VideoProcAmp_Saturation=" +
                            to_string(Val) +
                            ((Flags > VideoProcAmp_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pProcAmp->Get(VideoProcAmp_Sharpness, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("VideoProcAmp_Sharpness=" +
                            to_string(Val) +
                            ((Flags > VideoProcAmp_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pProcAmp->Get(VideoProcAmp_WhiteBalance, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("VideoProcAmp_WhiteBalance=" +
                            to_string(Val) +
                            ((Flags > VideoProcAmp_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                pProcAmp->Release();
            } //if VideoProcAmpCapable

            if (CameraControlCapable) {
                logMe(LOG_DBG,"Bind to object with IID_IAMCameraControl SUCCEEDED");

                hr = pCamCtrl->Get(CameraControl_Exposure, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("CameraControl_Exposure=" +
                            to_string(Val) +
                            ((Flags > CameraControl_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pCamCtrl->Get(CameraControl_Focus, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("CameraControl_Focus=" +
                            to_string(Val) +
                            ((Flags > CameraControl_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pCamCtrl->Get(CameraControl_Iris, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("CameraControl_Iris=" +
                            to_string(Val) +
                            ((Flags > CameraControl_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pCamCtrl->Get(CameraControl_Pan, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("CameraControl_Pan=" +
                            to_string(Val) +
                            ((Flags > CameraControl_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pCamCtrl->Get(CameraControl_Roll, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("CameraControle_Roll=" +
                            to_string(Val) +
                            ((Flags > CameraControl_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pCamCtrl->Get(CameraControl_Tilt, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("CameraControl_Tilt=" +
                            to_string(Val) +
                            ((Flags > CameraControl_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                hr = pCamCtrl->Get(CameraControl_Zoom, &Val, &Flags);
                if (SUCCEEDED(hr)) {
                    settArray.push_back("CameraControl_Zoom=" +
                            to_string(Val) +
                            ((Flags > CameraControl_Flags_Auto) ? " [Manual]" : " [Auto]"));
                }

                pCamCtrl->Release();
            } // if CameraControlCapable

			settArray.push_back("---end of the device #" + to_string(i));
			settArray.push_back("");
			// add indexes of the device descriptions end
            idxArray.push_back(settArray.size());

            pPropBag->Release();
            pMoniker->Release();
        } //while

}

//manipulate video device settings:
//get moniker for video device and call right function.
void MyDevicesSettings(int gsd) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {
        IEnumMoniker *pEnum;

        logMe(LOG_DBG,"Enumerate Devices...");

        hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
        if (SUCCEEDED(hr)) {
            switch (gsd) {
            case GET_SETT:
                GetDeviceSettings(pEnum);
                break;
            case SET_SETT:
                SetDeviceSettings(pEnum);
                break;
            case DIS_SETT:
			    DisplayDeviceSettings();
                break;
            case DIS_INFO:
                DisplayDeviceInformation(pEnum);
                break;
            default:
                logMe(LOG_ERR,"Unknown action requested.");
                break;
            }
            pEnum->Release();
            logMe(LOG_DBG, "Enemurate SUCCEEDED.");
        } else {
            logMe(LOG_DBG, "HRESULT:" + to_string(hr));
        }

        CoUninitialize();
    }
}

void CamSetAll::loadSett(string cfgfilename) {
    logMe(LOG_DBG,"reading... " + cfgfilename);
    //read from .cfg file to RAM
	string line;
	ifstream cfgfile;
    cfgfile.open (cfgfilename, ios::binary); //read .cfg file
	if (cfgfile.is_open()) {
		while (getline(cfgfile,line)) {
            //ignore string with '/' at start of the string (comments)
            //short logic used
            if ((line.length() != 0) && (line[0] != '/')) {
                settArray.push_back(line);
                // Fill idxArray with device description boundaries
				if (line.find("Device #") == 0) {
					idxArray.push_back(settArray.size());
				} else if (line.find("---end of the device #") == 0)
					idxArray.push_back(settArray.size());
			}
		}
	} else {
        throw string("Unable to open file: " + cfgfilename);
	}
	cfgfile.close();
    //ignore small descriptions
    if (settArray.size() > 1) {
        MyDevicesSettings(SET_SETT);
    } else logMe(LOG_DBG, "File too small: " + cfgfilename);
}

void CamSetAll::saveSett(string cfgfilename){
    //read from device and save to .cfg file
    MyDevicesSettings(GET_SETT); //get from device
	logMe(LOG_DBG, "Get Divice settings COMPLETE");
	MyDevicesSettings(DIS_SETT); //display settings
	logMe(LOG_DBG, "Display Divice settings COMPLETE");

	ofstream cfgfile;
    cfgfile.open (cfgfilename, ios::trunc | ios::binary); //overwrite .cfg file
	if (cfgfile.is_open()) {
        cfgfile << "/ WebCameraConfig settings file" << "\n"; //write info line (comment)
        for (uint32_t i = 0; i < settArray.size(); i++)
			cfgfile << settArray[i] + "\n"; //write line
	} else {
        logMe(LOG_ERR, "Unable to open file for writing: " + cfgfilename);
	}
	cfgfile.close();
}

void CamSetAll::displayFoundDevices() {
    MyDevicesSettings(DIS_INFO);
}

