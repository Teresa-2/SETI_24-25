#!/bin/bash

set -e # se c'è un errore il programma si ferma

function convert_exp_to_dec {
	echo $(sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g' <<<"$1")
} # per convertire i numeri in notazione esponenziale in numeri decimali perché gnuplot non li legge

function calculate {
	#echo $(expr "$1" | bc -l) "| cut -c 1-9"
	echo $( echo "scale=9; $1" | bc )
} # per calcolare le operazioni matematiche in float perché bash non le fa

function create_graphic {
	startLine=($(head -n 1 < ../data/$1_throughput.dat)) # leggo la prima riga del file
	Byte_min=$(convert_exp_to_dec "${startLine[0]}") 
	T1=$(convert_exp_to_dec "${startLine[1]}")

	endLine=($(tail -n 1 < ../data/$1_throughput.dat)) # leggo l'ultima riga del file
	Byte_Max=$(convert_exp_to_dec "${endLine[0]}")
	T2=$(convert_exp_to_dec "${endLine[1]}")

	Delay_min=$(calculate "(((${Byte_min}) / ${T1}))") # delay min = Byte / Throughput
	Delay_Max=$(calculate "(((${Byte_Max}) / ${T2}))") # delay max = Byte / Throughput
	# calcolo i parametri costanti per la curva LBF
	B=$(calculate "((${Byte_Max} - ${Byte_min}) / (${Delay_Max} - ${Delay_min}))") # bandwidth = (Byte_max - Byte_min) / (Delay_Max - Delay_min)
	L=$(calculate "(((${Delay_min} * ${Byte_Max}) - (${Delay_Max} * ${Byte_min})) / (${Byte_Max} - ${Byte_min}))") # latency = (Delay_min * Byte_max - Delay_max * Byte_min) / (Byte_max - Byte_min)

	# creo il grafico con gnuplot
	# tramite il file ../data/$1_throughput.dat disegno la curva del throughput usando come x la prima colonna e come y la seconda colonna
	# tramite la funzione lbf(x) disegno la curva del modello LBF usando le costanti L e B calcolate precedentemente
	gnuplot <<-eNDgNUPLOTcOMMAND
		set title "$1^^ Throughput vs Message Size"
		set term png size 900, 700
		set output "../data/latency_bandwidth_$1.png"
		set logscale x 2
		set logscale y 10
		set xlabel "msg size (B)"
		set ylabel "throughput (KB/s)"
		lbf(x) = x / ( $L + x / $B )
		plot "../data/$1_throughput.dat" using 1:2 title "$1^^ Throughput" with linespoints, \
			lbf(x) title "Latency-Bandwidth model with L=$(echo $L) and B=$(echo $B)" with linespoints
		clear
	eNDgNUPLOTcOMMAND

	#xdg-open ../data/latency_bandwidth_$1 &
}

create_graphic 'tcp' # chiamo la funzione per creare il grafico con i dati del protocollo TCP
create_graphic 'udp' # chiamo la funzione per creare il grafico con i dati del protocollo UDP