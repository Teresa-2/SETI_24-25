//1 In quale sezione del manuale troviamo i formati di file?
/*La sezione del manuale che contiene informazioni sui formati dei file è la sezione 5. 
Quindi, se stai cercando informazioni su un particolare formato di file, 
dovresti cercare nella sezione 5 del manuale.
Per accedere a una pagina del manuale di una particolare sezione, puoi usare il comando man 
seguito dal numero della sezione e il nome della pagina. 
Ad esempio, per visualizzare la pagina del manuale per il formato di file passwd 
(che si trova nella sezione 5), potresti usare il comando man 5 passwd.*/

//2 A che cosa serve -p in mkdir(1) ?
/*Quando usi mkdir con l'opzione -p o --parents, il comando creerà tutte le directory genitore 
necessarie per il percorso specificato, se non esistono già. Inoltre, se la directory finale esiste già, 
non verrà segnalato alcun errore.
Ad esempio, se esegui mkdir -p /a/b/c e nessuna di queste directory esiste, 
mkdir creerà prima la directory /a, poi la directory /b dentro /a, e infine la 
directory /c dentro /b. Se una o più di queste directory esistono già, 
mkdir non segnalerà alcun errore e creerà solo le directory mancanti.*/

//3. Che cosa fa il comando apropos -s 1 directory | grep current ?
//Per i pigri: http://explainshell.com/explain?cmd=apropos+-s+1+directory+%7C+grep+current
//Nota: su alcuni sistemi (Mac OS X?) potrebbe non essere presente l opzione -s
/* apropos - search the manual page names and descriptions
-s  Search only the given manual sections
grep searches the named input FILEs (or standard input if no files are named, or if a single hyphen-minus
(-) is given as file name) for lines containing a match to the given PATTERN.  By  default,  grep  prints
the matching lines. */
 
//4. Come faccio a “stampare” il nome della directory di lavoro?
/*Per stampare il nome della directory di lavoro, puoi usare il comando pwd.*/

//5. Come posso vedere se un comando, per esempio cd, `e interno (built-in) o esterno? (Suggerimento: leggete
//bene il testo, il comando per farlo `e gi`a stato nominato ,)
/*Per vedere se un comando è interno o esterno, puoi usare il comando type.*/

//6. Come faccio, con il comando ls, a elencare anche i file che iniziano con . (il carattere punto)?
/*Per elencare anche i file che iniziano con ., puoi usare l'opzione -a o --all.
oppure ll*/

//7. Cosa cambia se uso -h con ls?
/*L'opzione -h o --human-readable cambia la visualizzazione delle dimensioni dei file in modo che
siano più leggibili per gli umani. 
ls -lh
ls -lah
*/

//8. E possibile creare una directory il cui nome contenga degli spazi? `
/*Sì, è possibile creare una directory il cui nome contenga spazi. 
Per fare ciò, è necessario utilizzare le virgolette intorno al nome della directory.
mkdir "my directory"
*/

//9. Dove porta il comando cd senza argomenti? Come potete fare in modo che vi porti 
//da un’altra parte?
/*Senza argomenti, il comando cd porta alla directory home dell'utente corrente.
Per andare in un'altra directory, puoi usare il comando cd seguito dal percorso della directory
oppure cd ../ per ritornare alla cartella successiva.*/

//10. Che differenza c’`e fra cd e pushd?
/*Il comando cd cambia la directory corrente, mentre il comando pushd cambia la 
directory corrente e salva la directory precedente in una pila di directory. 
Questo ti permette di tornare facilmente a directory precedenti utilizzando il 
comando popd. Ad esempio, se sei nella directory /home/user e digiti pushd Documents,
il tuo shell cambierà la directory corrente in /home/user/Documents e salverà 
/home/user nella pila. Poi, se digiti popd, il tuo shell tornerà alla directory 
/home/user..*/

