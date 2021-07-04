#ifndef BAKUP_AGENT_RCLONEVERSIONCHECKER_H
#define BAKUP_AGENT_RCLONEVERSIONCHECKER_H
#include <string>
#include <cstdio>
#include <iostream>

using namespace std;

class RCloneVersionChecker {
    public:
        RCloneVersionChecker();

        // Get rclone's version
        static string getRCloneVersion();
};


#endif //BAKUP_AGENT_RCLONEVERSIONCHECKER_H
