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
#define MAX_NR_OF_KEYS 5 //!<1-5 keys can be sent per button press or coin insertion

enum Command {SET_COIN_REJECT, SET_BUTTON_SHORT, SET_BUTTON_LONG, SET_COIN, DUMP_CONFIG, BAD_COMMAND};
//SET_COIN_REJECT 'R' --> set coin rejection to off (0b) or on (> 0b)
//SET_BUTTON_SHORT 'S' --> set keys sent on short button press. followed by 1 byte button number and KEYS_NUMBER_OF bytes of key codes. unused codes must be 0!
//SET_BUTTON_LONG 'L' --> set keys sent on short button press. followed by 1 byte button number and KEYS_NUMBER_OF bytes of key codes. unused codes must be 0!
//SET_COIN 'C' --> set keys sent on coin insertion. followed by 1 byte coin number and KEYS_NUMBER_OF bytes of key codes. unused codes must be 0!
//DUMP_CONFIG 'D' --> dump version information and all configured data to serial port after waiting for a short while. for debug purposes.
#define COMMAND_OK "OK\n" //!<Response sent when a command is detected.
#define COMMAND_NOK "NK\n" //!<Response sent when the command or its arguments are not ok.
#define COMMAND_TERMINATOR 10 //!<Command terminator is LF aka '\n'

std::map<Command, uint8_t> commandMap; //!<Maps arduino commands to their serial terminal values.
std::map<std::string, uint8_t> keyNameMap; //!<Maps key name strings to their unsigned char value.

Command command = BAD_COMMAND; //!<The command that was passed on the command line.
std::vector<uint8_t> commandData; //string containing the whole command

std::string serialPortName = "/dev/ttyUSB0";

//---------------------------------------------------------------------------------------------------------------------------

void setup()
{
    //setup the values which are sent for every command onthe terminal
    commandMap[SET_COIN_REJECT] = 'R';
    commandMap[SET_BUTTON_SHORT] = 'S';
    commandMap[SET_BUTTON_LONG] = 'L';
    commandMap[SET_COIN] = 'C';
    commandMap[DUMP_CONFIG] = 'D';
    //set names for keys that are read from the command line and their value sent to the arduino
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
	std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "MAMEduino " << MAMEDUINO_VERSION_STRING << ConsoleStyle() << " - Configure the Arduino Leonardo MAME interface." << std::endl;
}

void printUsage()
{
    std::cout << std::endl;
    std::cout << "Usage: mameduino <SERIAL_DEVICE> <COMMAND>" << std::endl;
    std::cout << "Valid commands:" << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-r \"on\"|\"off\"" << ConsoleStyle() << " - Set coin rejection to on or off." << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-s BUTTON# KEY ..." << ConsoleStyle() << " - Set keyboard keys to send when button is SHORT-pressed."  << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-l BUTTON# KEY ..." << ConsoleStyle() << " - Set keyboard keys to send when button is LONG-pressed."  << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-c COIN# KEY ..." << ConsoleStyle() << " - Set keyboard keys to send when coin is inserted." << std::endl;
    std::cout << ConsoleStyle(ConsoleStyle::CYAN) << "-d " << ConsoleStyle() << " - Dump version and current configuration of Arduino program." << std::endl;
    std::cout << "Currently valid buttons: 0-4." << std::endl;
    std::cout << "Currently valid coins: 0-2." << std::endl;
    std::cout << "Up to 5 keys are supported. Special keys are referenced by their names: " << std::endl;
    std::cout << "  LCTRL, LSHIFT, LALT, LGUI, RCTRL, RSHIFT, RALT, RGUI," << std::endl;
    std::cout << "  UP, DOWN, LEFT, RIGHT, BACKSPACE, TAB, RETURN, ESC," << std::endl;
    std::cout << "  INSERT, DELETE, PAGEUP, PAGEDOWN, HOME, END, F1-F12" << std::endl;
    std::cout << "Also the reset and power pin/button can be accessed: " << std::endl;
    std::cout << "  PIN_RESET, PIN_POWER" << std::endl;
    std::cout << "It makes no sense to send more than one \"key press\" here..." << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "mameduino /dev/ttyUSB0 -r on (turn coin rejection on)" << std::endl;
    std::cout << "mameduino /dev/ttyS0 -s 0 UP UP LEFT (send cursor keys for button 0)" << std::endl;
    std::cout << "mameduino /dev/ttyS0 -c 2 b l a r g (send \"blarg\" for coin 2)" << std::endl;
}

