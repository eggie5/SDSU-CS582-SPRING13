#!/usr/bin/gnuplot

# this plots the file called random.dat using ponint size (ps) 0.5 and 
# point type (pt) 7.
plot "fm_demod" with lines
# this makes gnuplot wait 1 second before continuing
pause .5
# this instructs gnuplot to re-plot the file random.dat
replot
# and "reread" does the trick. It instructs gnuplot to reread this file, so 
# it will work as a loop.
reread