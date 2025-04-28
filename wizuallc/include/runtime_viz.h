#ifndef RUNTIME_VIZ_H
#define RUNTIME_VIZ_H

#include <stdlib.h> // For size_t

/**
 * @brief Runtime function to generate a scatter plot from two vectors.
 *        Writes data to "plot_data.txt" and calls gnuplot via "plot.gp".
 *
 * @param x_data Pointer to the double array for X coordinates.
 * @param x_size Number of elements in x_data.
 * @param y_data Pointer to the double array for Y coordinates.
 * @param y_size Number of elements in y_data.
 */
void c_scatter_plot(double *x_data, size_t x_size, double *y_data, size_t y_size);

#endif // RUNTIME_VIZ_H 