// $Id: cppstrtok.cpp,v 1.8 2017-09-21 15:51:23-07 - - $

// Use cpp to scan a file and print line numbers.
// Print out each input line read in, then strtok it for
// tokens.

#include <string>
#include <iostream>
using namespace std;

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <getopt.h>

#include "auxlib.h"
#include "string_set.h"

const string CPP = "/usr/bin/cpp -nostdinc ";
constexpr size_t LINESIZE = 1024;

// Chomp the last character from a buffer if it is delim.
void chomp(char* string, char delim)
{
    size_t len = strlen(string);
    if (len == 0) return;
    char* nlpos = string + len - 1;
    if(*nlpos == delim) *nlpos = '\0';
}

// Run cpp against the lines of the file.
void cpplines(FILE* pipe, const char* filename)
{
    int linenr = 1;
    char inputname[LINESIZE];
    strcpy(inputname, filename);
    for (;;) {
        char buffer[LINESIZE];
        char* fgets_rc = fgets(buffer, LINESIZE, pipe);
        if(fgets_rc == nullptr) break;
        chomp(buffer, '\n');
        //printf("%s:line %d: [%s]\n", filename, linenr, buffer);
        // http://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html
        int sscanf_rc = sscanf(buffer,
                               "# %d \"%[^\"]\"",
                               &linenr,
                               inputname);
        if (sscanf_rc == 2)
        {
            /*printf ("DIRECTIVE: line %d file \"%s\"\n",
                    linenr,
                    inputname);*/
            continue;
        }
        char* savepos = nullptr;
        char* bufptr = buffer;
        for (int tokenct = 1;; ++tokenct)
        {
            char* token = strtok_r(bufptr, " \t\n", &savepos);
            bufptr = nullptr;
            if (token == nullptr) break;
            string_set::intern(token);


            /*printf("token %d.%d: [%s]\n",
                   linenr,
                   tokenct,
                   token);*/
        }
        ++linenr;
    }
}

int main(int argc, char** argv)
{
    int opt;
    string preProcArgs = "";
    string filename = argv[argc - 1];

    if(!(filename.substr(filename.find_last_of(".") + 1) == "oc")) {
        cout << "File " +
                filename +
                " is not a .oc file! Exiting..."
                << endl;
        return EXIT_FAILURE;
    }

    while( ( opt = getopt (argc, argv, "lyD:@:") ) != -1 )
    {
        switch(opt)
        {
            case 'l':
                //TODO: later
                break;
            case 'y':
                //TODO: later
                break;
            case '@':
                set_debugflags(optarg);
                break;
            case 'D':
                if(optarg == filename)
                {
                    cerr << "Flag -D requires a string." << endl;
                    return EXIT_FAILURE;
                }

                preProcArgs.append(" -D ");
                preProcArgs.append(optarg);
                break;
            default:
                break;
        }
    }

    preProcArgs.append(" " + filename);
    string preProcCommand = CPP + preProcArgs;
    FILE* pipe = nullptr;
    pipe = popen(preProcCommand.c_str(), "r");

    int exitStatus = EXIT_SUCCESS;
    if(pipe == nullptr)
    {
        exitStatus = EXIT_FAILURE;
        string execname = "no";
        fprintf (stderr,
                 "%s: %s: %s\n",
                 "basename(argv[0])",
                 preProcCommand.c_str(),
                 strerror (errno));
    }
    else
    {
        cpplines(pipe, preProcArgs.c_str());
        string strFilename = filename;
        strFilename.erase(strFilename.end()-3, strFilename.end());
        strFilename.append(".str");
        FILE *nfile= fopen(strFilename.c_str(), "w+");
        string_set::dump(nfile);
        int pcloseVal = pclose(pipe);
        fclose(nfile);
        //eprint_status(preProcCommand.c_str(), pcloseVal);
        if(pcloseVal != 0) exitStatus = EXIT_FAILURE;
    }

    return exitStatus;
}