bool readKeys(int argc, const char * argv[], int startIndex)
{
    if (startIndex < argc) {
        int argumentIndex = 0;
        for(int i = startIndex; i < argc && (i - startIndex) < 5;) {
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
    //first argument must be device
    std::string argument = argv[1];
    if (argument.at(0) == '/') {
        serialPortName = argument;
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
                std::stringstream tempstream(argv[i++]);
                tempstream >> buttonIndex;
                if (buttonIndex < 0 || buttonIndex > MAX_BUTTON_INDEX) {
                    std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Button index must be 0-" << MAX_BUTTON_INDEX << ", but was " << buttonIndex << "." << ConsoleStyle() << std::endl;
                    return false;
                }
                command = argument == "-s" ? SET_BUTTON_SHORT : SET_BUTTON_LONG;
                commandData.push_back(commandMap[command]);
                commandData.push_back(static_cast<uint8_t>(buttonIndex));
                //read next arguments: keys
                if (readKeys(argc, argv, i)) {
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
                std::stringstream tempstream(argv[i++]);
                tempstream >> coinIndex;
                if (coinIndex < 0 || coinIndex > MAX_COIN_INDEX) {
                    std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Coin index must be 0-" << MAX_COIN_INDEX << ", but was " << coinIndex << "." << ConsoleStyle() << std::endl;
                    return false;
                }
                command = SET_COIN;
				commandData.push_back(commandMap[command]);
				commandData.push_back(static_cast<uint8_t>(coinIndex));
                //read next arguments: keys
                if (readKeys(argc, argv, i)) {                    
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

bool writeToPort(const int serialPort, const unsigned char * data, const ssize_t size)
{
	//write bytes to the port
	ssize_t n = write(serialPort, data, size);
	if (n != size) {
		std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Failed write to serial port!" << ConsoleStyle() << std::endl;
		return false;
	}
	return true;
}

int main(int argc, const char * argv[])
{
	setup();

    printVersion();
    
    if (argc < 2 || !readArguments(argc, argv) || command == BAD_COMMAND) {
        printUsage();
        return -1;
    }

    std::cout << "Opening serial port " << serialPortName << " ..." << std::endl;
    int serialPort = open(serialPortName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (serialPort <= 0) {
		std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Failed to open serial port " << serialPortName << "!" << ConsoleStyle() << std::endl;
		return -2;
	}

	std::cout << "Setting serial port to 38400bps, 8N1 mode..." << std::endl;
	//store current terminal options
	termios oldOptions;
	tcgetattr(serialPort, &oldOptions);
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
	if (tcsetattr(serialPort, TCSANOW, &options) != 0) {
		std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Failed set serial port options!" << ConsoleStyle() << std::endl;
		close(serialPort);
		return -3;
	}

    //terminate command with a line break
    commandData.push_back(COMMAND_TERMINATOR);
    //write command to port
	std::cout << "Sending command to Arduino..." << std::endl;
	if (!writeToPort(serialPort, commandData.data(), commandData.size() * sizeof(uint8_t))) {
		tcsetattr(serialPort, TCSAFLUSH, &oldOptions);
		close(serialPort);
		return -4;
	}
	
	std::cout << "Waiting for response from Arduino..." << std::endl;
	
	if (command == DUMP_CONFIG) {
	    //read response from arduino
	    char buffer[256];
	    bool responseReceived = false;
	    while (!responseReceived) {
	        //sleep some time to transfer commands
    	    usleep(200*1000);
	        ssize_t bytesRead = read(serialPort, &buffer, sizeof(buffer) - 1);
	        if (bytesRead > 0) {
    	        //response received, null-terminate string
	    	    buffer[bytesRead] = '\0';
	    	    std::string response(buffer);
	    	    //std::cout << response;
	    	    //split string by '\n's
	    	    size_t pos = 0;
                while ((pos = response.find('\n')) != std::string::npos) {
                    std::string token = response.substr(0, pos + 1);
                    if (token == COMMAND_OK || token == COMMAND_NOK) {
                        responseReceived = true;
                        break;
                    }
                    else {
                        std::cout << token;
                        response.erase(0, pos + 1);
                    }
                }
	        }
	    }
	}
	else {
	    //sleep some time to transfer commands
	    usleep(200*1000);
	    //set block options
	    options.c_cc[VTIME] = 2; //for for 0.1s per character
        options.c_cc[VMIN] = 3; //blocking read until 3 chars received
        tcsetattr(serialPort, TCSANOW, &options);
	    //read response from arduino
	    char buffer[256];
	    ssize_t bytesRead = read(serialPort, &buffer, sizeof(buffer) - 1);
	    //std::cout << bytesRead << " bytes received." << std::endl;
	    if (bytesRead < 3) {
		    //reading failed or response too short
		    std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Bad response from Arduino!" << ConsoleStyle() << std::endl;
		    tcsetattr(serialPort, TCSAFLUSH, &oldOptions);
		    close(serialPort);
		    return -5;
	    }
	    else {
		    //response received, null-terminate string
		    buffer[bytesRead] = '\0';
		    std::string response(buffer);
		    //std::cout << "Arduino responded: \"" << response << "\"." << std::endl;
		    if (response == COMMAND_OK) {
			    std::cout << ConsoleStyle(ConsoleStyle::GREEN) << "Succeded." << ConsoleStyle() << std::endl;
		    }
		    else {
			    std::cout << ConsoleStyle(ConsoleStyle::RED) << "Error: Sending the command failed!" << ConsoleStyle() << std::endl;
			    tcsetattr(serialPort, TCSAFLUSH, &oldOptions);
			    close(serialPort);
			    return -5;
		    }
	    }
	}

	//restore old port settings
	tcsetattr(serialPort, TCSAFLUSH, &oldOptions);
	//close port and terminate
	close(serialPort);
	return 0;
}
