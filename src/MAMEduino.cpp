#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include "MAMEduino.h"
#include "consolestyle.h"

//---------------------------------------------------------------------------------------------------------------------------

#define MAX_BUTTON_INDEX 4 //!<button index 0-4 are supported
#define MAX_COIN_INDEX 2 //!<coin indices 0-2 are supported
#define MAX_NR_OF_KEYS 6 //!<1-6 keys can be sent per button press or coin insertion

enum Command {SET_COIN_REJECT, SET_BUTTON_SHORT, SET_BUTTON_LONG, SET_COIN, DUMP_CONFIG, CHECK_VERSION, BAD_COMMAND};
//SET_COIN_REJECT 'R' --> set coin rejection to off (0b) or on (> 0b)
//SET_BUTTON_SHORT 'S' --> set keys sent on short button press. followed by 1 byte button number and KEYS_NUMBER_OF bytes of key codes. unused codes must be 0!
//SET_BUTTON_LONG 'L' --> set keys sent on short button press. followed by 1 byte button number and KEYS_NUMBER_OF bytes of key codes. unused codes must be 0!
//SET_COIN 'C' --> set keys sent on coin insertion. followed by 1 byte coin number and KEYS_NUMBER_OF bytes of key codes. unused codes must be 0!
//DUMP_CONFIG 'D' --> dump version information and all configured data to serial port after waiting for a short while. for debug purposes.
//CHECK_VERSION '?' --> send version string to serial port. used by the PC side to find MAMEduino serial port.
const std::string COMMAND_OK = "OK\n"; //!<Response sent when a command is detected.
const std::string COMMAND_NOK = "NK\n"; //!<Response sent when the command or its arguments are not ok.
const char COMMAND_TERMINATOR = 10; //!<Command terminator is LF aka '\n'

std::map<Command, uint8_t> commandMap; //!<Maps arduino commands to their serial terminal values.
std::map<std::string, uint8_t> keyNameMap; //!<Maps key name strings to their unsigned char value.

Command command = BAD_COMMAND; //!<The command that was passed on the command line.
std::vector<uint8_t> commandData; //string containing the whole command

std::string serialPortName = ""; //!<Default serial port device name.
bool beVerbose = false; //!<Set to true to display more output.

bool autodetectPort = false; //!<Set to true to autodetect the port upon start.
const std::string possiblePortNames[] {
    "USB0", "USB1", "USB2", "USB3", "USB4", "USB5", "USB6", "USB7", "USB8", "USB9", 
    "ACM0", "ACM1", "ACM2", "ACM3", "ACM4", "ACM5", "ACM6", "ACM7", "ACM8", "ACM9", ""
}; //!<List of possible path name. Simplest method I guess...

//---------------------------------------------------------------------------------------------------------------------------

void setup()
{
    //setup the values which are sent for every command onthe terminal
    commandMap[SET_COIN_REJECT] = 'R';
    commandMap[SET_BUTTON_SHORT] = 'S';
    commandMap[SET_BUTTON_LONG] = 'L';
    commandMap[SET_COIN] = 'C';
    commandMap[DUMP_CONFIG] = 'D';
    commandMap[CHECK_VERSION] = '?';
    //set names for keys that are read from the command line and their value sent to the arduino
    keyNameMap["CLEAR"] = 0; //clear all key bindings for a press mode of a button
    keyNameMap["LCTRL"] = 128;
    keyNameMap["LSHIFT"] = 129;
    keyNameMap["LALT"] = 130;
    keyNameMap["LGUI"] = 131;
    keyNameMap["RCTRL"] = 132;
    keyNameMap["RSHIFT"] = 133;
    keyNameMap["RALT"] = 134;
    keyNameMap["RGUI"] = 135;
    keyNameMap["UP"] = 218;
    keyNameMap["DOWN"] = 217;
    keyNameMap["LEFT"] = 216;
    keyNameMap["RIGHT"] = 215;
    keyNameMap["BACKSPACE"] = 178;
    keyNameMap["TAB"] = 179;
    keyNameMap["RETURN"] = 176;
    keyNameMap["ESC"] = 177;
    keyNameMap["INSERT"] = 209;
    keyNameMap["DELETE"] = 212;
    keyNameMap["PAGEUP"] = 211;
    keyNameMap["PAGEDOWN"] = 214;
    keyNameMap["HOME"] = 210;
    keyNameMap["END"] = 213;
    keyNameMap["F1"] = 194;
    keyNameMap["F2"] = 195;
    keyNameMap["F3"] = 196;
    keyNameMap["F4"] = 197;
    keyNameMap["F5"] = 198;
    keyNameMap["F6"] = 199;
    keyNameMap["F7"] = 200;
    keyNameMap["F8"] = 201;
    keyNameMap["F9"] = 202;
    keyNameMap["F10"] = 203;
    keyNameMap["F11"] = 204;
    keyNameMap["F12"] = 205;
    keyNameMap["PIN_RESET"] = 254;
    keyNameMap["PIN_POWER"] = 255;
}

