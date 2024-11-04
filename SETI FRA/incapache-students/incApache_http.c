/*
 * incApache_http.c: implementazione del protocollo HTTP per il web server
 *                   del corso di SETI
 *
 * Programma sviluppato a supporto del laboratorio di
 * Sistemi di Elaborazione e Trasmissione del corso di laurea
 * in Informatica classe L-31 presso l'Universita` degli Studi di
 * Genova, per l'anno accademico 2023/2024.
 *
 * Copyright (C) 2012-2014 by Giovanni Chiola <chiolag@acm.org>
 * Copyright (C) 2015-2016 by Giovanni Lagorio <giovanni.lagorio@unige.it>
 * Copyright (C) 2016-2023 by Giovanni Chiola <chiolag@acm.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/***
#define DEBUG
***/

#define COOKIE_EXPIRE "; Expires=Wed, 31 Dec 2024 23:59:59 GMT"

#include "incApache.h"

#define OptimizeTCP

#define RESPONSE_CODE_OK		200
#define RESPONSE_CODE_MOVED_PERMANENTLY	301
#define RESPONSE_CODE_NOT_MODIFIED	304
#define RESPONSE_CODE_BAD_REQUEST	400
#define RESPONSE_CODE_NOT_FOUND		404
#define RESPONSE_CODE_INTERNAL_ERROR	500
#define RESPONSE_CODE_NOT_IMPLEMENTED	501

static int CurUID = 0;
static int UserTracker[MAX_COOKIES];
pthread_mutex_t cookie_mutex = PTHREAD_MUTEX_INITIALIZER;

void remove_newline_and_carriage_return(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
		if (str[i] == '\n' || str[i] == '\r') {
			str[i] = '\0';
			break;
		}
	}
}

int get_new_UID(void)
{
    int retval;

    /*** Compute retval by incrementing CurUID mod MAX_COOKIES
     *** and reset UserTracker[retval] to 0.
     *** Be careful in order to avoid race conditions ***/
/*** TO BE DONE 7.0 START ***/
	pthread_mutex_lock(&cookie_mutex);
    retval = (CurUID + 1) % MAX_COOKIES;
    CurUID = retval; // Update CurUID to the next value
    UserTracker[retval] = 0;
	pthread_mutex_unlock(&cookie_mutex);
/*** TO BE DONE 7.0 END ***/

    return retval;
}


int keep_track_of_UID(int myUID)
{
    int newcount;
    if ( (myUID < 0) || (myUID >= MAX_COOKIES) )
        return -1;

    /*** Increment UserTracker[myUID] and return the computed value.
     *** Be careful in order to avoid race conditions ***/
/*** TO BE DONE 7.0 START ***/
	pthread_mutex_lock(&cookie_mutex);
	newcount = ++UserTracker[myUID];
	pthread_mutex_unlock(&cookie_mutex);
/*** TO BE DONE 7.0 END ***/

    return newcount;
}

#include <stdbool.h>

bool isAlreadySet(int cookie) {//check if the cookie is already set
	if ( (cookie < 0) || (cookie >= MAX_COOKIES) )
		return false;
	if ( UserTracker[cookie] >= 1 )
		return true;
	return false;
}


