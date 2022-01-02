// Mark LaFreniere
// Creation: 11/23/2021
// Modified: 12/9/2021
// Merge sort and storing of an input file transaction
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
#include<chrono>

using namespace std;

string ConvertToString(char[]);
void handle_error(const char*);
static void wc(char const*);
vector<string> GetSortedRuns();
void MergeSort();

int main() {

    string input;

    cout << "Enter a WRITE transaction file: ";
    cin >> input;

    // write sorted runs for new write transaction
    wc(input.c_str());

    // Completely sort all partially sorted runs
    MergeSort();

    cout << "New transaction file successfully written" << endl;
    return 0;
}

// function for converting a char array to a string
string ConvertToString(char cstr[]) {
    string nstr(cstr);
    return nstr;
}

// handle various errors
void handle_error(const char* msg) {
    perror(msg);
    exit(255);
}

// read write transaction file and create needed sorted runs
static void wc(char const* fname)
{
    int fileNumber = 1;
    string fileName = "tmp";
    string fullFileName;
    map<array<char,32>,array<char,32>> readMap;
    array<char,32> key;
    array<char,32> value;

    // declare const buffer for reading transactions
    static const size_t LINE_SIZE = 70;
    static const auto BUFFER_SIZE = LINE_SIZE * 1024;

    // declare const buffer for writing sorted runs
    static const size_t WRITE_LINE_SIZE = 66;
    static const auto WRITE_BUFFER_SIZE = WRITE_LINE_SIZE * 1024;

    int fd = open(fname, O_RDONLY);
    if (fd == -1)
        handle_error("open");

    /* Advise the kernel of our access pattern.  */
    posix_fadvise(fd, 0, 0, 1);  // FDADVICE_SEQUENTIAL

    // declare transaction file
    char buf[BUFFER_SIZE + 1];
    uintmax_t lines = 0;
    int outputCursor;

    // declarations for writing
    char sortedBuf[WRITE_BUFFER_SIZE + 1];
    int fdw;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    while (size_t bytes_read = read(fd, buf, BUFFER_SIZE))
    {
        if (bytes_read == (size_t)-1)
            handle_error("read failed");
        if (!bytes_read)
            break;

        // iterate through each line
        for (int i = 0; i < bytes_read; i += LINE_SIZE) {
            // buf[i + 4] start of each line's key
            // buf[i + 37] start of each line's value

            // get key from data line
            for(int j = 0; j < 32; j++){
                key[j]=buf[i + j + 4];
            }
            // get value from data line
            for(int j = 0; j < 32; j++){
                value[j]=buf[i + j + 37];
            }

            // insert key and value into map
            readMap[key] = value;
        }

        // reset sortedBuf's output cursor
        outputCursor = 0;
        // copy map to writing buffer
        for (auto it = readMap.cbegin(); it != readMap.cend(); ++it)
        {
            // input sorted map of key into sorted buf
            for (int j = 0; j < 32; j++) { 
                sortedBuf[outputCursor++] = it->first[j];
            }

            sortedBuf[outputCursor++] = ' ';
            
            // input sorted map of value into sorted buf
            for (int j = 0; j < 32; j++) { 
                sortedBuf[outputCursor++] = it->second[j];
            } 

            sortedBuf[outputCursor++] = '\n';
        }

        // open sorted run of 1024 bytes to a tmp file 
        fullFileName = fileName + to_string(fileNumber);
        fdw = open(fullFileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, mode);

        // write contents of sortedBuf to tmp file
        write(fdw, sortedBuf, WRITE_BUFFER_SIZE);

        // sync changes
        fsync(fdw);

        // close file
        close(fdw);

        // increment file number identifier
        fileNumber++;

        // clear map for next buf read
        readMap.clear();

        for (char* p = buf; (p = (char*)memchr(p, '\n', (buf + bytes_read) - p)); ++p)
            ++lines;
    }

    // close transaction file
    close(fd);
    return;
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
            // sort out bad file names before push (only allow tmp and kv)
            tmp = ConvertToString(ent->d_name);
            if(tmp.size() >= 3) {
                if((tmp.substr(0,3) == "tmp") || (tmp.substr(0,2) == "kv")) {
                    tmpVector.push_back(tmp);
                }
            }
        }
        closedir (dir); 
    }
    return tmpVector;
}