void printVersion()
{
	std::cout << ConsoleStyle(ConsoleStyle::GREEN) << "MAMEduino " << MAMEDUINO_VERSION_STRING << ConsoleStyle() << " - The Arduino Leonardo MAME interface" << std::endl;
}

void printUsage()
{
    std::cout << "Usage:" << ConsoleStyle(ConsoleStyle::CYAN) << " mameduino <SERIAL_DEVICE> <COMMAND>" << ConsoleStyle() << std::endl;
    std::cout << "SERIAL_DEVICE should be e.g. " << ConsoleStyle(ConsoleStyle::CYAN) << "/dev/ttyACM0" << ConsoleStyle() << 
                 ", or use " << ConsoleStyle(ConsoleStyle::CYAN) << "-a" << ConsoleStyle() << " to auto-detect it." << std::endl;    
    std::cout << "Valid commands:" << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-r \"on\"|\"off\"" << ConsoleStyle() << " - Set coin rejection to on or off." << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-s BUTTON# KEY ..." << ConsoleStyle() << " - Set keyboard keys to send when button is SHORT-pressed."  << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-l BUTTON# KEY ..." << ConsoleStyle() << " - Set keyboard keys to send when button is LONG-pressed."  << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-c COIN# KEY ..." << ConsoleStyle() << " - Set keyboard keys to send when coin is inserted." << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-d" << ConsoleStyle() << " - Dump version and current configuration of Arduino program." << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-h/-?/--help" << ConsoleStyle() << " - Show this help." << std::endl;
    std::cout << "Currently valid buttons: 0-" << MAX_BUTTON_INDEX << "." << std::endl;
    std::cout << "Currently valid coins: 0-" << MAX_COIN_INDEX << "." << std::endl;
    std::cout << "Up to " << MAX_NR_OF_KEYS << " keys are supported. Special keys are referenced by their names: " << std::endl;
    std::cout << "  LCTRL, LSHIFT, LALT, LGUI, RCTRL, RSHIFT, RALT, RGUI," << std::endl;
    std::cout << "  UP, DOWN, LEFT, RIGHT, BACKSPACE, TAB, RETURN, ESC," << std::endl;
    std::cout << "  INSERT, DELETE, PAGEUP, PAGEDOWN, HOME, END, F1-F12" << std::endl;
    std::cout << "The reset and power pin/button can be accessed with PIN_RESET and PIN_POWER." << std::endl;
    std::cout << "Your can clear key bindings for a button/coin with the keyword CLEAR." << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "mameduino -a -r on" << ConsoleStyle() << " (auto-detect serial port, turn coin rejection on)" << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "mameduino /dev/ttyS0 -s 0 UP LEFT" << ConsoleStyle() << " (set cursor keys for button 0, short press)" << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "mameduino /dev/ttyACM0 -l 1 CLEAR" << ConsoleStyle() << " (remove all keys for button 1, long press)" << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "mameduino /dev/ttyS0 -l 3 PIN_POWER" << ConsoleStyle() << " (pulse power pin for button 1, long press)" << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "mameduino /dev/ttyUSB0 -c 2 b l a h r g" << ConsoleStyle() << " (send \"blahrg\" for coin 2)" << std::endl;
}

bool readKeys(int argc, const char * argv[], int startIndex)
{
    if (startIndex < argc) {
        int argumentIndex = 0;
        for(int i = startIndex; i < argc && (i - startIndex) < MAX_NR_OF_KEYS;) {
            //check if key is a single character or a modifier etc.
            std::string key = argv[i++];
            if (key.size() > 1) {
                //elaborate key. check map
                auto keyIt = keyNameMap.find(key);
                if (keyIt != keyNameMap.cend()) {
                    //found. append to command arguments
                    commandData.push_back(keyIt->second);
                    argumentIndex++;
                }
                else {
                    //not found. complain to user
                    std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Unknown key name \"" << key << "\"." << ConsoleStyle() << std::endl;
                    return false;
                }
            }
            else {
                //simple character. append to command arguments
                commandData.push_back(key.at(0));
                argumentIndex++;
            }
        }
        return true;
    }
    else {
        std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: No key presses specified." << ConsoleStyle() << std::endl;
    }
    return false;
}

