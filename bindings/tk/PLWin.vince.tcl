# PLWin.tcl
# Geoffrey Furnish
# 9 May 1994
#
# @> [incr Tcl] interface to PLplot
#
# $Id$
#
# $Log$
# Revision 1.1  1996/10/25 20:18:15  furnish
# Temporarily add this file, while investigating how to move forward on
# integrating Vince's work.
#
# Revision 1.7  1994/10/10  19:44:45  furnish
# Stub method for plshade.
#
# Revision 1.6  1994/10/10  17:22:32  furnish
# New method, but need many more.
#
# Revision 1.5  1994/06/16  18:33:51  mjl
# Modified argument lists for plline and plpoin methods to use matrix names.
#
# Revision 1.4  1994/06/10  20:46:58  furnish
# Mirror plpoin.  More of the API still needs doing.
#
# Revision 1.3  1994/06/09  20:07:13  mjl
# Cleaned up and switched to plplot direct commands embedded in plframe
# widget (through "<widget> cmd <command> <args>" syntax).
#
# Revision 1.2  1994/05/11  08:07:30  furnish
# Debugging.  Needs to do own toprow for some reason.
#
# Revision 1.1  1994/05/09  17:59:19  furnish
# The new [incr Tcl] interface to the PLplot Tcl extensions.
#
###############################################################################

package require Itk

itcl::class PLWin {
    inherit itk::Widget

    constructor {args} {
	itk_component add plwin {
	    plframe $itk_interior.plwin -relief sunken
	} { 
	    keep -width -height
	}
	pack $itk_component(plwin) -side bottom -expand yes -fill both

	eval itk_initialize $args
    }

    method plcol {color} {
	$itk_interior.plwin cmd plcol0 $color
    }

    method plcont {data clev} {
	upvar $data d
	upvar $clev c
	$itk_interior.plwin cmd plcont d c
    }

    method plenv {xmin xmax ymin ymax just axis} {
	$itk_interior.plwin cmd plenv $xmin $xmax $ymin $ymax $just $axis
    }

    method pllab {xlab ylab toplab} {
	$itk_interior.plwin cmd pllab $xlab $ylab $toplab
    }

    method plline {n x y} {
	$itk_interior.plwin cmd plline $n $x $y
    }

    method plpoin {n x y code} {
	$itk_interior.plwin cmd plpoin $n $x $y $code
    }

    method plshade {data xmin xmax ymin ymax sh_min sh_max sh_col} {
	$itk_interior.plwin cmd plshade $data $xmin $xmax $ymin $ymax \
	    $sh_min $sh_max 1 $sh_col 
    }

    public variable name
}
