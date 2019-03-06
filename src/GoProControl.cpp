/*
GoProControl.cpp

Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, 
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <GoProControl.h>

GoProControl::GoProControl(WiFiClient client, const String ssid, const String pwd, const uint8_t camera)
{
	_client = client;
	_ssid = ssid;
	_pwd = pwd;
	_camera = camera;

	if (_camera == HERO3) // HERO3, HERO3+, HERO3BLACK, HERO3BLACK+
	{
		// URL scheme: http://HOST/param1/PARAM2?t=PASSWORD&p=%OPTION
		// example:	  http://10.5.5.9/camera/SH?t=password&p=%01

		_url = "http://" + _host + "/camera/";
	}
	else if (_camera >= HERO4) // HERO4, 5, 6, 7:
	{
		// URL scheme: http://HOST/gp/gpControl/....
		// Basic functions (record, mode, tag, poweroff): http://HOST/gp/gpControl/command/PARAM?p=OPTION
		// example change mode to video: http://10.5.5.9/gp/gpControl/command/mode?p=0
		// Settings: http://HOST/gp/gpControl/setting/option
		// example change video resolution to 1080p: http://10.5.5.9/gp/gpControl/setting/2/9)

		_url = "http://" + _host + "/gp/gpControl/";
	}
}

uint8_t GoProControl::begin()
{
	if (checkConnection())
	{
		if (_debug)
		{
			_debug_port->println("Already connected");
		}
		return false;
	}

	if (_camera <= HERO2)
	{
		if (_debug)
		{
			_debug_port->println("Camera not supported");
		}
		return -1;
	}

	if (_debug)
	{
		_debug_port->println("Attempting to connect to SSID: " + _ssid);
		_debug_port->println("using password: " + _pwd);
	}

	WiFi.begin(_ssid.c_str(), _pwd.c_str());

	while (WiFi.status() == WL_IDLE_STATUS)
	{
		;
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		if (_debug)
		{
			_debug_port->println("Connected");
		}
		_connected = true;
		return true;
	}
	else if (WiFi.status() == WL_CONNECT_FAILED)
	{
		if (_debug)
		{
			_debug_port->println("Connection failed");
		}
		_connected = false;
		return -2;
	}
	else
	{
		_debug_port->print("WiFi.status(): ");
		_debug_port->println(WiFi.status());
		_connected = false;
		return false;
	}
}

void GoProControl::end()
{
	if (!checkConnection())
	{
		return;
	}

	if (_debug)
	{
		_debug_port->println("Closing connection");
	}
	_client.stop();
	WiFi.disconnect();
	_connected = false;
}

uint8_t GoProControl::keepAlive()
{
	if (!checkConnection()) // camera not connected
	{
		return false;
	}

	if (millis() - _last_request <= KEEP_ALIVE) // we made a request not so much earlier
	{
		return false;
	}
	else // time to ask something to the camera
	{
		if (_debug)
		{
			_debug_port->println("Keeping connection alive");
		}
		//confirmPairing(); todo not working
	}
}

uint8_t GoProControl::confirmPairing()
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	String request;

	if (_camera == HERO3)
	{
		request = _url + "DL?t=" + _pwd;
	}
	else if (_camera >= HERO4)
	{
		//todo get deviceName
		request = _url + "command/wireless/pair/complete?success=1&deviceName=ESPBoard";
	}

	return sendRequest(request);
}

uint8_t GoProControl::turnOn()
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (isOn()) // camera is on
	{
		if (_debug)
		{
			_debug_port->println("turn off the camera first");
		}
		return false;
	}

	String request;

	if (_camera == HERO3)
	{
		request = _url + "PW?t=" + _pwd + "&p=%01";
	}
	else if (_camera >= HERO4)
	{
		// todo
		//sendWoL();
		return true;
	}

	return sendRequest(request);
}

uint8_t GoProControl::turnOff()
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;

	if (_camera == HERO3)
	{
		request = _url + "PW?t=" + _pwd + "&p=%00";
	}
	else if (_camera >= HERO4)
	{
		request = _url + "command/system/sleep";
	}

	return sendRequest(request);
}

uint8_t GoProControl::isOn()
{
	// does this command exist?
	return true;
}

uint8_t GoProControl::checkConnection()
{
	if (getStatus())
	{
		if (_debug)
		{
			_debug_port->println("Connected");
		}
		return true;
	}
	else
	{
		if (_debug)
		{
			_debug_port->println("Not connected");
		}
		return false;
	}
}

uint8_t GoProControl::shoot()
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;

	if (_camera == HERO3)
	{
		request = _url + "SH?t=" + _pwd + "&p=%01";
	}
	else if (_camera >= HERO4)
	{
		request = _url + "command/shutter?p=1";
	}

	return sendRequest(request);
}

uint8_t GoProControl::stopShoot()
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;

	if (_camera == HERO3)
	{
		request = _url + "SH?t=" + _pwd + "&p=%00";
	}
	else if (_camera >= HERO4)
	{
		request = _url + "command/shutter?p=0";
	}

	return sendRequest(request);
}

////////////////////////////////////////////////////////////
////////                  Settings                  ////////
////////////////////////////////////////////////////////////

uint8_t GoProControl::setMode(const uint8_t option)
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;
	String parameter;

	if (_camera == HERO3)
	{
		switch (option)
		{
		case VIDEO_MODE:
			parameter = "00";
			break;
		case PHOTO_MODE:
			parameter = "01";
			break;
		case BURST_MODE:
			parameter = "02";
			break;
		case TIMELAPSE_MODE:
			parameter = "03";
			break;
		case TIMER_MODE:
			parameter = "04";
			break;
		case PLAY_HDMI_MODE:
			parameter = "05";
			break;
		default:
			_debug_port->println("Wrong parameter for setMode");
			return -1;
		}

		request = _url + "CM?t=" + _pwd + "&p=%" + parameter;
	}
	else if (_camera >= HERO4)
	{
		//todo: add sub-modes
		switch (option)
		{
		case VIDEO_MODE:
			parameter = "0";
			break;
		case PHOTO_MODE:
			parameter = "1";
			break;
		case MULTISHOT_MODE:
			parameter = "2";
			break;
		default:
			_debug_port->println("Wrong parameter for setMode");
			return -1;
		}

		request = _url + "command/mode?p=" + parameter;
	}

	return sendRequest(request);
}

uint8_t GoProControl::setOrientation(const uint8_t option)
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;
	String parameter;

	if (_camera == HERO3)
	{
		switch (option)
		{
		case ORIENTATION_UP:
			parameter = "00";
			break;
		case ORIENTATION_DOWN:
			parameter = "01";
			break;
		default:
			_debug_port->println("Wrong parameter for setOrientation");
			return -1;
		}

		request = _url + "UP?t=" + _pwd + "&p=%" + parameter;
	}
	else if (_camera >= HERO4)
	{
		switch (option)
		{
		case ORIENTATION_UP:
			parameter = "0";
			break;
		case ORIENTATION_DOWN:
			parameter = "1";
			break;
		case ORIENTATION_AUTO:
			parameter = "2";
			break;
		default:
			_debug_port->println("Wrong parameter for setOrientation");
			return -1;
		}

		request = _url + "setting/52/" + parameter;
	}

	return sendRequest(request);
}

////////////////////////////////////////////////////////////
////////                   Video                   /////////
////////////////////////////////////////////////////////////

uint8_t GoProControl::setVideoResolution(const uint8_t option)
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;
	String parameter;

	if (_camera == HERO3)
	{
		switch (option)
		{
		case VR_1080p:
			parameter = "06";
			break;
		case VR_960p:
			parameter = "05";
			break;
		case VR_720p:
			parameter = "03";
			break;
		case VR_WVGA:
			parameter = "01";
			break;
		default:
			_debug_port->println("Wrong parameter for setVideoResolution");
			return -1;
		}

		request = _url + "VR?t=" + _pwd + "&p=%" + parameter;
	}
	else if (_camera >= HERO4)
	{
		switch (option)
		{
		case VR_4K:
			parameter = "1";
			break;
		case VR_2K:
			parameter = "4";
			break;
		case VR_2K_SuperView:
			parameter = "5";
			break;
		case VR_1440p:
			parameter = "7";
			break;
		case VR_1080p_SuperView:
			parameter = "8";
			break;
		case VR_1080p:
			parameter = "9";
			break;
		case VR_960p:
			parameter = "10";
			break;
		case VR_720p_SuperView:
			parameter = "11";
			break;
		case VR_720p:
			parameter = "12";
			break;
		case VR_WVGA:
			parameter = "13";
			break;
		default:
			_debug_port->println("Wrong parameter for setVideoResolution");
			return -1;
		}

		request = _url + "setting/2/" + parameter;
	}

	return sendRequest(request);
}

uint8_t GoProControl::setVideoFov(const uint8_t option)
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;
	String parameter;

	if (_camera == HERO3)
	{
		switch (option)
		{
		case WIDE_FOV:
			parameter = "00";
			break;
		case MEDIUM_FOV:
			parameter = "01";
			break;
		case NARROW_FOV:
			parameter = "02";
			break;
		default:
			_debug_port->println("Wrong parameter for setVideoFov");
			return -1;
		}

		request = _url + "FV?t=" + _pwd + "&p=%" + parameter;
	}
	else if (_camera >= HERO4)
	{
		switch (option)
		{
		case WIDE_FOV:
			parameter = "0";
			break;
		case MEDIUM_FOV:
			parameter = "1";
			break;
		case NARROW_FOV:
			parameter = "2";
			break;
		case LINEAR_FOV:
			parameter = "4";
			break;
		default:
			_debug_port->println("Wrong parameter for setVideoFov");
			return -1;
		}

		request = _url + "setting/4/" + parameter;
	}

	return sendRequest(request);
}

uint8_t GoProControl::setFrameRate(const uint8_t option)
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;
	String parameter;

	if (_camera == HERO3)
	{
		switch (option)
		{
		case FR_240:
			parameter = "0a";
			break;
		case FR_120:
			parameter = "09";
			break;
		case FR_100:
			parameter = "08";
			break;
		case FR_60:
			parameter = "07";
			break;
		case FR_50:
			parameter = "06";
			break;
		case FR_48:
			parameter = "05";
			break;
		case FR_30:
			parameter = "04";
			break;
		case FR_25:
			parameter = "03";
			break;
		case FR_24:
			parameter = "02";
			break;
		case FR_12p5:
			parameter = "0b";
			break;
		case FR_15:
			parameter = "01";
			break;
		case FR_12:
			parameter = "00";
			break;
		default:
			_debug_port->println("Wrong parameter for setFrameRate");
			return -1;
		}

		request = _url + "FS?t=" + _pwd + "&p=%" + parameter;
	}
	else if (_camera >= HERO4)
	{
		switch (option)
		{
		case FR_240:
			parameter = "0";
			break;
		case FR_120:
			parameter = "1";
			break;
		case FR_100:
			parameter = "2";
			break;
		case FR_90:
			parameter = "3";
			break;
		case FR_80:
			parameter = "4";
			break;
		case FR_60:
			parameter = "5";
			break;
		case FR_50:
			parameter = "6";
			break;
		case FR_48:
			parameter = "7";
			break;
		case FR_30:
			parameter = "8";
			break;
		case FR_25:
			parameter = "9";
			break;
		default:
			_debug_port->println("Wrong parameter for setFrameRate");
			return -1;
		}

		request = _url + "setting/3/" + parameter;
	}

	return sendRequest(request);
}

uint8_t GoProControl::setVideoEncoding(const uint8_t option)
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;
	String parameter;

	if (_camera == HERO3)
	{
		switch (option)
		{
		case NTSC:
			parameter = "00";
			break;
		case PAL:
			parameter = "01";
			break;
		default:
			_debug_port->println("Wrong parameter for setVideoEncoding");
			return -1;
		}

		request = _url + "VM?t=" + _pwd + "&p=%" + parameter;
	}
	else if (_camera >= HERO4)
	{
		switch (option)
		{
		case NTSC:
			parameter = "0";
			break;
		case PAL:
			parameter = "1";
			break;
		default:
			_debug_port->println("Wrong parameter for setVideoEncoding");
			return -1;
		}

		request = _url + "setting/57/" + parameter;
	}

	return sendRequest(request);
}

////////////////////////////////////////////////////////////
////////                   Photo                   /////////
////////////////////////////////////////////////////////////

uint8_t GoProControl::setPhotoResolution(const uint8_t option)
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;
	String parameter;

	if (_camera == HERO3)
	{
		switch (option)
		{
		case PR_11MP_WIDE:
			parameter = "00";
			break;
		case PR_8MP_WIDE:
			parameter = "01";
			break;
		case PR_5MP_WIDE:
			parameter = "02";
			break;
		default:
			_debug_port->println("Wrong parameter for setPhotoResolution");
			return -1;
		}

		request = _url + "PR?t=" + _pwd + "&p=%" + parameter;
	}
	else if (_camera >= HERO4)
	{
		switch (option)
		{
		case PR_12MP_WIDE:
			parameter = "0";
			break;
		case PR_12MP_LINEAR:
			parameter = "10";
			break;
		case PR_12MP_MEDIUM:
			parameter = "8";
			break;
		case PR_12MP_NARROW:
			parameter = "9";
			break;
		case PR_7MP_WIDE:
			parameter = "1";
			break;
		case PR_7MP_MEDIUM:
			parameter = "2";
			break;
		case PR_5MP_WIDE:
			parameter = "3";
			break;
		default:
			_debug_port->println("Wrong parameter for setPhotoResolution");
			return -1;
		}

		request = _url + "setting/17/" + parameter;
	}

	return sendRequest(request);
}

uint8_t GoProControl::setTimeLapseInterval(float option)
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	if (option != 0.5 || option != 1 || option != 5 || option != 10 || option != 30 || option != 60)
	{
		if (_debug)
		{
			_debug_port->println("Wrong parameter for setTimeLapseInterval");
		}
		return -1;
	}

	String request;
	String parameter;

	// float cannot be used in switch statements
	if (option == 0.5)
	{
		option = 0;
	}
	const uint8_t integer_option = (int)option;

	if (_camera == HERO3)
	{
		switch (integer_option)
		{
		case 60:
			parameter = "3c";
			break;
		case 30:
			parameter = "1e";
			break;
		case 10:
			parameter = "0a";
			break;
		case 5:
			parameter = "05";
			break;
		case 1:
			parameter = "01";
			break;
		case 0: // 0.5
			parameter = "00";
			break;
		default:
			_debug_port->println("Wrong parameter for setTimeLapseInterval");
			return -1;
		}

		request = _url + "TI?t=" + _pwd + "&p=%" + parameter;
	}
	else if (_camera >= HERO4)
	{
		switch (integer_option)
		{
		case 60:
			parameter = "6";
			break;
		case 30:
			parameter = "5";
			break;
		case 10:
			parameter = "4";
			break;
		case 5:
			parameter = "3";
			break;
		case 1:
			parameter = "1";
			break;
		case 0: // 0.5
			parameter = "0";
			break;
		default:
			_debug_port->println("Wrong parameter for setTimeLapseInterval");
			return -1;
		}

		request = _url + "setting/5/" + parameter;
	}

	return sendRequest(request);
}

uint8_t GoProControl::setContinuousShot(const uint8_t option)
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	if (option != 0 || option != 3 || option != 5 || option != 10)
	{
		if (_debug)
		{
			_debug_port->println("Wrong parameter for setContinuousShot");
		}
		return -1;
	}

	String request;
	String parameter;

	if (_camera == HERO3)
	{
		switch (option)
		{
		case 10:
			parameter = "0a";
			break;
		case 5:
			parameter = "05";
			break;
		case 3:
			parameter = "03";
			break;
		case 0:
			parameter = "00";
			break;
		default:
			_debug_port->println("Wrong parameter for setContinuousShot");
			return -1;
		}

		request = _url + "CS?t=" + _pwd + "&p=%" + parameter;
	}
	else if (_camera >= HERO4)
	{
		// Not supported in Hero4/5/6/7
		return false;
	}
	return sendRequest(request);
}

////////////////////////////////////////////////////////////
////////                   Others                   ////////
////////////////////////////////////////////////////////////

uint8_t GoProControl::localizationOn()
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}
	/*
	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}
*/
	String request;

	if (_camera == HERO3)
	{
		request = _url + "LL?t=" + _pwd + "&p=%01";
	}
	else if (_camera >= HERO4)
	{
		request = _url + "command/system/locate?p=1";
	}

	return sendRequest(request);
}