bool readArguments(int argc, const char * argv[])
{
    //first argument must be device, autodetect or help command
    std::string argument = argv[1];
    if (argument.length() > 5 && argument.substr(0, 5) == "/dev/") {
        //serial port passed on command line
        serialPortName = argument;
    }
    else if (argument == "-a") {
        //autodetect command passed
        autodetectPort = true;
    }
    else if (argument == "-?" || argument == "-h" || argument == "--help") {
        //help command passed
        printUsage();
        exit(0);
    }
    else {
        std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: First argument must be a serial port device string." << ConsoleStyle() << std::endl;
        return false;
    }
    for(int i = 2; i < argc;) {
        //read argument from list
        argument = argv[i++];
        //check what it is
        if (argument == "-d") {
            command = DUMP_CONFIG;
			commandData.push_back(commandMap[command]);
			return true;
        }
        else if (argument == "-r") {
            //czeck if we have another argument
            if (i < argc) {
                //read next argument: "on" or "off"
                std::string onoff = argv[i++];
                if (onoff != "on" && onoff != "off") {
                    std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Button reject argument was \"" << onoff << "\", but must be \"on\" or \"off\"." << ConsoleStyle() << std::endl;
                    return false;
                }
                command = SET_COIN_REJECT;
				commandData.push_back(commandMap[command]);
				commandData.push_back(onoff == "on" ? 1 : 0);
                return true;
            }
            else {
                std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Button reject argument missing." << ConsoleStyle() << std::endl;
            }
        }
        else if (argument == "-s" || argument == "-l") {
            //check if we have at least two more arguments
            if ((i + 1) < argc) {
                //read next argument: button index
                int buttonIndex;
                std::istringstream tempStream(argv[i]);
                if (!(tempStream >> buttonIndex) || buttonIndex < 0 || buttonIndex > MAX_BUTTON_INDEX) {
                    std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Button index must be 0-" << MAX_BUTTON_INDEX << ", but was " << argv[i] << "." << ConsoleStyle() << std::endl;
                    return false;
                }
                command = argument == "-s" ? SET_BUTTON_SHORT : SET_BUTTON_LONG;
                commandData.push_back(commandMap[command]);
                commandData.push_back(static_cast<uint8_t>(buttonIndex));
                //read next arguments: keys
                if (readKeys(argc, argv, ++i)) {
                    return true;
                }
            }
            else {
                std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Too few arguments for command (two are needed)." << ConsoleStyle() << std::endl;
            }
        }
        else if (argument == "-c") {
            //check if we have at least two more arguments
            if ((i + 1) < argc) {
                //read next argument: coin index
                int coinIndex;
                std::istringstream tempStream(argv[i]);
                if (!(tempStream >> coinIndex) || coinIndex < 0 || coinIndex > MAX_COIN_INDEX) {
                    std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Coin index must be 0-" << MAX_COIN_INDEX << ", but was " << argv[i] << "." << ConsoleStyle() << std::endl;
                    return false;
                }
                command = SET_COIN;
				commandData.push_back(commandMap[command]);
				commandData.push_back(static_cast<uint8_t>(coinIndex));
                //read next arguments: keys
                if (readKeys(argc, argv, ++i)) {                    
                    return true;
                }
            }
            else {
                std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Too few arguments for command (two are needed)." << ConsoleStyle() << std::endl;
            }
        }
        else {
            std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Unknown command \"" << argument << "\"!" << ConsoleStyle() << std::endl;
            return false;
        }
    }
    return false;
}

bool serialPortExists(const std::string & portName)
{
    int portHandle = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (portHandle <= 0) {
        return false;
    }
    close(portHandle);
    return true;
}

bool openSerialPort(int & portHandle, const std::string & portName, termios * oldOptions)
{
    //try opening serial port
    if (beVerbose) {
        std::cout << "Opening serial port " << portName << " ..." << std::endl;
    }
    portHandle = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (portHandle <= 0) {
		std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Failed to open serial port " << portName << "!" << ConsoleStyle() << std::endl;
		return false;
	}
	//no set serial port to proper settings
	if (beVerbose) {
    	std::cout << "Setting serial port to 38400bps, 8N1 mode..." << std::endl;
    }
	//store current terminal options
	tcgetattr(portHandle, oldOptions);
	//clear new terminal options
	termios options;
	memset(&options, 0, sizeof(termios));
	//set baud rate to 38400 Baud
	cfsetispeed(&options, B38400);
    cfsetospeed(&options, B38400);
    //set mode to 8N1
    options.c_cflag |= (CLOCAL | CREAD); //Enable the receiver and set local mode
    options.c_cflag &= ~PARENB; //no parity
    options.c_cflag &= ~CSTOPB; //one stop bit
    options.c_cflag &= ~CSIZE; //size mask flag
    options.c_cflag |= CS8; //8 bit
    //set raw output
    options.c_oflag &= ~OPOST;
    //set input mode (non-canonical, no echo)
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //turn parity off
    //options.c_iflag = IGNPAR;
    //flush serial port
    //tcflush(serialPort, TCIFLUSH);
	//set terminal options
	if (tcsetattr(portHandle, TCSANOW, &options) != 0) {
		std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Failed set serial port options!" << ConsoleStyle() << std::endl;
		close(portHandle);
		return false;
	}
	return true;
}