//11. A cosa serve which? Qual `e il suo exit-status? (l’exit-status dell’ultimo 
//comando viene memorizzato nella variabile $?, quindi potete vederlo con echo $?)
/*Il comando which cerca il percorso di un comando specificato.
Se il comando viene trovato, which restituirà il percorso del comando e 
uscirà con un exit status di 0.
Se il comando non viene trovato, which non restituirà alcun output e
uscirà con un exit status di 1
il quale viene salvano nella variabile $?, quindi per verificare l'exitstatus, 
basta fare echo $?.*/

//12. Create un alias (usando alias) per elencare i file con estensione .c; 
//ricordatevi che la shell separa il suo
//input dove trova degli spazi ed espande le wildcard. . .
/*Per creare un alias per elencare i file con estensione .c, puoi usare il comando 
alias seguito dal nome dell'alias, un uguale e il comando che vuoi eseguire.
alias nomealias='ls *.c'
nb, se killi il terminale l'alias creato in questo modo muore con lei, 
credo che per mantenerlo vivo vada scritto il comando nel file .bashrc, 
cosi' all'avvio del terminale vienne creato l'alias*/

//13. Stampate le prime 3 linee del file /etc/group, usando head(1). 
//Poi, stampatene le ultime 3, usando tail(1)
/*Per stampare le prime 3 linee di un file, puoi usare il comando head -n 3 seguito 
dal nome del file o del percorso del file.
Per stampare le ultime 3 linee di un file, puoi usare il comando tail -n 3 seguito
dal nome del file o del percorso del file.*/

///14. Scrivete un comando per contare quanti utenti hanno /bin/false come shell.
//    • per sapere dove guardare, passwd(5)
//    • per contare le linee che contengono un certo pattern, grep(1)
//    • per estrarre, da ogni linea, solo il campo “optional user command interpreter” (
//        cio`e la shell dell’utente), cut(1)
/*/etc/passwd contains one line for each user account, with seven fields delimited by
       colons (“:”).
         These fields are:
            name:password:UID:GID:GECOS:directory:shell
            1    2        3   4   5     6         7

Per contare quanti utenti hanno /bin/false come shell, puoi utilizzare una combinazione 
dei comandi grep, cut e wc. Il file da esaminare è /etc/passwd, che contiene le 
informazioni degli utenti.
Il comando cut può essere utilizzato per estrarre il campo della shell dall'output. 
Il file /etc/passwd utilizza : come delimitatore di campo, e la shell è l'ultimo campo.
Il comando grep può essere utilizzato per filtrare le linee che contengono /bin/false.
Infine, il comando wc -l può essere utilizzato per contare il numero di linee, 
che corrisponde al numero di utenti con /bin/false come shell.

Ecco il comando completo: cut -d: -f7 /etc/passwd | grep -c '/bin/false'

In questo comando:
cut -d: -f7 /etc/passwd estrae il settimo campo (la shell) da ogni linea in /etc/passwd.
grep -c '/bin/false' conta il numero di linee che contengono /bin/false. */


//15. Elencate, senza duplicazioni, le shell specificate in passwd; 
//vi servir`a uniq(1) ma non solo: leggete bene
//cosa fa uniq e che altro vi suggerisce di guardare la sua man-page
/*Filter adjacent matching lines from INPUT (or standard input), writing to OUTPUT 
(or standard output).
With no options, matching lines are merged to the first occurrence.
A volte, quando si utilizza il comando uniq, si desidera solo rimuovere le linee 
duplicate consecutive. Per fare ciò, è possibile utilizzare l'opzione -d o --repeated.
 il manuale suggerisce di guardare anche 
comm(1):
Compare sorted files FILE1 and FILE2 line by line.
When FILE1 or FILE2 (not both) is -, read standard input.
With  no  options, produce three-column output.  Column one contains lines unique to
FILE1, column two contains lines unique to FILE2, and column  three  contains  lines
common to both files.
join(1):
For  each  pair  of input lines with identical join fields, write a line to standard
output.  The default join field is the first, delimited by blanks.
When FILE1 or FILE2 (not both) is -, read standard input.
sort(1):
Write sorted concatenation of all FILE(s) to standard output.
With no FILE, or when FILE is -, read standard input.

il comando cut può essere utilizzato per estrarre il campo della shell dall'output. Il file /etc/passwd utilizza : come delimitatore di campo, e la shell è l'ultimo campo.
Il comando sort può essere utilizzato per ordinare le linee, che è necessario prima di utilizzare uniq.
Infine, il comando uniq può essere utilizzato per rimuovere le linee duplicate.
cut -d: -f7 /etc/passwd | sort | uniq
In questo comando:

cut -d: -f7 /etc/passwd estrae il settimo campo (la shell) da ogni linea in /etc/passwd.
sort ordina le linee.
uniq rimuove le linee duplicate. */


