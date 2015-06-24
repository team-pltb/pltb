set terminal pdf font "arial,8"
set output "eval/res/histograms/plots/models_per_ic_histogram.pdf"

#set title "Model selection depicted by information criteria"

set key outside top horizontal noreverse enhanced autotitle columnhead
set style fill solid 1.0 border lt -1
set style data histograms
set xtics rotate

set ylabel "# selections"
set xlabel "Models"

uparam=""
do for [i=2:6] {
    uparam=uparam.sprintf("using %d:xtic(1)", i)
    if (i != 6) {
        uparam=uparam.", ''"
    }
}

set macro

plot "eval/res/histograms/model_data/combined" @uparam