void send_response(int client_fd, int response_code, int cookie,
#ifdef INCaPACHE_7_1
		   int is_http1_0, int thread_no,
#endif
		   char *filename, struct stat *stat_p)
{
	time_t now_t = time(NULL);
	struct tm now_tm;
	time_t file_modification_time;
	struct tm file_modification_tm;
	char time_as_string[MAX_TIME_STR];
	char http_header[MAX_HEADER_SIZE] = "HTTP/1.";
	size_t header_size;
	int fd = -1;
	off_t file_size = 0;
	char *mime_type = NULL;
	const char * const HTML_mime = "text/html";
	struct stat stat_buffer;
	size_t header_sent;
	debug("  ... start send_response(response_code=%d, filename=%s)\n", response_code, filename);

	/*** Compute date of servicing current HTTP Request using a variant of gmtime() ***/
/*** TO BE DONE 7.0 START ***/
	if(gmtime_r(&now_t, &now_tm) == 0) {
		fail("gmtime_r failed");
	}
/*** TO BE DONE 7.0 END ***/

	strftime(time_as_string, MAX_TIME_STR, "%a, %d %b %Y %T GMT", &now_tm);
#ifdef INCaPACHE_7_1
	strcat(http_header, is_http1_0 ? "0 " : "1 ");
#else /* #ifdef INCaPACHE_7_1 */
	strcat(http_header, "0 ");
#endif /* #ifdef INCaPACHE_7_1 */
	switch (response_code) {
	case RESPONSE_CODE_OK:
		if (filename != NULL) {
			if (stat_p == NULL) {
				stat_p = &stat_buffer;
				if (stat(filename, stat_p)) {
				    debug("stat failed");
                                    response_code = RESPONSE_CODE_INTERNAL_ERROR;
                                    goto int_err;
				}
			} else {
				fd = open(filename, O_RDONLY);
				if (fd<0) {
				    debug("send_response: cannot open file for reading (has the file vanished?)");
                                    response_code = RESPONSE_CODE_INTERNAL_ERROR;
                                    goto int_err;
                                  }
				debug("    ... send_response(%d, %s): opened file %d\n", response_code, filename, fd);
			}
			mime_type = get_mime_type(filename);
			debug("    ... send_response(%d, %s): got mime type %s\n", response_code, filename, mime_type);

			/*** compute file_size and file_modification_time ***/
/*** TO BE DONE 7.0 START ***/
			file_size = stat_p->st_size; // file size
			file_modification_time = stat_p->st_mtime; // file modification time
/*** TO BE DONE 7.0 END ***/

			debug("      ... send_response(%3d,%s) : file opened, size=%lu, mime=%s\n", response_code, filename, (unsigned long)file_size, mime_type);
		        strcat(http_header, "200 OK");
		} else
		    strcat(http_header, "200 OK"); 
		break;
	case RESPONSE_CODE_MOVED_PERMANENTLY:
		strcat(http_header, "301 Moved Permanently");
		strcat(http_header, "Location: ");
		strcat(http_header, filename);
		break;
	case RESPONSE_CODE_NOT_MODIFIED:
		strcat(http_header, "304 Not Modified");
		break;
	case RESPONSE_CODE_BAD_REQUEST:
		strcat(http_header, "400 Bad Request");
		break;
	case RESPONSE_CODE_NOT_FOUND:
		strcat(http_header, "404 Not Found");
		if ((fd = open(HTML_404, O_RDONLY)) >= 0) {

			/*** compute file_size, mime_type, and file_modification_time of HTML_404 ***/
/*** TO BE DONE 7.0 START ***/
			stat_p = &stat_buffer;
			if (stat(HTML_404, stat_p)) {//Get file attributes about HTML_404 and put them in stat_p
			    debug("stat failed (HTML_404)");
                                response_code = RESPONSE_CODE_INTERNAL_ERROR;
                                goto int_err;
			}
			mime_type = strdup(HTML_mime);//Get the mime type of HTML_404
			debug("    ... send_response(%d, %s): got mime type %s\n", response_code, HTML_404, mime_type);
			file_modification_time = stat_p -> st_mtime;
			file_size = stat_p -> st_size;
			debug("      ... send_response(%3d,%s) : file opened, size=%lu, mime=%s\n", response_code, HTML_404, (unsigned long)file_size, mime_type);
/*** TO BE DONE 7.0 END ***/

		}
		break;
	case RESPONSE_CODE_INTERNAL_ERROR:
            int_err:
		strcat(http_header, "500 Internal Error");
		break;
	default:

/*** TO BE OPTIONALLY CHANGED START ***/
		strcat(http_header, "501 Method Not Implemented\r\nAllow: HEAD,GET");
/*** TO BE OPTIONALLY CHANGED END ***/

		if ((fd = open(HTML_501, O_RDONLY)) >= 0) {

			/*** compute file_size, mime_type, and file_modification_time of HTML_501 ***/
/*** TO BE DONE 7.0 START ***/
			stat_p = &stat_buffer;
			if (stat(HTML_501, stat_p)) {
			    debug("stat failed (HTML_501)");
                                response_code = RESPONSE_CODE_INTERNAL_ERROR;
                                goto int_err;
			}
			mime_type = strdup(HTML_mime);
			debug("    ... send_response(%d, %s): got mime type %s\n", response_code, HTML_501, mime_type);
			file_modification_time = stat_p -> st_mtime;
			file_size = stat_p -> st_size;
			debug("      ... send_response(%d,%s) : file opened, size=%lu, mime=%s\n", response_code, HTML_501, (unsigned long)file_size, mime_type);
/*** TO BE DONE 7.0 END ***/

		}
		break;
	}
	strcat(http_header, "\r\nDate: ");
	strcat(http_header, time_as_string);
        if ( cookie >= 0 ) {
            /*** set permanent cookie in order to identify this client ***/
/*** TO BE DONE 7.0 START ***/
			sprintf(http_header + strlen(http_header), "\r\nSet-Cookie: UserID=%i", cookie);
			//if(!isAlreadySet(cookie))		
			strcat(http_header, COOKIE_EXPIRE);
/*** TO BE DONE 7.0 END ***/

        }
#ifdef INCaPACHE_7_1
	strcat(http_header, "\r\nServer: incApache 7.1 for SETI.\r\n");
	if (response_code >= 500 || is_http1_0)
		strcat(http_header, "Connection: close\r\n");
#else
	strcat(http_header, "\r\nServer: incApache 7.0 for SETI.\r\n");
	strcat(http_header, "Connection: close\r\n");
#endif
	if (file_size > 0 && mime_type != NULL) {
		sprintf(http_header + strlen(http_header), "Content-Length: %lu \r\nContent-Type: %s\r\nLast-Modified: ", (unsigned long)file_size, mime_type);

		/*** compute time_as_string, corresponding to file_modification_time, in GMT standard format;
		     see gmtime and strftime ***/
/*** TO BE DONE 7.0 START ***/
		if (gmtime_r(&file_modification_time, &file_modification_tm) == 0)
			fail("Could not convert file modification time to broken-down time in send response");
		if(strftime(time_as_string, MAX_TIME_STR, "%a, %d %b %Y %T GMT", &file_modification_tm) == 0)
			fail("Could not convert file modification time to string in send response");
/*** TO BE DONE 7.0 END ***/

		strcat(http_header, time_as_string);
		strcat(http_header, "\r\n");
	}
	strcat(http_header, "\r\n");
	debug("      ... send_response(%d, %s) : header prepared\n", response_code, filename);
	printf("Sending the following response:\n%s\n",http_header);
	header_size = strlen(http_header);
#ifdef INCaPACHE_7_1
	join_prev_thread(thread_no);
#endif
#ifdef OptimizeTCP
	if ((header_sent=send_all(client_fd, http_header, header_size, (fd >= 0 ? MSG_MORE : 0))) < header_size)
#else
	if ((header_sent=send_all(client_fd, http_header, header_size, 0)) < header_size )
#endif
	{
		if (header_sent==-1)
			fail_errno("incApache: could not send HTTP header");
		debug("header partially sent; header_size=%lu, header_sent=%lu\n", (unsigned long) header_size, (unsigned long) header_sent);
		close(fd);
		fd = -1;
	}
	debug("      ... send_response(%d, %s): header sent\n", response_code, filename);
	if (fd >= 0) {

		/*** send fd file on client_fd, then close fd; see syscall sendfile  ***/
/*** TO BE DONE 7.0 START ***/
		if ( sendfile(client_fd, fd, NULL, file_size) < 0) {
			fail_errno("sendfile failed");
		}
		if((fd) < 0) {
			fail_errno("close failed");
		}
/*** TO BE DONE 7.0 END ***/

	}
#ifdef OptimizeTCP
	if (fd >= 0) {
		int optval = 1;
		if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int)))
			fail_errno("Cannot set socket options");
	}
