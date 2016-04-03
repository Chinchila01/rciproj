#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifndef HELPER_H
#define HELPER_H

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1B[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/* Name of file where default hashtable is located */
#define HASHFILE "hashtable.txt"

/* define boolean variable types */
typedef int bool;
#define true 1
#define false 0


/* Structure where we list valid commands and description, to be used by function help */
struct command_d{
	char* cmd_name;
	char* cmd_help;
};

/* Function: help
*  -------------------
*  Displays a list of all commands available and a description
* 
*  Parameters: struct command_d commands[] -> array of structures with a command name and its description
*  returns: 0
*/
int help(struct command_d commands[],int ncmd);

/*
 * Function:  show_usage
 * --------------------
 * Show the correct usage of the program command
 *
 *  returns:  -1 always (when this runs, it means something about the user's entered arguments went wrong)
 */
int show_usage();

/* Function: encrypt
 * --------------------
 * Encrypt byte according to table on hashTable
 *
 * Parameters: int c -> byte to encrypt
 *
 * returns: encrypted byte if successful
 *          empty char if error ocurred
 */
bool encrypt(unsigned char* encrypted, unsigned char c, char * HASHTABLE);


/* Function: list_users
*  -------------------
*  Lists all users currently registered on the proper name server
* 
*  Parameters: char* serverfile -> string with name of file where the server stores names
*              char* servername -> string with surname associated with the server
*  returns: 0 if listed successfully
*           -1 if error ocurred
*/
int list_users(char* serverfile, char* servername);

/* Function: reset_file
*  ---------------------
*   Resets server file when server starts
*
*   Parameters: char* serverfile -> string with name of file where the server stores names
*   returns: 0 if reset was successful
*            -1 if error ocurred
*/
int reset_file(char* serverfile);

#endif