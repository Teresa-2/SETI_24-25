# HTTP 1.1

La debug console continua a dare errore. non riesco a capire come procedere...

# correzione parsing cookie
incapache 2 > Parsing del cookie: 

option_val = strtok_r(NULL, "", &strtokr_save);
					char *uid_pos = strstr(option_val, "UserID=");
					if (uid_pos) sscanf(uid_pos, "UserID=%d", &UIDcookie);

# DA CHIEDERE
# gestione cookie (da chiedere)
Quando il client fornisce al server un valore out of range per il cookie, il server assegna al client un nuovo cookie grazie al metodo get_new_uid. Il valore di cookie che il server assegna al client che ha il cookie sbagliato potrebbe essere già stato usato da altre richieste precedenti. In questo caso, il contatore di richieste associato a tale cookie viene sovrascritto e riparte da zero. è corretto? perché ha senso? non capiamo la motivazione. 