void closeSerialPort(const int portHandle, const termios * oldOptions)
{
	//restore old port settings
	tcsetattr(portHandle, TCSAFLUSH, oldOptions);
	//close port and terminate
	close(portHandle);
}

bool writeToSerialPort(const int portHandle, const unsigned char * data, const ssize_t size)
{
	//write bytes to the port
	ssize_t n = write(portHandle, data, size);
	if (n != size) {
		std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Failed write to serial port!" << ConsoleStyle() << std::endl;
		return false;
	}
	return true;
}

bool getResponseFromSerial(const int portHandle, std::string & response, int waitTimeMs = 200)
{
    //clear response string
    response.clear();
    //start reading from serial port
    char buffer[256];
    bool responseReceived = false;
    int waitStep = 50;
    std::string combinedResponse;
    while (!responseReceived && waitTimeMs > 0) {
        //sleep some time to transfer commands
        usleep(waitStep*1000);
        ssize_t bytesRead = read(portHandle, &buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            //response received, null-terminate string and add to combined response
    	    buffer[bytesRead] = '\0';
    	    combinedResponse += buffer;
    	    //check if the message is long enough
        	if (combinedResponse.length() >= COMMAND_OK.length()) {
                //check if the end of our message is OK\n or NK\n
         	    if (combinedResponse.substr(combinedResponse.length() - COMMAND_OK.length(), COMMAND_OK.length()) == COMMAND_OK) {
         	        //remove end from response and return
         	        response = combinedResponse.substr(0, combinedResponse.length() - COMMAND_OK.length());
         	        return true;
                }
                else if (combinedResponse.substr(combinedResponse.length() - COMMAND_NOK.length(), COMMAND_NOK.length()) == COMMAND_NOK) {
                    return false;
                }
            }
        }
        waitTimeMs -= waitStep;
    }
    return false;
}

int main(int argc, const char * argv[])
{
	setup();

    printVersion();
    
    if (argc < 2 || !readArguments(argc, argv) || command == BAD_COMMAND) {
        std::cout << std::endl;
        printUsage();
        return -1;
    }

    termios oldOptions;
    int portHandle = -1;
    if (autodetectPort) {
        //try to autodetect port. get post names from list
        std::string portNameCandidate;
        for (int i = 0; !possiblePortNames[i].empty() && serialPortName.empty(); ++i) {
            //try to open port
            portNameCandidate = "/dev/tty" + possiblePortNames[i];
            if (serialPortExists(portNameCandidate) && openSerialPort(portHandle, portNameCandidate, &oldOptions)) {
                //opening worked. send version command
                std::vector<uint8_t> versionCommand;
                versionCommand.push_back('?');
                versionCommand.push_back(COMMAND_TERMINATOR);
                if (writeToSerialPort(portHandle, versionCommand.data(), versionCommand.size() * sizeof(uint8_t))) {
                    //sending worked. receive response.                           
                    std::string response;
                    if (getResponseFromSerial(portHandle, response) && response.substr(0, 10) == "MAMEduino ") {
                        if (beVerbose) {
                            std::cout << ConsoleStyle(ConsoleStyle::GREEN) << response << " found at " << portNameCandidate << "." << ConsoleStyle() << std::endl;
                        }
		                serialPortName = portNameCandidate;
                    }
                }
                closeSerialPort(portHandle, &oldOptions);
            }
        }
        if (serialPortName.empty()) {
            std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Failed to auto-detect serial port!" << ConsoleStyle() << std::endl;
            return -2;
        }
    }

    //open port with port name given on command line or filled by auto-detection
    if (!openSerialPort(portHandle, serialPortName, &oldOptions)) {
        return -2;
    }
    
    if (beVerbose) {
    	std::cout << "Sending command to Arduino..." << std::endl;
    }
    //terminate command with a line break
    commandData.push_back(COMMAND_TERMINATOR);
    //write command to port
	if (!writeToSerialPort(portHandle, commandData.data(), commandData.size() * sizeof(uint8_t))) {
		closeSerialPort(portHandle, &oldOptions);
		return -3;
	}
	
	if (beVerbose) {
    	std::cout << "Waiting for response from Arduino..." << std::endl;
    }
    //read response from arduino
    std::string response;
    if (getResponseFromSerial(portHandle, response)) {
        if (command == DUMP_CONFIG) {
            std::cout << response;
        }
        std::cout << ConsoleStyle(ConsoleStyle::GREEN) << "Command succeded." << ConsoleStyle() << std::endl;
    }
    else {
        std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Sending the command failed!" << ConsoleStyle() << std::endl;
        closeSerialPort(portHandle, &oldOptions);
        return -4;
    }

    //close port	
	closeSerialPort(portHandle, &oldOptions);

	return 0;
}
