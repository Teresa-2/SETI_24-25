# RELAZIONE LABORATORIO INCAPACHE A.A. 2024/2025

### Partecipanti
- Teresa de Jesus Fernandes, 4190022
- Elisa Gotelli, 5666586

## ORGANIZZAZIONE MATERIALI
La cartella presenta i file compilati e, in aggiunta:
- la directory "debug" che contiene al suo interno tre script di testing per verificare il corretto funzionamento del laboratorio
- questa relazione

## STATO DI COMPLETAMENTO
- incapache 8.0 e 8.1 completati
- metodo opzionale POST non implementato

## DEBUG
- sono state inserite all'interno del codice alcune linee di debug per verificare la corretta esecuzione dei diversi metodi
- abbiamo utilizzato il debugger gdb per risolvere gli errori del codice e verificarne il corretto funzionamento
- nella directory "debug" sono stati inseriti tre diversi test, utili a testare il corretto funzionamento del progetto. In particolare: 
    - test1.sh, test2.sh e threads.sh funzionano con gli errori indicati nella sezione seguente *"Problematiche"*
    - inoltre in test1.sh è stato necessario inserire, tra una richiesta e quella successiva, una breve interruzione (mediante funzione SLEEP) per evitare un sovraccarico al gestore delle richieste del server. Ad esempio, in assenza di SLEEP, l'esecuzione del test1.sh (talvolta) ha fallito. Analogamente sono state inserite anche alcune pause nel test test2.sh
    - test1.sh eseguito in loop per 100 volte funzionava, tuttavia gli altri due test (test2.sh e thread.sh) quando eseguiti in loop per 10 o 100 iterazioni non riuscivano a completare tutte le operazioni richieste, interrompendosi prima del previsto
    

## PROBLEMATICHE
- non riusciamo a gestire i casi di conferimento di cookie multipli in una singola richiesta. In particolare, il server legge la richiesta del client ma non salva i cookie successivi al primo presentati nella richiesta stessa (ipotizziamo una cattiva implementazione della funzione *parse_cookie*)
- in caso di richieste molto numerose, osserviamo che il server non accetta più connessioni e ipotizziamo che questo sia dovuto alla presenza di un firewall o che questo avvenga per motivi di sicurezza
- quando il client indica nella richiesta un valore di cookie non idoneo per il server (e.g. valore out of bound), il server attribuisce a tale client un cookie di valore pari a quello indicato nella variabile CurUID e resetta di conseguenza il contatore associato a tale cookie (perdendo così il numero di accessi relativi al cookie = CurUID precedentemente salvati nel contatore)
- non funzionano i test proposti in loop