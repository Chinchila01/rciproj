#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "helper.h"

#ifndef COMM_H
#define COMM_H

/* Function: comUDP
*  -------------------
*  Sends a UDP message and waits for an answer
* 
*  Parameters: char* msg      -> message to be sent
*  			   char* dst_ip   -> ip of the destination of the message
*              char* dst_port -> port of the destination of the message
*
*  returns: Reply in char* format
*           NULL if error ocurred
*/
char * comUDP(char * msg, char * dst_ip, char * dst_port);

/* Function: usrRegister
*  -------------------
*  Registers a user
* 
*  Parameters: char* snpip    -> pointer to string with proper name server's ip address in format xxx.xxx.xxx.xxx
*  			   char* port     -> pointer to a string with proper name server's port number
*              char* fullname -> pointer to a string with full name of user in format name.surname
*			   char* my_ip    -> pointer to a string with user ip address in format xxx.xxx.xxx.xxx
*              char* my_port  -> pointer to a string with user port number
*
*  returns: true if registration was successful
*           false if error ocurred
*/
bool usrRegister(char * snpip, char * port, char * full_name, char * my_ip, char * my_port);

/* Function: usrExit
*  -------------------
*  Unregisters the user from the proper name server
* 
*  Parameters: char* snpip    -> pointer to string with proper name server's ip address in format xxx.xxx.xxx.xxx
*  			   char* port     -> pointer to a string with proper name server's port number
*              char* fullname -> pointer to a string with full name of user in format name.surname
*
*  returns: true if unregistration was successful
*           false if error ocurred
*/
bool usrExit(char * snpip, char * port, char * full_name);

/* Function: queryUser
*  -------------------
*  Queries proper name server about the location of a certain user
* 
*  Parameters: char* snpip -> pointer to string with proper name server's ip address in format xxx.xxx.xxx.xxx
*  			   char* port  -> pointer to a string with proper name server's port number
*              char* user  -> pointer to a string with user's name to query about in format name.fullname
*
*  returns: string with user's location if query was successful
*           NULL if user was not found or an error ocurred
*/
char * queryUser(char * snpip, char * port, char * user);

/*
 * Function:  reg_sa
 * --------------------
 * Registers a surname on the surname server using UDP messages.
 *
 *  Parameters: char* saip    -> pointer to a string that contains the surname server's ip address in xxx.xxx.xxx.xxx format
 *  	 		char* saport  -> pointer to a string that contains the surname server's port
 *  		    char* surname -> pointer to a string with the surname to be registered 
 *  			char* localip -> ip of the machine in which the local server is located
 *  			char* snpport -> port that the local server is listening to
 *
 *  returns:  0 if register was successful
 *			 -1 if unregister was unsuccessful	
 */
int reg_sa(char* saip, char* saport, char* surname, char* localip, char* snpport);

/*
 * Function:  unreg_sa
 * --------------------
 * Unregisters a surname from the surname server using UDP messages.
 *
 *  Parameters: char* saip    -> pointer to a string with surname server's ip address in format xxx.xxx.xxx.xxx
 *  			char* saport  -> pointer to a string with surname server's port number
 *  			char* surname -> pointer to a string with the surname to be unregistered 
 *
 *  returns:  0 if unregister was successful
 *	         -1 if unregister was unsuccessful	
 */
int unreg_sa(char* saip, char* saport, char* surname);

/* Function: reg_user
*  -----------------------
*  Registers a user
*
*  Parameters: char* buffer -> pointer to a string with user's information in format REG name;surname;ip;port
*              	 int n      -> integer with number of bytes received
*              char* serverfile -> string with name of file where the server stores names
*			   char* servername -> string with surname associated with the server
*
*  Returns: "OK" if successful
*	       "NOK - " + error specification if error occurs
*/
char* reg_user(char* buffer, int n, char* serverfile, char* servername);

/* Function: unreg_user
*  --------------------
*  Unregisters a user
*
*  Parameters: char* buffer -> pointer to a string with user's information in format UNR name;surname
*  			     int n      -> integer with number of bytes received
*			   char* serverfile -> string with name of file where the server stores names
*			   char* servername -> string with surname associated with the server
*
*  returns: "OK" if unregister is successful
*          "NOK - " + error specification if error occurs
*/
char* unreg_user(char *buffer, int n, char* serverfile, char* servername);

/* Function: qry_asrv
*  -------------------
*  Function to query surname server about the location of an np server with a certain surname
*
*  Parameters: char* saip   -> pointer to a string with the surname server's ip address in format xxx.xxx.xxx.xxx
*  			   char* saport -> pointer to a string with the surname server's port number
*  			   char* surname-> surname to query surname server
*
*  returns: string w/ reply from surname server in the format surname;snpip;snpport
*           empty string if not successful
*/
char* qry_asrv(char* saip, char* saport, char* surname);

/* Function: qry_namesrv
*  -------------------
*  Function to query the proper name server
*
*  Parameters: char* snpip -> pointer to a string with proper name server ip address in format xxx.xxx.xxx.xxx
*  			   char* snpport-> pointer to a string with proper name server port number
*			   char* firstname -> pointer to a string with firstname to query address
*  			   char* surname -> pointer to a string with surname to query address
*
*  returns: string w/ reply from proper name server in the format surname;snpip;snpport
*           empty string if not successfull
*/
char* qry_namesrv(char* snpip, char* snpport, char* firstname, char* surname);

/* Function: find_user
*  -----------------------
*  Uses qry_aserv or qry_namesrv if needed to find the location of a user
*
*  Parameters: char* buffer -> pointer to a string with query in format QRY name.surname
*                int n      -> integer with number of bytes received
*              char* saip   -> pointer to a string with surname server's ip address in format xxx.xxx.xxx.xxx
*			   char* saport -> pointer to a string with surname server's port number
*			   char* serverfile -> string with name of file where the server stores names
*			   char* servername -> string with surname associated with the server
*
*  returns: string with info of user if successful
*           string with NOK if unsuccessful
*/
char* find_user(char* buffer,int n,char* saip, char* saport, char* serverfile, char* servername);

#endif