//16. Elencate tutti gli utenti del sistema, in ordine alfabetico inverso
/*Per elencare tutti gli utenti del sistema in ordine alfabetico inverso, 
puoi utilizzare il comando cut per estrarre il campo dell'utente da /etc/passwd,
il comando sort per ordinare le linee e l'opzione -r per invertire l'ordine,
e il comando uniq per rimuovere le linee duplicate.
cut -d: -f1 /etc/passwd | sort -r | uniq
In questo comando: 

cut -d: -f1 /etc/passwd estrae il primo campo (l'utente) da ogni linea in /etc/passwd.
sort -r ordina le linee in ordine alfabetico inverso.
uniq rimuove le linee duplicate.*/

//17. Elencate i PID (e nient’altro) di tutti i processi del sistema (sono tanti, se ve ne escono meno di cento c’`e
//probabilmente qualcosa che non quadra); vedete ps(1)
//    • Assicuratevi che ps scriva solo i PID, senza nessuna intestazione
//    • Scrivete un comando per contare quanti sono; vedete wc(1)

/*Per elencare i PID di tutti i processi del sistema, puoi utilizzare il comando ps
con l'opzione -e o --everyone per elencare tutti i processi, e l'opzione -o o --format
per specificare il formato di output. Per elencare solo i PID, puoi utilizzare il formato
pid. Per contare i PID, puoi utilizzare il comando wc -l.
ps -eo pid | wc -l */

//18. Dire cosa fanno i seguenti comandi:
//    • top; se installato (non lo `e di default su molte distribuzioni), 
//              potete anche provare htop
//    • pstree
//    • xdg-open .
//    • find ˜ -type d -empty -ls