uint8_t GoProControl::localizationOff()
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}
	//todo test to turn it on and off with the camera off
	/*
	if (!isOn()) // camera is off 
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}*/

	String request;

	if (_camera == HERO3)
	{
		request = _url + "LL?t=" + _pwd + "&p=%00";
	}
	else if (_camera >= HERO4)
	{
		request = _url + "command/system/locate?p=0";
	}

	return sendRequest(request);
}

uint8_t GoProControl::deleteLast()
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;

	if (_camera == HERO3)
	{
		request = _url + "DL?t=" + _pwd;
	}
	else if (_camera >= HERO4)
	{
		request = _url + "command/storage/delete/last";
	}

	return sendRequest(request);
}

uint8_t GoProControl::deleteAll()
{
	if (!checkConnection()) // not connected
	{
		if (_debug)
		{
			_debug_port->println("Connect the camera first");
		}
		return false;
	}

	if (!isOn()) // camera is off
	{
		if (_debug)
		{
			_debug_port->println("turn on the camera first");
		}
		return false;
	}

	String request;

	if (_camera == HERO3)
	{
		request = _url + "DA?t=" + _pwd;
	}
	else if (_camera >= HERO4)
	{
		request = _url + "command/command/storage/delete/all";
	}

	return sendRequest(request);
}

