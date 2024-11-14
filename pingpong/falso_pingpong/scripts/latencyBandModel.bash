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
# B = (N2-N1)/(D(N2)-D(N1))
# L0 = (D(N1)*N2 - D(N2)*N1)/(N2-N1)
# T(N) * D(N) = N pertanto D(N) = N / T(N)

# Determination of T(N1) and T(N2)
FirstT=${HeadLine[1]} #T(N1)
LastT=${TailLine[1]} #T(N2)

# Determination of D(N1) and D(N2)
FirstD=$(echo "scale=15; $FirstN / $FirstT" | bc) #D(N1)
LastD=$(echo "scale=15; $LastN / $LastT" | bc) #D(N2)

# Determination of B
Band=$(echo "scale=15; ($LastN - $FirstN) / ($LastD - $FirstD)" | bc)

# Determination of L0
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