/*top - display Linux processes
top - 11:10:32 up  1:30,  1 user,  load average: 0.02, 0.07, 0.03
Tasks:  55 total,   1 running,  54 sleeping,   0 stopped,   0 zombie
%Cpu(s):  0.1 us,  0.1 sy,  0.0 ni, 99.7 id,  0.0 wa,  0.0 hi,  0.1 si,  0.0 st
MiB Mem :   7903.1 total,   4820.9 free,   1151.2 used,   1930.9 buff/cache
MiB Swap:   2048.0 total,   2048.0 free,      0.0 used.   6494.7 avail Mem

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
   2838 frances+  20   0   11.8g 466136  52236 S   1.0   5.8   2:20.24 node
      1 root      20   0  167016  12456   8192 S   0.0   0.2   0:00.82 systemd
      2 root      20   0    2324   1264   1152 S   0.0   0.0   0:00.00 init-systemd(Ub
      5 root      20   0    2368     68     68 S   0.0   0.0   0:00.00 init
     40 root      19  -1   47724  15788  14760 S   0.0   0.2   0:00.08 systemd-journal
     60 root      20   0   21884   5800   4440 S   0.0   0.1   0:00.15 systemd-udevd
     76 root      20   0    4492    196     48 S   0.0   0.0   0:00.00 snapfuse
     78 root      20   0    4492    172     28 S   0.0   0.0   0:00.00 snapfuse
     79 root      20   0    4624    172     24 S   0.0   0.0   0:00.00 snapfuse
     80 root      20   0    4684   1892   1416 S   0.0   0.0   0:00.69 snapfuse
     83 root      20   0    4492    192     44 S   0.0   0.0   0:00.00 snapfuse
     87 root      20   0    4728   1916   1416 S   0.0   0.0   0:02.00 snapfuse
     91 root      20   0    4948   1660   1180 S   0.0   0.0   0:00.84 snapfuse
    108 systemd+  20   0   25532  12572   8372 S   0.0   0.2   0:00.10 systemd-resolve
    139 root      20   0    4304   2680   2436 S   0.0   0.0   0:00.00 cron
    141 message+  20   0    8588   4504   3964 S   0.0   0.1   0:00.10 dbus-daemon
    147 root      20   0   30096  19496  10292 S   0.0   0.2   0:00.07 networkd-dispat
    148 syslog    20   0  222400   6980   4172 S   0.0   0.1   0:00.01 rsyslogd
    149 root      20   0 2058988  47004  19400 S   0.0   0.6   0:01.96 snapd
    150 root      20   0   15324   7328   6384 S   0.0   0.1   0:00.10 systemd-logind
    213 root      20   0  107080  21044  12928 S   0.0   0.3   0:00.04 unattended-upgr
    216 root      20   0    3236   1068    980 S   0.0   0.0   0:00.00 agetty
    218 root      20   0    3192   1112   1024 S   0.0   0.0   0:00.00 agetty

    htop la stessa cosa ma con una grafica piu' carina

pstree visualizza un albero dei processi
systemd─┬─2*[agetty]
        ├─cron
        ├─dbus-daemon
        ├─init-systemd(Ub─┬─SessionLeader───Relay(2742)─┬─cpptools-srv───19*[{cpptools-srv}]
        │                 │                             └─sh───sh───sh───node─┬─node─┬─cpptoo+
        │                 │                                                   │      ├─node──+++
        │                 │                                                   │      └─14*[{n+
        │                 │                                                   ├─node───12*[{n+
        │                 │                                                   ├─node─┬─bash
        │                 │                                                   │      └─11*[{n+
        │                 │                                                   └─10*[{node}]
        │                 ├─SessionLeader───Relay(2765)───node───6*[{node}]
        │                 ├─SessionLeader───Relay(2774)───node───6*[{node}]
        │                 ├─SessionLeader───Relay(10083)───bash───pstree
        │                 ├─init───{init}
        │                 ├─login───bash
        │                 └─{init-systemd(Ub}
        ├─networkd-dispat
        ├─packagekitd───2*[{packagekitd}]
        ├─polkitd───2*[{polkitd}]
        ├─rsyslogd───3*[{rsyslogd}]
        ├─snapd───18*[{snapd}]
        ├─8*[snapfuse]
        ├─subiquity-serve───python3.10─┬─python3
        │                              └─5*[{python3.10}]
        ├─systemd───(sd-pam)
        ├─systemd-journal
        ├─systemd-logind
        ├─systemd-resolve
        ├─systemd-udevd
        └─unattended-upgr───{unattended-upgr}

xdg-open . apre la cartella corrente con il programma predefinito per la cartella
dopo aver installato i dovuti visualizzatori di cartelle

find ˜ -type d -empty -ls trova le cartelle vuote nella home e le elenca



19. 
Scrivete, compilate e testate un programma C che. . .
esce con exit-status 42; potete usare, indifferentemente, 
return dal main o la funzione exit(3) (dove il
numero 3 indica la sezione del manuale, ovviamente). 
Verificate che il programma funzioni visualizzando,
dopo averlo lanciato, il valore della variabile $? dalla shell */

#include <stdlib.h>\

int main(void){
    exit(42);
}
