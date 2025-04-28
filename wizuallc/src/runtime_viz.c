#include "runtime_viz.h"
#include <stdio.h>
#include <stdlib.h>

void c_scatter_plot(double *x_data, size_t x_size, double *y_data, size_t y_size) {
    printf("Executing scatter_plot runtime function...\n");

    if (!x_data || !y_data) {
        fprintf(stderr, "Error: scatter_plot received NULL data pointer.\n");
        return;
    }

    if (x_size != y_size) {
        fprintf(stderr, "Error: scatter_plot requires vectors of the same size (%ld != %ld).\n", x_size, y_size);
        return;
    }

    if (x_size == 0) {
        printf("Warning: scatter_plot called with empty vectors.\n");
        return;
    }

    // --- Write data to file ---
    const char* data_filename = "plot_data.txt";
    FILE *fp = fopen(data_filename, "w");
    if (!fp) {
        perror("Error opening plot data file");
        return;
    }

    fprintf(fp, "# X Y\n"); // Header
    for (size_t i = 0; i < x_size; ++i) {
        fprintf(fp, "%f %f\n", x_data[i], y_data[i]);
    }
    fclose(fp);
    printf("Data written to %s\n", data_filename);

    // --- Call Gnuplot --- 
    // Assumes gnuplot is in the system's PATH and plot.gp exists
    const char* gnuplot_command = "gnuplot plot.gp"; 
    printf("Calling Gnuplot: %s\n", gnuplot_command);
    int ret = system(gnuplot_command);
    if (ret != 0) {
        fprintf(stderr, "Warning: system() call to gnuplot returned %d. Is gnuplot installed and in PATH? Does plot.gp exist?\n", ret);
    }
    printf("scatter_plot runtime function finished.\n");
} 