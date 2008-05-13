-- $Id$

-- Simple vector plot example

-- Copyright (C) 2008 Jerry Bauck

-- This file is part of PLplot.

-- PLplot is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Library Public License as published
-- by the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.

-- PLplot is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Library General Public License for more details.

-- You should have received a copy of the GNU Library General Public License
-- along with PLplot; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

with
    System,
    Ada.Numerics,
    Ada.Numerics.Long_Elementary_Functions,
    PLplot_Traditional,
    PLplot_Auxiliary;
use
    Ada.Numerics,
    Ada.Numerics.Long_Elementary_Functions,
    System,
    PLplot_Traditional,
    PLplot_Auxiliary;

-- COMMENT THIS LINE IF YOUR COMPILER DOES NOT INCLUDE THESE 
-- DEFINITIONS, FOR EXAMPLE, IF IT IS NOT ADA 2005 WITH ANNEX G.3 COMPLIANCE.
--with Ada.Numerics.Long_Real_Arrays; use Ada.Numerics.Long_Real_Arrays;
@Ada_Is_2007_With_and_Use_Numerics@

procedure x22a is
    -- Pairs of points making the line segments used to plot the user defined arrow 
    arrow_x  : Real_Vector(0 .. 5) := (-0.5, 0.5, 0.3, 0.5,  0.3, 0.5);
    arrow_y  : Real_Vector(0 .. 5) := ( 0.0, 0.0, 0.2, 0.0, -0.2, 0.0);
    arrow2_x : Real_Vector(0 .. 5) := (-0.5, 0.3, 0.3, 0.5,  0.3, 0.3);
    arrow2_y : Real_Vector(0 .. 5) := ( 0.0, 0.0, 0.2, 0.0, -0.2, 0.0);


    -- Vector plot of the circulation about the origin
    procedure circulation is
        dx, dy, x, y : Long_Float;
        nx : constant Integer := 20;
        ny : constant Integer := 20;
        xmin, xmax, ymin, ymax : Long_Float;
        u, v : Real_Matrix(0 .. nx - 1, 0 .. ny -1);
        cgrid2 : aliased Transformation_Data_Type_2
           (x_Last => nx - 1,
            y_Last => ny - 1);
    begin
        dx := 1.0;
        dy := 1.0;

        xmin := Long_Float(-nx / 2) * dx;
        xmax := Long_Float( nx / 2) * dx;
        ymin := Long_Float(-ny / 2) * dy;
        ymax := Long_Float( ny / 2) * dy;

        -- Create data - circulation around the origin. 
        for i in 0 .. nx - 1 loop
            x := (Long_Float(i - nx / 2) + 0.5) * dx;
                for j in 0 .. ny - 1 loop
                    y := (Long_Float(j - ny / 2) + 0.5) * dy;
                    cgrid2.xg(i, j) := x;
                    cgrid2.yg(i, j) := y;
                    u(i, j) := y;
                    v(i, j) := -x;
                end loop;
        end loop;

        -- Plot vectors with default arrows 
        plenv(xmin, xmax, ymin, ymax, 0, 0);
        pllab("(x)", "(y)", "#frPLplot Example 22 - circulation");
        plcol0(2);
        plvect(u, v, 0.0, pltr2'access, cgrid2'Address);
        plcol0(1);
    end circulation;


    --Vector plot of flow through a constricted pipe
    procedure constriction is
        dx, dy, x, y : Long_Float;
        xmin, xmax, ymin, ymax : Long_Float;
        Q, b, dbdx : Long_Float;
        nx : constant Integer := 20;
        ny : constant Integer := 20;
        u, v : Real_Matrix(0 .. nx - 1, 0 .. ny -1);
        cgrid2 : aliased Transformation_Data_Type_2
           (x_Last => nx - 1,
            y_Last => ny - 1);
    begin
        dx := 1.0;
        dy := 1.0;

        xmin := Long_Float(-nx / 2) * dx;
        xmax := Long_Float( nx / 2) * dx;
        ymin := Long_Float(-ny / 2) * dy;
        ymax := Long_Float( ny / 2) * dy;

        Q := 2.0;
        for i in 0 .. nx - 1 loop
            x := (Long_Float(i - nx / 2) + 0.5) * dx;
                for j in 0 .. ny - 1 loop
                    y := (Long_Float(j - ny / 2) + 0.5) * dy;
                    cgrid2.xg(i, j) := x;
                    cgrid2.yg(i, j) := y;
                    b := ymax / 4.0 * (3.0 - cos(pi * x / xmax));
                    if abs(y) < b then
                        dbdx := ymax / 4.0 * sin(pi * x / xmax) * y / b;
                        u(i, j) := Q * ymax / b;
                        v(i, j) := dbdx * u(i, j);
                    else 
                        u(i, j) := 0.0;
                        v(i, j) := 0.0;
                    end if;
            end loop;
        end loop;

        plenv(xmin, xmax, ymin, ymax, 0, 0);
        pllab("(x)", "(y)", "#frPLplot Example 22 - constriction");
        plcol0(2);
        plvect(u, v, -0.5, pltr2'access, cgrid2'Address);
        plcol0(1);
    end constriction;


    -- Vector plot of the gradient of a shielded potential (see example 9)
    procedure potential is
        nper   : constant Integer := 100;
        nlevel : constant Integer := 10;
        nr     : constant Integer := 20;
        ntheta : constant Integer := 20;

        eps, q1, d1, q1i, d1i, q2, d2, q2i, d2i : Long_Float;
        div1, div1i, div2, div2i : Long_Float;
        r, theta, x, y, dz : Long_Float;
        xmin, xmax, ymin, ymax, rmax, zmax, zmin : Long_Float;
        u, v, z : Real_Matrix(0 .. nr - 1, 0 .. ntheta - 1);
        px, py : Real_Vector(0 .. nper - 1);
        clevel : Real_Vector(0 .. nlevel - 1);
        cgrid2 : aliased Transformation_Data_Type_2
           (x_Last => nr - 1,
            y_Last => ntheta - 1);
        
        function pow(x, y : Long_Float) return Long_Float is
            Result : Long_Float := 1.0;
        begin
            for i in 1 .. Integer(y) loop
                Result := Result * x;
            end loop;
            return Result;
        end pow;

    begin
        -- Potential inside a conducting cylinder (or sphere) by method of images.
        -- Charge 1 is placed at (d1, d1), with image charge at (d2, d2).
        -- Charge 2 is placed at (d1, -d1), with image charge at (d2, -d2).
        -- Also put in smoothing term at small distances.
        rmax := Long_Float(nr);
        eps := 2.0;

        q1 := 1.0;
        d1 := rmax / 4.0;

        q1i := - q1 * rmax / d1;
        d1i := (rmax * rmax) / d1;

        q2 := -1.0;
        d2 := rmax / 4.0;

        q2i := - q2 * rmax / d2;
        d2i := (rmax * rmax) / d2;

        for i in 0 .. nr - 1 loop
            r := 0.5 + Long_Float(i);
            for j in 0 .. ntheta - 1 loop
                theta := 2.0 * pi / Long_Float(ntheta - 1) * (0.5 + Long_Float(j));
                x := r * cos(theta);
                y := r * sin(theta);
                cgrid2.xg(i, j) := x;
                cgrid2.yg(i, j) := y;
                div1  := sqrt(pow(x-d1, 2.0)  + pow(y-d1, 2.0) + pow(eps, 2.0));
                div1i := sqrt(pow(x-d1i, 2.0) + pow(y-d1i, 2.0) + pow(eps, 2.0));
                div2  := sqrt(pow(x-d2, 2.0)  + pow(y+d2, 2.0) + pow(eps, 2.0));
                div2i := sqrt(pow(x-d2i, 2.0) + pow(y+d2i, 2.0) + pow(eps, 2.0));
                
                z(i, j) := q1/div1 + q1i/div1i + q2/div2 + q2i/div2i;
                u(i, j) := -q1*(x-d1)/pow(div1, 3.0) - q1i*(x-d1i)/pow(div1i, 3.00)
                    - q2*(x-d2)/pow(div2,3.0) - q2i*(x-d2i)/pow(div2i, 3.0);
                v(i, j) := -q1*(y-d1)/pow(div1, 3.0) - q1i*(y-d1i)/pow(div1i, 3.00)
                    - q2*(y+d2)/pow(div2, 3.0) - q2i*(y+d2i)/pow(div2i, 3.0);
            end loop;
        end loop;

        xmin := Matrix_Min(cgrid2.xg);
        xmax := Matrix_Max(cgrid2.xg);
        ymin := Matrix_Min(cgrid2.yg);
        ymax := Matrix_Max(cgrid2.yg);
        zmin := Matrix_Min(z);
        zmax := Matrix_Max(z);

        plenv(xmin, xmax, ymin, ymax, 0, 0);
        pllab("(x)", "(y)", "#frPLplot Example 22 - potential gradient vector plot");

        -- Plot contours of the potential 
        dz := (zmax - zmin) / Long_Float(nlevel);
        for i in clevel'range loop
            clevel(i) := zmin + (Long_Float(i) + 0.5) * dz;
        end loop;
        plcol0(3);
        pllsty(2);
        plcont(z, 1, nr, 1, ntheta, clevel, pltr2'access, cgrid2'Address);
        pllsty(1);
        plcol0(1);

        -- Plot the vectors of the gradient of the potential 
        plcol0(2);
        plvect(u, v, 25.0, pltr2'access, cgrid2'Address);
        plcol0(1);

        -- Plot the perimeter of the cylinder 
        for i in px'range loop
            theta := (2.0 * pi / Long_Float(nper - 1)) * Long_Float(i);
            px(i) := rmax * cos(theta);
            py(i) := rmax * sin(theta);
        end loop;
        plline(px,py);
    end potential;


----------------------------------------------------------------------------
-- Generates several simple vector plots.
----------------------------------------------------------------------------
begin

    -- Parse and process command line arguments 
    plparseopts(PL_PARSE_FULL);

    -- Initialize plplot 
    plinit;

    circulation;

    -- Set arrow style using arrow_x and arrow_y then plot using these arrows.
    plsvect(arrow_x, arrow_y, False);
    constriction;

    -- Set arrow style using arrow2_x and arrow2_y then plot using these filled arrows. 
    plsvect(arrow2_x, arrow2_y, True);
    constriction;

    potential;

    plend;
end x22a;
