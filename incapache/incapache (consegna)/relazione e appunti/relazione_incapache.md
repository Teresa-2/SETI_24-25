# RELAZIONE LABORATORIO INCAPACHE A.A. 2024/2025

### Partecipanti
- Teresa de Jesus Fernandes, 4190022
- Elisa Gotelli, 5666586

## ORGANIZZAZIONE MATERIALI
La cartella presenta i file compilati e, in aggiunta:
- la directory "debug" che contiene al suo interno diversi script di testing per verificare il corretto funzionamento del laboratorio
- questa relazione

## DEBUG
- Sono state inserite all'interno del codice diverse linee di debug per verificare la corretta esecuzione dei diversi metodi
- abbiamo utilizzato il debugger gdb per risolvere gli errori del codice e verificare il corretto funzionamento
- Nella directory "debug" sono stati inseriti 4 diversi test, utili a testare il corretto funzionamento del progetto
- test1, test2 e threads funzionano se eseguiti singolarmente
- test1.sh looppato 100 volte funzionava, tuttavia gli altri due test (test2.sh e thread.sh) quando eseguiti in loop per diverse volte (e.g. 10, 100) non riuscivano a completare tutte le iterazioni richieste. Ipotizziamo che questo comportamento possa essere riconducibile a una delle seguenti ragioni:

## PROBLEMATICHE
- non riusciamo a gestire casi di conferimento di cookie multipli
- in caso di richieste molto numerose osserviamo che il server non accetta più connessioni e ipotizziamo che sia per la presenza del firewall o per motivi di sicurezza
- in caso di errore, riparte da zero il contatore dei cookie
- non funzionano i test "loop". 