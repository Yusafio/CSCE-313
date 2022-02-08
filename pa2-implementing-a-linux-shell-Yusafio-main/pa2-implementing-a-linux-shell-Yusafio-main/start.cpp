#include <stdio.h>
#include<iostream>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
using namespace std;

vector<string> parseSpace (string str);
vector<string> parsePipe(string str);
string trimQuotes(string s);

int main (){

    int STDIN = dup(0);
    int STDOUT = dup(1);

    vector<int> backgroundPIDs;
    string prevDir = get_current_dir_name();
    string oldDir;

    while (true){

        //check background processes
        int pidReturned;
        for(int i = 0; i < backgroundPIDs.size(); i++) {
            pidReturned = waitpid(backgroundPIDs[i],0, WNOHANG);

            if (pidReturned > 0) {
                cout << pidReturned << " finished" << endl;

            }
            else if (pidReturned == -1) {
                backgroundPIDs.erase(backgroundPIDs.begin() + i);
                i--;
            }
        }

        string username = getenv("USER");   //get username and time

        time_t curr_time;
        tm * curr_tm;
        char timeString[100];

        time(&curr_time);
        curr_tm = localtime(&curr_time);

        strftime(timeString, 50, "%c", curr_tm);

        string currDir = get_current_dir_name();

        dup2(STDIN, 0); //reset to std in out
        dup2(STDOUT,1);

        cout << username + " " + timeString + ":~" + currDir +"$ ";
        string inputline;
        getline (cin, inputline);   // get a line from standard input
        if (inputline == string("")){
            continue;
        }
        if (inputline == string("exit")){
            cout << "Bye!! End of shell" << endl;
            break;
        }

        vector<string> parsedPipeString = parsePipe(inputline);     //split by pipes

        vector<vector<string>> vectorOfVectors;
        vector<string> parsedSpaces;

        for (int i = 0; i < parsedPipeString.size(); i++) {     //split by spaces
            parsedSpaces = parseSpace(parsedPipeString[i]);
            vectorOfVectors.push_back(parsedSpaces);
        }


        for (int i = 0; i < vectorOfVectors.size(); i++) {
            for (int j = 0; j< vectorOfVectors[i].size(); j++) {
                vectorOfVectors[i][j] = trimQuotes(vectorOfVectors[i][j]);
            }
        }

        //check for cd
        if (vectorOfVectors[0][0] == (string) "cd") {
            if (vectorOfVectors[0][1] == "-") {
                oldDir = get_current_dir_name();
                chdir(prevDir.c_str());
                prevDir = oldDir;
            }
            else {
                if (prevDir != get_current_dir_name()) {
                    prevDir = get_current_dir_name();
                }
                chdir((char *) vectorOfVectors[0][1].c_str());
            }
        }

        // >1 means pipe found
        else if(vectorOfVectors.size() > 1) {

            for (int i = 0; i < vectorOfVectors.size(); i++) {


                //prep args for > and <

                vector<string> newArgs;

                bool write = false;
                string writename = "";

                bool read = false;
                string readname = "";

                for (int k = 0; k < vectorOfVectors[i].size(); k++) {
                    if (vectorOfVectors[i][k] == (string) ">") {
                        k++;
                        write = true;
                        writename = vectorOfVectors[i][k];
                    }
                    else if (vectorOfVectors[i][k] == (string) "<"){
                        k++;
                        read = true;
                        readname = vectorOfVectors[i][k];
                    }
                    else {
                        string toAdd = vectorOfVectors[i][k];
                        newArgs.push_back(toAdd);
                    }
                }


                char* args2[newArgs.size() + 1];
                for (int i = 0; i < newArgs.size(); i++) {
                    args2[i] = (char *) newArgs[i].c_str();
                }

                args2[newArgs.size()] = NULL;

                int fd [2];
                pipe(fd);

                int pid = fork();

                if(pid == 0) {

                    close (fd[0]);
                    if (i < vectorOfVectors.size() - 1) {
                        dup2(fd[1], 1);
                    }


                    // open file for writing
                    if (write) {
                        char* filename = (char*) writename.c_str();
                        int fdWR = open (filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        dup2(fdWR, 1);

                    }
                    //open file for reading
                    if (read) {
                        char* filename = (char*) readname.c_str();
                        int fdRD = open (filename, O_RDONLY, 0);
                        dup2(fdRD, 0);
                    }
                    

                    execvp(args2[0], args2);


                }
                else {
                    if (i == vectorOfVectors.size() - 1) {
                        waitpid(pid, 0, 0);
                    }

                    dup2(fd[0], 0);
                    close (fd[1]);

                }

            }
        }
        else {

            //no pipes

            char* args[vectorOfVectors[0].size() + 1];

            for (int i = 0; i < vectorOfVectors[0].size(); i++) {
                args[i] = (char *) vectorOfVectors[0][i].c_str();
            }

            args[vectorOfVectors[0].size()] = NULL;

            //check if we run in background
            if ((string) args[vectorOfVectors[0].size() - 1] == (string)"&") {
                args[vectorOfVectors[0].size() - 1] = NULL;

                int pid = fork ();
                if (pid == 0) {
                    execvp(args[0], args);
                }
                else {
                    backgroundPIDs.push_back(pid);
                }
            }
            else {

                vector<string> newArgs;

                bool write = false;
                string writename = "";

                bool read = false;
                string readname = "";

                for (int k = 0; k < vectorOfVectors[0].size(); k++) {
                    if (vectorOfVectors[0][k] == (string) ">") {
                        k++;
                        write = true;
                        writename = vectorOfVectors[0][k];
                    }
                    else if (vectorOfVectors[0][k] == (string) "<"){
                        k++;
                        read = true;
                        readname = vectorOfVectors[0][k];
                    }
                    else {
                        string toAdd = vectorOfVectors[0][k];
                        newArgs.push_back(toAdd);
                    }
                }


                char* args2[newArgs.size() + 1];
                for (int i = 0; i < newArgs.size(); i++) {
                    args2[i] = (char *) newArgs[i].c_str();
                }

                args2[newArgs.size()] = NULL;


                int pid = fork ();
                if (pid == 0){ //child process
                    // preparing the input command for execution

                    // open file for writing
                    if (write) {
                        char* filename = (char*) writename.c_str();
                        int fdWR = open (filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        dup2(fdWR, 1);

                    }
                    //open file for reading
                    if (read) {
                        char* filename = (char*) readname.c_str();
                        int fdRD = open (filename, O_RDONLY, 0);
                        dup2(fdRD, 0);
                    }

                    
                    
                    execvp(args2[0], args2);

                }else{
                    waitpid (pid, 0, 0); // wait for the child process 
                    // we will discuss why waitpid() is preferred over wait()
                }

            }

        }
    }
}

vector<string> parseSpace (string str) {

    string token = "";
    vector<string> stringVector;
    int singleQuoteCount = 0;
    int doubleQuoteCount = 0;

    for (int i = 0; i< str.length(); i++) {

        if (str[i] == '\'') {
            singleQuoteCount++;
        }
        if (str[i] == '\"') {
            doubleQuoteCount++;
        }

        if (str[i] == ' ' && singleQuoteCount%2 == 0 && doubleQuoteCount%2 == 0) {
            if (token != "") {
                stringVector.push_back(token);
            }
            token = "";
        }
        else {
            token.append(str.substr(i, 1));
        }

    }


    if (token != "") {
        stringVector.push_back(token);
    }

    return stringVector;

}

vector<string> parsePipe(string str) {

    string token = "";
    vector<string> stringVector;
    int singleQuoteCount = 0;
    int doubleQuoteCount = 0;

    for (int i = 0; i< str.length(); i++) {

        if (str[i] == '\'') {
            singleQuoteCount++;
        }
        if (str[i] == '\"') {
            doubleQuoteCount++;
        }


        if (str[i] == '|' && singleQuoteCount%2 == 0 && doubleQuoteCount%2 == 0) {
            stringVector.push_back(token);
            token = "";
        }
        else {
            token.append(str.substr(i, 1));
        }

    }



    if (token != "") {
        stringVector.push_back(token);
    }

    return stringVector;
    
}

string trimQuotes(string s) {
    if(s[0] == '\"') {
        s = s.substr(1, s.length() - 2);
    }
    else if (s[0] == '\'') {
        s = s.substr(1, s.length() - 2);
    }
    return s;
}