#! /bin/bash
# Expect protocol name as first parameter (tcp or udp)

# Define input and output file names
ThroughFile="../data/$1_throughput.dat";
PngName="../data/LB$1.png";

#getting the first and the last line of the file
HeadLine=($(head $ThroughFile --lines=1))
TailLine=($(tail $ThroughFile --lines=1))

#getting the information from the first and the last line
FirstN=${HeadLine[0]} #N1
LastN=${TailLine[0]} #N2

# TO BE DONE START

FirstT=${HeadLine[1]}
LastT=${TailLine[1]}

FirstD=$(echo "scale=15; $FirstN / $FirstT" | bc)
LastD=$(echo "scale=15; $LastN / $LastT" | bc)

Band=$(echo "scale=15; ($LastN - $FirstN) / ($LastD - $FirstD)" | bc)

Latency=$(echo "scale=15; ($FirstD * $LastN - $LastD * $FirstN) / ($LastN - $FirstN)" | bc)

# TO BE DONE END

# Plotting the results
gnuplot <<-eNDgNUPLOTcOMMAND
  set term png size 900, 700
  set output "${PngName}"
  set logscale x 2
  set logscale y 10
  set xlabel "msg size (B)"
  set ylabel "throughput (KB/s)"
  set xrange[$FirstN:$LastN]
  lbmodel(x)= x / ($Latency + (x/$Band))

# TO BE DONE START

    lbf(x) = x / ( $Latency + x / $Band )
    plot lbf(x) title "Latency-Bandwidth model with L=$Latency and B=$Band" with linespoints, \
       "../data/$1_throughput.dat" using 1:2 title "median Throughput" with linespoints

# TO BE DONE END

  clear

eNDgNUPLOTcOMMAND