////////////////////////////////////////////////////////////
////////                   Debug                   /////////
////////////////////////////////////////////////////////////

void GoProControl::enableDebug(HardwareSerial *debug_port, const uint32_t debug_baudrate)
{
	_debug = true;
	_debug_port = debug_port;
	_debug_port->begin(debug_baudrate);
}

void GoProControl::disableDebug()
{
	_debug_port->end();
	_debug = false;
}

uint8_t GoProControl::getStatus()
{
	return _connected;
}

void GoProControl::printStatus()
{
	_debug_port->print("\nSSID: ");
	_debug_port->println(WiFi.SSID());
	_debug_port->print("IP Address: ");
	_debug_port->println(WiFi.localIP());
	_debug_port->print("signal strength (RSSI):");
	_debug_port->print(WiFi.RSSI());
	_debug_port->println(" dBm\n");
	//WiFi.BSSID(bssid);
	//todo add more info like mode (photo, video), fow and so on
}

////////////////////////////////////////////////////////////
////////               Communication               /////////
////////////////////////////////////////////////////////////

uint8_t GoProControl::sendRequest(const String request)
{
	//_client.stop(); //is it needed?
	if (!_client.connect(_host.c_str(), _port))
	{
		if (_debug)
		{
			_debug_port->println("Connection lost");
		}
		_connected = false;
		return false;
	}

	_http.get(request);
	uint16_t response = _http.responseStatusCode();
	_last_request = millis();

	if (_debug)
	{
		_debug_port->println("My request: " + request);
		_debug_port->print("Response: ");
		_debug_port->println(response);
	}

	if (response == 200)
	{
		if (_debug)
		{
			_debug_port->println("Command: Accepted");
		}
		return true;
	}
	else if (response == 403)
	{
		if (_debug)
		{
			_debug_port->println("Command: Wrong password");
		}
		return -1;
	}
	else if (response == 410)
	{
		if (_debug)
		{
			_debug_port->println("Command: Failed");
		}
		return -2;
	}
	else
	{
		if (_debug)
		{
			_debug_port->println("Command: Unknown error");
		}
		return -3;
	}
}

void GoProControl::sendWoL(WiFiUDP udp, byte *mac, size_t size_of_mac)
{
	byte preamble[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	byte i;
	IPAddress addr(255, 255, 255, 255);
	udp.begin(9);
	udp.beginPacket(addr, 9); //sending packet at 9,

	udp.write(preamble, sizeof preamble);

	for (i = 0; i < 16; i++)
	{
		udp.write(mac, size_of_mac);
	}
	udp.endPacket();
	delay(2000);
}
