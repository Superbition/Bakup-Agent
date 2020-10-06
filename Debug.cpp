#include "Debug.h"

Debug::Debug(bool debugMode) {
    // Set the given debug mode for the class
    this->debug = debugMode;
}

void Debug::print(string debugString) {
    // If the program is running in debug mode
    if(this->debug)
    {
        // Print the given string
        cout << "[" << time(NULL) << "] " << debugString << endl;
    }
}

void Debug::setDebugMode(bool newDebugMode)
{
    this->debug = newDebugMode;
}