// merge sort partially sorted runs and make them completely sorted
void MergeSort() {
    vector<string> dataFiles = GetSortedRuns();

    // declare all needed variables
    int numberOfRuns = dataFiles.size();
    int fileNumber = 1;
    string fileName = "tkv";
    string fullFileName;
    string tmpFileName;
    static const size_t TKV_LINE_SIZE = 66;
    static const auto TKV_BUFFER_SIZE = TKV_LINE_SIZE * 1024;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fdtkv;
    int fdtmp;
    vector< vector< char > > sortedRunLines;
    vector< vector< char > > completelySortedVector;
    vector<int> nextLine;
    int min;
    
    // prep sizes for vectors
    sortedRunLines.resize(numberOfRuns);
    nextLine.resize(numberOfRuns);
    for(int i = 0; i < sortedRunLines.size(); i++){
        sortedRunLines.at(i).resize(TKV_LINE_SIZE);
    }

    // have vector decide line for each tmp file, start at line 0
    for(int i = 0; i < numberOfRuns; i++) {
        nextLine.at(i) = 0;
    }

    // open all tmp or kv files to get first line of each.
    // add that line to its respective spot on the vector.
    for(int i = 0; i < numberOfRuns; i++) {
        tmpFileName = dataFiles.at(i);
        fdtmp = open(tmpFileName.c_str(), O_RDONLY);
        if (fdtmp == -1) {
            handle_error("open");
        }
            
        // Advise the kernel of our access pattern.
        posix_fadvise(fdtmp, 0, 0, 1);

        char sbuf[TKV_LINE_SIZE + 1];

        size_t bytes_read = read(fdtmp, sbuf, TKV_LINE_SIZE);

        // read first line
        for (int j = 0; j < 66; j++) { 
            sortedRunLines.at(i).at(j) = sbuf[j];
        }
        close(fdtmp);
        nextLine.at(i)++;
    }

    // buf for reading next lines while finding minimum
    char buf[TKV_BUFFER_SIZE + 1];
    // buf for writing merged file
    char sortedBuf[TKV_BUFFER_SIZE + 1];
    int outputCursor = 0;

    // start writing process of 1 tkv file
    for(int i = 0; i < numberOfRuns; i++) {

        // open tkv for this iteration
        fullFileName = fileName + to_string(fileNumber) + ".txt";
        fdtkv = open(fullFileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, mode);
        if (fdtkv == -1) {
            handle_error("open");
        }

        // find minimum and add to vector until it reaches buffer size
        while(completelySortedVector.size() != 1024){

            // find minimum entry
            min = 0;
            for(int j = 1; j < sortedRunLines.size(); j++) {
                for(int k = 0; k < 32; k++) {
                    if(sortedRunLines.at(j).at(k) < sortedRunLines.at(min).at(k)) {
                        min = j;
                        break;
                    }
                    else if (sortedRunLines.at(j).at(k) == sortedRunLines.at(min).at(k)) {
                        continue;
                    }
                    else {
                        break;
                    }
                }
            }

            // write minimum into tmp vector
            completelySortedVector.push_back(sortedRunLines.at(min));

            // set minumum's next line to lower than possible value if finished with respective tmp file, then initiates next loop
            if(nextLine.at(min) == 1024){
                for(int j = 0; j < sortedRunLines.at(min).size(); j++) {
                    sortedRunLines.at(min).at(j) = 'g';
                }
                continue;
            }

            // read new key and value to replace min's place in sortedRunLines
            tmpFileName = dataFiles.at(min);
            fdtmp = open(tmpFileName.c_str(), O_RDONLY);
            if (fdtmp == -1) {
                handle_error("open");
            }
            posix_fadvise(fdtmp, 0, 0, 1);

            size_t bytes_read = read(fdtmp, buf, (TKV_LINE_SIZE * nextLine.at(min)) + TKV_LINE_SIZE);      

            // read first line
            for (int j = 0; j < 66; j++) { 
                sortedRunLines.at(min).at(j) = buf[j + (TKV_LINE_SIZE * nextLine.at(min))];
            }
            close(fdtmp);

            // increment minimum's tmp file nextLine
            nextLine.at(min)++;
        }

        // write completely sorted vector to sortedBuf to prepare for writing
        for(int j = 0; j < completelySortedVector.size(); j++){
            for(int k = 0; k < completelySortedVector.at(j).size(); k++) {
                sortedBuf[outputCursor] = completelySortedVector.at(j).at(k);
                outputCursor++;
            }
        }
        
        // write completelySortedVector to tkv file
        write(fdtkv, sortedBuf, TKV_BUFFER_SIZE);
        fsync(fdtkv);
        close(fdtkv);
        // reset/increment important variables
        outputCursor = 0;
        completelySortedVector.resize(0);
        fileNumber++;
    }

    // Next delete all tmp and kv files in the directory
    string dtmp;
    for(int i = 0; i < dataFiles.size(); i++) {
        dtmp = dataFiles.at(i);
        remove(dtmp.c_str());
    }

    // Then rename tkv files to kv
    string ntmp;
    for(int i = 0; i < numberOfRuns; i++) {
        dtmp = "tkv" + to_string(i + 1) + ".txt";
        ntmp = "kv" + to_string(i + 1) + ".txt";
        rename(dtmp.c_str(), ntmp.c_str());
    }
}