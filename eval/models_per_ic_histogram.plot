set terminal pdf
set output "histo.pdf"
set boxwidth 0.75 absolute
set style fill solid 1.0 border -1
set style histogram rowstacked
set style data histograms
set xtics rotate
set ytics 10
set yrange [0:170]
set ylabel "#"
set xlabel "Models"
plot "res/histograms/model_data/combined" using 2 title "AIC", '' using 3 title "AICcC", '' using 4 title "AICcRC", '' using 5 title "BICc", '' using 6:xtic(1) title "BICcRC"
