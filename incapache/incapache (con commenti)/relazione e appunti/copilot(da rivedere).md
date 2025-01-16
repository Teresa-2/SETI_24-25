# RELAZIONE LABORATORIO INCAPACHE A.A. 2024/2025

### Partecipanti
- Teresa de Jesus Fernandes, 4190022
- Elisa Gotelli, 5666586

## ORGANIZZAZIONE MATERIALI
La cartella presenta i file compilati e, in aggiunta:
- la directory "debug" che contiene al suo interno diversi script di testing per verificare il corretto funzionamento del laboratorio
- questa relazione

## DEBUG
- Sono state inserite all'interno del codice diverse linee di debug per verificare la corretta esecuzione dei diversi metodi.
- Abbiamo utilizzato il debugger gdb per risolvere gli errori del codice e verificare il corretto funzionamento.
- Nella directory "debug" sono stati inseriti 4 diversi test, utili a testare il corretto funzionamento del progetto.
- I test `test1.sh`, `test2.sh` e `threads.sh` funzionano correttamente quando eseguiti singolarmente.
- Tuttavia, quando `test1.sh` viene eseguito in loop per 100 iterazioni, funziona correttamente, mentre `test2.sh` e `threads.sh` non riescono a completare tutte le iterazioni richieste quando eseguiti in loop (e.g., 10, 100 iterazioni). Questo comportamento potrebbe essere dovuto a una delle seguenti ragioni:

## PROBLEMATICHE
- Non riusciamo a gestire correttamente i casi di conferimento di cookie multipli.
- In caso di richieste molto numerose, osserviamo che il server non accetta più connessioni. Ipotizziamo che questo comportamento possa essere dovuto a limiti di connessione simultanea, ritardi nella chiusura delle connessioni, o esaurimento delle risorse del sistema.
- In caso di errore, il contatore dei cookie riparte da zero.
- I test "loop" non funzionano correttamente quando eseguiti in loop per diverse iterazioni. Anche se ogni test rappresenta un'unica connessione, il server sembra non riuscire a gestire correttamente le richieste successive.