#endif
	debug("  ... end send_response(%d, %s)\n", response_code, filename);
	free(mime_type);
}


void manage_http_requests(int client_fd
#ifdef INCaPACHE_7_1
		, int connection_no
#endif
)
{
#define METHOD_NONE		 0
#define METHOD_HEAD		 1
#define METHOD_GET		 2
#define METHOD_POST		 4
#define METHOD_NOT_CHANGED	 8
#define METHOD_CONDITIONAL	16
#define MethodIsConditional(m) ((m)&METHOD_CONDITIONAL)
	FILE *client_stream = fdopen(client_fd, "r");
	char *http_request_line = NULL;
	char *strtokr_save;
	size_t n = 0;
	int http_method;
	struct tm since_tm;
	struct stat *stat_p;
        int UIDcookie = -1;
#ifdef INCaPACHE_7_1
	int is_http1_0 = 0;
	int thread_idx;
#endif
	if (!client_stream)
		fail_errno("cannot open client stream");
	while (getline(&http_request_line, &n, client_stream) >= 0) {
		char *method_str, *filename, *protocol;
		char *http_option_line = NULL;
		char *option_name, *option_val;
		printf("\n-----------------------------------------------\n");
		printf("Received the following request:\n");
		printf(" %s", http_request_line);
#ifdef INCaPACHE_7_1
		thread_idx = find_unused_thread_idx(connection_no);
#endif

		/*** parse first line defining the 3 strings method_str,
		 *** filename, and protocol ***/
/*** TO BE DONE 7.0 START ***/
		debug("\n");
		method_str = strtok_r(http_request_line, " ", &strtokr_save);

		filename = strtok_r(NULL, " ", &strtokr_save);
		protocol = strtok_r(NULL, " \r\n", &strtokr_save);

/*** TO BE DONE 7.0 END ***/
		debug("   ... method_str=%s, filename=%s (0=%c), protocol=%s (len=%d)\n",
		      method_str, filename, filename ? filename[0] : '?', protocol, (int)(protocol ? strlen(protocol) : 0));
		if (method_str == NULL || filename == NULL || protocol == NULL ||
		    filename[0] != '/' || strncmp(protocol, "HTTP/1.", 7) != 0 ||
		    strlen(protocol) != 8) {
			debug("       ... Bad Request!\n");
			SEND_RESPONSE(client_fd, RESPONSE_CODE_BAD_REQUEST, UIDcookie,
#ifdef INCaPACHE_7_1
				      1, connection_no, thread_idx,
#endif
				      NULL, NULL);
			free(http_request_line);
			break;
		}
#ifdef INCaPACHE_7_1
		is_http1_0 = !strcmp(protocol, "HTTP/1.0");
#endif
		memset(&since_tm, 0, sizeof(since_tm));
		http_method = METHOD_NONE;
		if (strcmp(method_str, "GET") == 0)
			http_method = METHOD_GET;
		else if (strcmp(method_str, "HEAD") == 0)
			http_method = METHOD_HEAD;
		else if (strcmp(method_str, "POST") == 0)
			http_method = METHOD_POST;
		debug("     ... http_method=%d\n", http_method);
		for (http_option_line = NULL, n = 0;
		     getline(&http_option_line, &n, client_stream) >= 0 && strcmp(http_option_line, "\r\n") != 0;
		     free(http_option_line), http_option_line = NULL, n = 0) {
			option_name = strtok_r(http_option_line, ": ", &strtokr_save);
			
			if ( option_name != NULL ) {
			    if ( strcmp(option_name, "Cookie") == 0 ) {
                                /*** parse the cookie in order to get the UserID and count the 
								 * number of requests coming from this client ***/
/*** TO BE DONE 7.0 START ***/
					myDebug("    ... strtoksave= ");
					for (int i = 0; i < strlen(strtokr_save); ++i) {
						myDebug("%c", strtokr_save[i]);
					}
					myDebug("\n");
					
					while((option_val = strtok_r(NULL, "=", &strtokr_save)) != NULL) {
						myDebug("    ... option_val= ");
						for (int i = 0; i < strlen(option_val); ++i) {
							myDebug("%c", option_val[i]);
						}
						myDebug("\n");
						if (strncmp(option_val, " UserID", 7) == 0 || strncmp(option_val, "UserID", 6) == 0) {
							
							option_val = strtok_r(NULL, ";", &strtokr_save);
							myDebug("    ... option_val= ");
							for (int i = 0; i < strlen(option_val); ++i) {
								myDebug("%c", option_val[i]);
							}
							myDebug("\n");
							for (int i = 0; i < strlen(option_val) - 1; ++i) {
								if (option_val[i] < '0' || option_val[i] > '9') {
									UIDcookie = -1;
									break;
								}
							}
							UIDcookie = atoi(option_val);
							printf("    ... UIDcookie=%d\n", UIDcookie);
							if(!isAlreadySet(UIDcookie)){
								const time_t ten_hours = 60*60*10;
								time_t now_t = time(NULL);
								char time_as_string[MAX_TIME_STR];
								struct tm cookie_Expire_Date_tm;
								time_t expire_date_time_t = now_t + ten_hours;
								gmtime_r(&expire_date_time_t, &cookie_Expire_Date_tm);
								strftime(time_as_string, MAX_TIME_STR, "%a, %d %b %Y %T GMT", &cookie_Expire_Date_tm);
								printf("Expires: ");
								for (int i = 0; i < strlen(time_as_string); ++i) {
									printf("%c", time_as_string[i]);
								}
								printf("\n");
							}
							
							break;
						}else { option_val = strtok_r(NULL, " ", &strtokr_save);

						}
					}
					/*option_val = strtok_r(NULL, " ", &strtokr_save);
					if(option_val != NULL && strncmp(option_val, "Expires", 7) != 0) {
						debug("	   ... Optionval= ");
						for(int i = 0; i < strlen(option_val); ++i) {
							debug("%c", option_val[i]);
						}
						debug("\n");
					}		
				//cheching if client provided a expiration date for the cookie, otherwise we let it do by the system
					if (option_val != NULL && strncmp(option_val, "Expires", 7 ) == 0) {
						struct tm cookie_Expire_Date_tm;
						myDebug("       ... option_name= ");
						for (int i = 0; i < strlen(option_val); ++i) {
							printf("%c", option_val[i]);
						}
						myDebug("\n 	 ... option_val= ");
						debug(" ");
    					option_val = strtok_r(NULL, "\r\n", &strtokr_save);
						for (int i = 0; i < strlen(option_val); ++i) {
							printf("%c", option_val[i]);
						}
						printf("\n");
    					if (option_val != NULL ) {
							strptime(option_val, "%a, %d %b %Y %T GMT", &cookie_Expire_Date_tm);
							myDebug("Seconds: %d\n", cookie_Expire_Date_tm.tm_sec);
    						myDebug("Minutes: %d\n", cookie_Expire_Date_tm.tm_min);
							myDebug("Hours: %d\n", cookie_Expire_Date_tm.tm_hour);
							myDebug("Day of the month: %d\n", cookie_Expire_Date_tm.tm_mday);
							myDebug("Month: %d\n", cookie_Expire_Date_tm.tm_mon);
							myDebug("Year: %d\n", cookie_Expire_Date_tm.tm_year);
							myDebug("Day of the week: %d\n", cookie_Expire_Date_tm.tm_wday);
							myDebug("Day of the year: %d\n", cookie_Expire_Date_tm.tm_yday);
							myDebug("Daylight saving time flag: %d\n", cookie_Expire_Date_tm.tm_isdst);
							if ( cookie_Expire_Date_tm.tm_isdst != 1 && cookie_Expire_Date_tm.tm_isdst != -1 && cookie_Expire_Date_tm.tm_isdst != 0) {
								debug("Daylight saving time flag= %d\n", cookie_Expire_Date_tm.tm_isdst);
								cookie_Expire_Date_tm.tm_isdst = -1;
								debug("Strptime failed, adjusted daylight field\n");
							}
							time_t cookie_Expire_Date_time_t = my_timegm(&cookie_Expire_Date_tm);
#ifdef FORSE				if(cookie_Expire_Date_time_t == -1){
								debug("       ... Bad Request!\n");
							SEND_RESPONSE(client_fd, RESPONSE_CODE_BAD_REQUEST, UIDcookie,
#ifdef INCaPACHE_7_1
				     		1, connection_no, thread_idx,
#endif
				      		NULL, NULL);
							free(http_request_line);
							break;
#endif						}
								//fail("Error in converting cookie expire date to time_t\n");
							time_t now_t = time(NULL);
							debug("...checking if expired: cookie_Expire_Date_tm = %ld, ", cookie_Expire_Date_time_t);
							debug("now_t = %ld\n", now_t);
					  		if(now_t > cookie_Expire_Date_time_t){
								UIDcookie = -1;
					    		printf("\nCookie has expired \n");
					   		} //if not expired we keep che UIDcookie got previously by the request 
						}
						else {
							debug("       ... Bad Request!\n");
							SEND_RESPONSE(client_fd, RESPONSE_CODE_BAD_REQUEST, UIDcookie,
#ifdef INCaPACHE_7_1
				     		1, connection_no, thread_idx,
#endif
				      		NULL, NULL);
							free(http_request_line);
							break;
						}
    				} else { if(!isAlreadySet(UIDcookie)){//setto io la data di scadenza del coockie
							debug("The string does not continue with 'Expires and is the first request'\n");
							const time_t ten_hours = 60*60*10;
							time_t now_t = time(NULL);
							char time_as_string[MAX_TIME_STR];
							struct tm cookie_Expire_Date_tm;
							time_t expire_date_time_t = now_t + ten_hours;
							gmtime_r(&expire_date_time_t, &cookie_Expire_Date_tm);
							strftime(time_as_string, MAX_TIME_STR, "%a, %d %b %Y %T GMT", &cookie_Expire_Date_tm);
							printf("Expires: ");
							for (int i = 0; i < strlen(time_as_string); ++i) {
								printf("%c", time_as_string[i]);
							}
							printf("\n");
						}*/
    				
					continue;
/*** TO BE DONE 7.0 END ***/

            	}
			    if ( http_method == METHOD_GET ) {

				/*** parse option line, recognize "If-Modified-Since" option,
				 *** and possibly add METHOD_CONDITIONAL flag to http_method
                                 *** and store date in since_tm
                                 ***/
/*** TO BE DONE 7.0 START ***/
				if ( strncmp(option_name, "If-Modified-Since", 17) == 0 ) {
					//option_val = strtok_r(NULL, " ", &strtokr_save);
					myDebug("	  ... option_name= ");
					debug("	  ...METHOD_CONDITIONAL: ");
					for (int i = 0; i < strlen(option_name); ++i) {
						debug("%c", option_name[i]);
					}
					myDebug("\n");
				    option_val = strtok_r(NULL, "\r\n", &strtokr_save);
					myDebug("    	... option_val= ");
					for (int i = 0; i < strlen(option_val); ++i) {
						debug("%c", option_val[i]);
					}
					debug("\n");
				    if ( option_val != NULL ) {
						strptime(option_val, " %a, %d %b %Y %T GMT", &since_tm);
						http_method = METHOD_CONDITIONAL; /* add METHOD_CONDITIONAL flag to http_method */
					}
				}
/*** TO BE DONE 7.0 END ***/

			    }
                        }
		}
		free(http_option_line);
                if ( UIDcookie >= 0 ) { /*** increment visit count for this user ***/
                    int current_visit_count = keep_track_of_UID(UIDcookie);

                    if ( current_visit_count < 0 ) /*** wrong Cookie value ***/

                        UIDcookie = get_new_UID();
                    else {
			printf("\n client provided UID Cookie %d for the %d time\n", UIDcookie, current_visit_count);
                        UIDcookie = -1;
                    }
                } else /*** user did not provide any Cookie ***/
                    UIDcookie = get_new_UID();

/*** TO BE OPTIONALLY CHANGED START ***/ /* this is just an example of how to handle POST requests */
		if (http_method == METHOD_POST){
			http_method = METHOD_NONE;
		}
		if (http_method == METHOD_NONE ) {
/*** TO BE OPTIONALLY CHANGED END ***/

			printf("method not implemented\n");
			SEND_RESPONSE(client_fd, 501, UIDcookie,
#ifdef INCaPACHE_7_1
				      1, connection_no, thread_idx,
#endif
				      method_str, NULL);
			break;
		}
		printf("\n-----------------------------------------------\n");
		if (strcmp(filename, "/") == 0)
			filename = "index.html";
		else
			filename += 1; /* remove leading '/' */
		debug("http_method=%d, filename=%s\n", http_method, filename);
		stat_p = my_malloc(sizeof(*stat_p));
		if (access(filename, R_OK) != 0 || stat(filename, stat_p) < 0) {
			debug("    ... file %s not found!\n", filename);
			free(stat_p);
			SEND_RESPONSE(client_fd, RESPONSE_CODE_NOT_FOUND, UIDcookie,
#ifdef INCaPACHE_7_1
				      is_http1_0, connection_no, thread_idx,
#endif
				      filename, NULL);
		} else {
			if (MethodIsConditional(http_method)) {

				/*** compare file last modification time and decide
				 *** whether to transform http_method to METHOD_NOT_CHANGED
				 *** Use something like timegm() to convert from struct tm to time_t
				 ***/
/*** TO BE DONE 7.0 START ***/
				time_t file_modification_time = stat_p -> st_mtime;
				time_t since_time = my_timegm(&since_tm);
				if (since_time < 0) {
					debug("       ... Bad Request!\n");
			SEND_RESPONSE(client_fd, RESPONSE_CODE_BAD_REQUEST, UIDcookie,
#ifdef INCaPACHE_7_1
				      1, connection_no, thread_idx,
#endif
				      NULL, NULL);
			free(http_request_line);
			break;
				}
				myDebug("    ... stat_p -> st_mtime data: %ld\n", file_modification_time);
				myDebug("    ... since_tm data: %ld\n", since_time);
				myDebug("    ... difftime data: %f\n", difftime(since_time, file_modification_time));
				debug(" ... checking conditional get : mtime = %ld, since = %ld\n", file_modification_time, since_time);
				if((since_time - file_modification_time) >= 0)
					http_method = METHOD_NOT_CHANGED;
				else 
					http_method = METHOD_GET;																								

/*** TO BE DONE 7.0 END ***/

			}
			switch (http_method) {
			case METHOD_HEAD :
				debug("    ... sending header for file %s\n", filename);
				free(stat_p);
				SEND_RESPONSE(client_fd, RESPONSE_CODE_OK, UIDcookie, /*** OK, without body ***/
#ifdef INCaPACHE_7_1
					      is_http1_0, connection_no, thread_idx,
#endif
					      filename, NULL);
				break;
			case METHOD_NOT_CHANGED :
				debug("    ... file %s not modified\n", filename);
				free(stat_p);
				SEND_RESPONSE(client_fd, RESPONSE_CODE_NOT_MODIFIED, UIDcookie, /*** Not Modified, without body ***/
#ifdef INCaPACHE_7_1
					      is_http1_0, connection_no, thread_idx,
#endif
					      NULL, NULL);
				break;
			case METHOD_GET :
				debug("    ... sending file %s\n", filename);
				SEND_RESPONSE(client_fd, RESPONSE_CODE_OK, UIDcookie, /*** OK, with body ***/
#ifdef INCaPACHE_7_1
					      is_http1_0, connection_no, thread_idx,
#endif
					      filename, stat_p);
				break;
			case METHOD_POST :

/*** TO BE OPTIONALLY DONE START ***/
				/*debug("    ... sending file %s\n", filename);
				SEND_RESPONSE(client_fd, RESPONSE_CODE_OK, UIDcookie,  OK, with body 
#ifdef INCaPACHE_7_1
					      is_http1_0, connection_no, thread_idx,
#endif
						filename, stat_p);*/
				break;
/*** TO BE OPTIONALLY DONE END ***/

			default:
				assert(0);
			}
		}
#ifdef INCaPACHE_7_1
		if (is_http1_0)
#endif
			break;
	}
#ifdef INCaPACHE_7_1
	join_all_threads(connection_no);
#endif
	if (close(client_fd))
		fail_errno("Cannot close the connection");
}