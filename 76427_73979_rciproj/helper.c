#include "helper.h"

/* Function: help
*  -------------------
*  Displays a list of all commands available and a description
* 
*  returns: 0
*/
int help(struct command_d commands[], int ncmd) {
	int i;

	printf(ANSI_COLOR_BLUE "HELP - Here are the commands available:\n");
	printf(ANSI_COLOR_BLUE "//------------------------------------------------\\\\");
	printf(ANSI_COLOR_WHITE "\n");

	for(i = 0; i < ncmd; i++) {
		printf("\t %s - %s\n",commands[i].cmd_name,commands[i].cmd_help);
	}
	printf(ANSI_COLOR_BLUE "\\\\------------------------------------------------//");
	printf(ANSI_COLOR_WHITE "\n");
	return 0;
}

/*
 * Function:  show_usage
 * --------------------
 * Show the correct usage of the program command
 *
 *  returns:  -1 always (when this runs, it means something about the user's entered arguments went wrong)
 */
int show_usage(int n){
	if(!n) printf("Usage: schat –n name.surname –i ip -p scport -s snpip -q snpport\n");
	else printf("Usage: snp -n surname -s snpip -q snpport [-i saip] [-p saport]\n");
	return -1;
}

/* Function: encrypt
 * --------------------
 * Encrypt byte according to table on hashTable
 *
 * Parameters: int c -> byte to encrypt
 *
 * returns: encrypted byte if successful
 *          empty char if error ocurred
 */
bool encrypt(unsigned char* encrypted, unsigned char c, char * HASHTABLE) {
	FILE *table;
	char buffer[512];
	int i;

	table = fopen(HASHTABLE,"r"); // open key file

	// show error message if cannot open
	if(table == NULL) {
		printf(ANSI_COLOR_RED "encrypt: error: %s",strerror(errno));
		printf(ANSI_COLOR_WHITE "\n");
		encrypted = NULL;

		// set key file to default
		HASHTABLE = malloc(strlen(HASHFILE));
		strcpy(HASHTABLE,HASHFILE);
		return false;
	}

	// read file until c line
	for(i = 0;i < c+1 ; i++){
		fgets(buffer,512,table);
	}

	// get encrypted char, store it on memory and deliver error if file bad formated
	if(sscanf(buffer,"%u",encrypted) != 1) {
		printf(ANSI_COLOR_RED "encrypt: error parsing file");
		printf(ANSI_COLOR_WHITE "\n");
		encrypted = NULL;
		return false;
	}
		
	fclose(table); // close file

	return true;
}

/* Function: list_users
*  -------------------
*  Lists all users currently registered on the proper name server
* 
*  Parameters: char* serverfile -> string with name of file where the server stores names
*              char* servername -> string with surname associated with the server
*  returns: 0 if listed successfully
*           -1 if error ocurred
*/
int list_users(char* serverfile, char* servername) {
	FILE *ufile;
	char buffer[512],uname[512],uip[512],uport[512];

	ufile = fopen(serverfile,"r");
	if(ufile == NULL) {
		printf(ANSI_COLOR_RED "list_users: error: %s", strerror(errno));
		printf(ANSI_COLOR_WHITE "\n");
		return -1;
	}	

	while(fgets(buffer,512,ufile) != NULL){
		if(strstr(buffer,"#") == NULL) {	/* If buffer contains # then line is not valid */
			if(sscanf(buffer,"%[^';'];%[^';'];%s",uname,uip,uport) != 3) {
				printf(ANSI_COLOR_RED "list_users: error reading user file");
				printf(ANSI_COLOR_WHITE "\n");
				fclose(ufile);
				return -1;
			}
			printf("%s.%s - %s - %s\n",uname,servername,uip,uport);
		}
	}
	printf(ANSI_COLOR_GREEN "list_users: Done!");
	printf(ANSI_COLOR_WHITE "\n");
	fclose(ufile);
	return 0;
} 


/* Function: reset_file
*  ---------------------
*   Resets server file when server starts
*
*   Parameters: char* serverfile -> string with name of file where the server stores names
*   returns: 0 if reset was successful
*            -1 if error ocurred
*/
int reset_file(char* serverfile){
	FILE *ufile;
	ufile = fopen(serverfile,"w");
	if(ufile == NULL) {
		printf(ANSI_COLOR_RED "reset_file: error: %s" ANSI_COLOR_RESET,strerror(errno));
		printf(ANSI_COLOR_WHITE "\n");
		return -1;
	}
	
	fclose(ufile);
	return 0;
}