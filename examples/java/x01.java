//---------------------------------------------------------------------------//
// $Id$
//
// Implementation of PLplot example 1 in Java.
//---------------------------------------------------------------------------//

package plplot.examples;

import plplot.core.*;

import java.lang.Math;

class x01 {

    double[] x, y, xs, ys;
    double xscale, yscale, xoff, yoff;
    PLStream pls;

    public static void main( String[] args ) 
    {
    // Now get on with the Java OO way of doing things...
        x01 x = new x01();
        x.run( args );
    }

    x01()
    {
    // Java lacks static arrays.
        x = new double[101];
        y = new double[101];

        xs = new double[6];
        ys = new double[6];
    }

    public void run( String[] args )
    {
        pls = new PLStream();

    // The following is drawn from x01c.c, but I don't currently know exactly
    // how to implement this stuff in Java, so I'm including it here for
    // future reference, but won't be implementing it at this time.
    // Ultimately I would like to see this fully implemented so that x01.java
    // can be a faithful implementation of the full x01c.c example program.

    // plplot initialization
    // Divide page into 2x2 plots unless user overrides.

        pls.ssub(2, 2);

    // Parse and process command line arguments.

//         plMergeOpts(options, "x01c options", notes);
        pls.ParseOpts( args, pls.PL_PARSE_FULL );

//     // Get version number, just for kicks.

//         plgver(ver);
//         fprintf(stderr, "Plplot library version: %s\n", ver);


    // Initialize PLplot.
        pls.init();

    // Select the multi-stroke font.
        pls.fontld( 1 );

    // Set up the data
    // Original case

        xscale = 6.;
        yscale = 1.;
        xoff = 0.;
        yoff = 0.;

    // Do a plot
        plot1(0);

    // Set up the data

        xscale = 1.;
        yscale = 0.0014;
        yoff = 0.0185;

    // Do a plot

        int digmax = 5;
        pls.syax(digmax, 0);

        plot1(1);

        plot2();

        plot3();

    // Let's get some user input

//         if (locate_mode) {
//             for (;;) {
//                 if (! plGetCursor(&gin)) break;
//                 if (gin.keysym == PLK_Escape) break;

//                 pltext();
//                 if (gin.keysym < 0xFF && isprint(gin.keysym)) 
//                     printf("wx = %f,  wy = %f, dx = %f,  dy = %f,  c = '%c'\n",
//                            gin.wX, gin.wY, gin.dX, gin.dY, gin.keysym);
//                 else
//                     printf("wx = %f,  wy = %f, dx = %f,  dy = %f,  c = 0x%02x\n",
//                            gin.wX, gin.wY, gin.dX, gin.dY, gin.keysym);

//                 plgra();
//             }
//         }

    // Don't forget to call plend() to finish off!
        pls.end();
    }

    void plot1( int do_test )
    {
        int i;
        double xmin, xmax, ymin, ymax;

        for( i=0; i < 60; i++ )
        {
            x[i] = xoff + xscale * (i + 1) / 60.0;
            y[i] = yoff + yscale * Math.pow(x[i], 2.);
        }

        xmin = x[0];
        xmax = x[59];
        ymin = y[0];
        ymax = y[59];

        for( i=0; i < 6; i++ )
        {
            xs[i] = x[i * 10 + 3];
            ys[i] = y[i * 10 + 3];
        }

    // Set up the viewport and window using PLENV. The range in X is 0.0 to
    // 6.0, and the range in Y is 0.0 to 30.0. The axes are scaled separately
    // (just = 0), and we just draw a labelled box (axis = 0).

        pls.col0(1);
        pls.env( xmin, xmax, ymin, ymax, 0, 0 );
        pls.col0(2);
        pls.lab( "(x)", "(y)", "#frPLplot Example 1 - y=x#u2" );

    // Plot the data points.

        pls.col0(4);
        pls.poin( 6, xs, ys, 9 );

    // Draw the line through the data.

        pls.col0(3);
        pls.line(60, x, y);
    }

    void plot2()
    {
        int i;

    // Set up the viewport and window using PLENV. The range in X is -2.0 to
    // 10.0, and the range in Y is -0.4 to 2.0. The axes are scaled
    // separately (just = 0), and we draw a box with axes (axis = 1).

        pls.col0(1);
        pls.env(-2.0, 10.0, -0.4, 1.2, 0, 1);
        pls.col0(2);
        pls.lab("(x)", "sin(x)/x", "#frPLplot Example 1 - Sinc Function");

    // Fill up the arrays.

        for (i = 0; i < 100; i++) {
            x[i] = (i - 19.0) / 6.0;
            y[i] = 1.0;
            if (x[i] != 0.0)
                y[i] = Math.sin(x[i]) / x[i];
        }

    // Draw the line.

        pls.col0(3);
        pls.wid(2);
        pls.line(100, x, y);
        pls.wid(1);
    }

    void plot3()
    {
        int space0 = 0, mark0 = 0, space1 = 1500, mark1 = 1500;
        int i;

    // For the final graph we wish to override the default tick intervals,
    // and so do not use plenv().

        pls.adv(0);

    // Use standard viewport, and define X range from 0 to 360 degrees, Y
    // range from -1.2 to 1.2.

        pls.vsta();
        pls.wind( 0.0, 360.0, -1.2, 1.2 );

    // Draw a box with ticks spaced 60 degrees apart in X, and 0.2 in Y.

        pls.col0(1);
        pls.box("bcnst", 60.0, 2, "bcnstv", 0.2, 2);

    // Superimpose a dashed line grid, with 1.5 mm marks and spaces. 
    // plstyl expects a pointer!

        pls.styl(1, mark1, space1);
        pls.col0(2);
        pls.box("g", 30.0, 0, "g", 0.2, 0);
        pls.styl(0, mark0, space0);

        pls.col0(3);
        pls.lab( "Angle (degrees)", "sine",
                 "#frPLplot Example 1 - Sine function" );

        for (i = 0; i < 101; i++) {
            x[i] = 3.6 * i;
            y[i] = Math.sin(x[i] * Math.PI / 180.0);
        }

        pls.col0(4);
        pls.line(101, x, y);
    }
}

//---------------------------------------------------------------------------//
//                              End of x01.java
//---------------------------------------------------------------------------//
