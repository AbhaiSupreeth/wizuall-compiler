# Gnuplot script: plot.gp
# Sets the output terminal (e.g., PNG, WXT, Aqua)
# Adjust the terminal based on your environment
# set terminal pngcairo size 800,600 enhanced font 'Verdana,10'
# set output 'plot_output.png'
set terminal wxt size 800,600 title 'WIZUALL Scatter Plot' enhanced # Interactive window

# Set plot title and labels
set title "Scatter Plot from WIZUALL"
set xlabel "X Axis"
set ylabel "Y Axis"

# Plot the data from plot_data.txt using column 1 as X and column 2 as Y
plot 'plot_data.txt' using 1:2 with points pointtype 7 title 'Data Points'

# Optional: Pause if using an interactive terminal (like wxt)
# pause -1 "Press Enter or close window to exit..." 