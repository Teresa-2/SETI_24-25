# HTTP 1.1

La debug console continua a dare errore. non riesco a capire come procedere...

# correzione parsing cookie
incapache 2 > Parsing del cookie: 

option_val = strtok_r(NULL, "", &strtokr_save);
					char *uid_pos = strstr(option_val, "UserID=");
					if (uid_pos) sscanf(uid_pos, "UserID=%d", &UIDcookie);