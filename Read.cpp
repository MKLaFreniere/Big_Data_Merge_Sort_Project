// Mark LaFreniere
// Creation: 11/23/2021
// Modified: 12/9/2021
// Read value from a key in sorted runs
#include<string>
#include<iostream>
#include<cstring>
#include<algorithm>
#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<map>
#include<array>
#include<dirent.h>
#include<vector>

using namespace std;

void handle_error(const char*);
vector<string> GetSortedRuns();
string ConvertToString(char[]);
void FindKey(string);
string BinarySearch(string, int, int, vector<string>, int);

int main() {

    string input;

    cout << "Enter a READ transaction file: ";
    cin >> input;

    FindKey(input);

    return 0;
}

// handle various errors
void handle_error(const char* msg) {
    perror(msg);
    exit(255);
}

// get file names of sorted runs
vector<string> GetSortedRuns() {
    vector<string> tmpVector;
    string tmp;
    DIR *dir;
    struct dirent *ent;
    // open directory where files are located
    if ((dir = opendir (".")) != NULL) {
        // print all the files and directories within directory 
        while ((ent = readdir (dir)) != NULL) {
            // sort out bad file names before push (only allow kv)
            tmp = ConvertToString(ent->d_name);
            if(tmp.size() >= 3) {
                if(tmp.substr(0,2) == "kv") {
                    tmpVector.push_back(tmp);
                }
            }
        }
        closedir (dir); 
    }
    return tmpVector;
}

// function for converting a char array to a string
string ConvertToString(char cstr[]) {
    string nstr(cstr);
    return nstr;
}

// find and return a value associated with a key in the sorted runs
void FindKey(string readFileName) {

    vector<string> dataFiles = GetSortedRuns();
    const int RD_LINE_SIZE = 37;
    const int RD_BUFFER_SIZE = RD_LINE_SIZE * 216;
    char buf[RD_BUFFER_SIZE + 1];
    vector<char> searchingLine; 
    int lineIdentifier = 0;
    char currentCharLine[RD_LINE_SIZE];
    string currentLine;
    string result;

    // open read transaction file for reading
    int fd = open(readFileName.c_str(), O_RDONLY);
    if (fd == -1)
        handle_error("open");

    // read file to buf
    posix_fadvise(fd, 0, 0, 1);  // FDADVICE_SEQUENTIAL

    size_t bytes_read = read(fd, buf, RD_BUFFER_SIZE);

    // each iteration searches for one key
    for(int i = 0; i < RD_BUFFER_SIZE; i += RD_LINE_SIZE) {

        // read current key for this iteration and save to char
        for(int j = 0; j < 36; j++){
            currentCharLine[j] = (buf[j + (RD_LINE_SIZE * lineIdentifier)]);
        }

        // convert char to string for easy comparison
        currentLine = (ConvertToString(currentCharLine)).substr(4,37);

        // binary search
        result = BinarySearch(currentLine, dataFiles.size() - 1, 0, dataFiles, 0);

        // return whether a key was found
        if (result == "-1") {
            cout << "  Key not found in the database" << endl << endl;
        }
        else {
            cout << "Value found:  " << result << endl;
        }

        // increment identifier to get next line
        lineIdentifier++;
    }

}

// binary search the sorted runs to find the matching key
string BinarySearch(string searchKey, int high, int low, vector<string> fileNames, int prevMid) {

    int fdsearch;
    static const size_t KEY_LINE_SIZE = 32;
    static const size_t FD_LINE_SIZE = 66;
    static const auto FD_BUFFER_SIZE = FD_LINE_SIZE * 1024;
    char sbuf[KEY_LINE_SIZE + 1];
    char currentCharSearchLine[KEY_LINE_SIZE + 1];
    string currentSearchLine;
    string tmpFileName;
    size_t bytes_read;

    // find what file the key would be located in
    if (high >= low) {

        // calculate midpoint
        int mid = low + (high - low) / 2;

        tmpFileName = fileNames.at(mid);
        fdsearch = open(tmpFileName.c_str(), O_RDONLY);
        if (fdsearch == -1) {
            handle_error("open");
        }
            
        // Advise the kernel of our access pattern.
        posix_fadvise(fdsearch, 0, 0, 1);

        bytes_read = read(fdsearch, sbuf, KEY_LINE_SIZE);

        // read first line
        for (int j = 0; j < KEY_LINE_SIZE; j++) { 
            currentCharSearchLine[j] = sbuf[j];
        }

        close(fdsearch);

        currentSearchLine = (ConvertToString(currentCharSearchLine)).substr(0,32);

        // use recursion for binary search to narrow down what file the key is in
        if(currentSearchLine > searchKey) {
            return BinarySearch(searchKey, mid - 1, low, fileNames, mid);
        }
        else {
            return BinarySearch(searchKey, high, mid + 1, fileNames, mid);
        }

    }



    // This part happens if a matched file has been found, after about a 3rd of files are read recursively
    // First line of the matched file must be read again. 
    // This is because if the key we are looking for is lower than the first
    // line of the matched file, then the key will actually be in the previous file.
    tmpFileName = fileNames.at(prevMid);
    fdsearch = open(tmpFileName.c_str(), O_RDONLY);
    if (fdsearch == -1) {
        handle_error("open");
    }
            
    // Advise the kernel of our access pattern.
    posix_fadvise(fdsearch, 0, 0, 1);

    bytes_read = read(fdsearch, sbuf, KEY_LINE_SIZE);

    // read first line
    for (int j = 0; j < KEY_LINE_SIZE; j++) { 
        currentCharSearchLine[j] = sbuf[j];
    }

    close(fdsearch);

    currentSearchLine = (ConvertToString(currentCharSearchLine)).substr(0,32);

    // see if search key is lower than matched file. Decrement file identifier if so. 
    if(currentSearchLine > searchKey) {
        prevMid--;
    }

    char ebuf[FD_BUFFER_SIZE + 1];
    char finalCharSearchLine[FD_LINE_SIZE];
    int lineIdentifier = 0;

    // Now open full file where key could possibly be located
    tmpFileName = fileNames.at(prevMid);
    fdsearch = open(tmpFileName.c_str(), O_RDONLY);
    if (fdsearch == -1)
        handle_error("open");

    posix_fadvise(fdsearch, 0, 0, 1);  // FDADVICE_SEQUENTIAL

    bytes_read = read(fdsearch, ebuf, FD_BUFFER_SIZE);

    cout << "Searched for: " << searchKey << endl;

    // read each line and compare key with linear search
    for(int i = 0; i < FD_BUFFER_SIZE; i += FD_LINE_SIZE) {

        for(int j = 0; j < FD_LINE_SIZE; j++) {
            finalCharSearchLine[j] = ebuf[j + (FD_LINE_SIZE * lineIdentifier)];
        }

        currentSearchLine = (ConvertToString(finalCharSearchLine)).substr(0,32);

        // if a match is found, return value and close the file
        if(searchKey == currentSearchLine) {
            string finalValue = (ConvertToString(finalCharSearchLine)).substr(33,66);
            close(fdsearch);
            return finalValue;
        }

        lineIdentifier++;
    }

    // if no match found, just close file and return message
    close(fdsearch);
    return "-1";
}