/* $Id$
 * $Log$
 * Revision 1.4  1994/07/19 22:33:16  mjl
 * Internal header file inclusion changed to /not/ use a search path so that
 * it will work better with makedepend.
 *
 * Revision 1.3  1994/06/30  18:55:50  mjl
 * Minor changes to eliminate gcc -Wall warnings.
 *
 * Revision 1.2  1994/06/24  20:41:35  mjl
 * Added error handler specific to pltcl.  Ensures output device is in text
 * mode before issuing error message.
 *
 * Revision 1.1  1994/06/23  22:51:28  mjl
 * A plotting interpreter that uses Tcl to drive PLplot primitives.  This can
 * be used with virtually any PLplot output driver.  The executable is an
 * extended tclsh that has been embellished with a (soon to be) large set
 * of Tcl commands for executing PLplot graphics calls.  The scripts are not
 * the same as those that plserver can execute, as the latter is object-based
 * and uses widget commands, whereas pltcl uses global commands to drive
 * PLplot.  The two style of commands are similar enough, however (differing
 * only by an introducer) that a text filter could be used to go between
 * them.
 */

/*----------------------------------------------------------------------*\
 * 
 * pltcl.c --
 *
 *	Main program for Tcl-interface to PLplot.  Allows interpretive
 *	execution of plotting primitives without regard to output driver.
 *
 * Maurice LeBrun
 * IFS, University of Texas at Austin
 * 19-Jun-1994
 *
\*----------------------------------------------------------------------*/

#include "plplotP.h"
#include "pltcl.h"

static void
plErrorHandler(Tcl_Interp *interp, int code, int tty);

extern void (*tclErrorHandler)(Tcl_Interp *interp, int code, int tty);

/*----------------------------------------------------------------------*\
 * main --
 *
 * Just a stub routine to call pltclMain.  The latter is nice to have when
 * building extended tclsh's, since then you don't have to rely on sucking
 * the Tcl main out of libtcl (which doesn't work correctly on all
 * systems/compilers/linkers/etc).  Hopefully in the future Tcl will
 * supply a sufficiently capable tclMain() type function that can be used
 * instead.
\*----------------------------------------------------------------------*/

int
main(int argc, char **argv)
{
    (void) plParseInternalOpts(&argc, argv, PL_PARSE_FULL);

    exit(pltclMain(argc, argv));
}

/*----------------------------------------------------------------------*\
 * plExitCmd
 *
 * PLplot/Tcl extension command -- handle exit.
 * The reason for overriding the normal exit command is so we can tell
 * the PLplot library to clean up.
\*----------------------------------------------------------------------*/

static int
plExitCmd(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{

/* Print error message if one given */

    if (interp->result != '\0')
	fprintf(stderr, "%s\n", interp->result);

    plspause(0);
    plend();

    Tcl_UnsetVar(interp, "tcl_prompt1", 0);
    Tcl_Eval(interp, "tclexit");

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    if (Tcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (Pltcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

/* Application-specific startup.  That means: for use in pltcl ONLY. */

/* Rename "exit" to "tclexit", and insert custom exit handler */

    Tcl_VarEval(interp, "rename exit tclexit", (char *) NULL);

    Tcl_CreateCommand(interp, "exit", plExitCmd,
                      (ClientData) NULL, (void (*)(ClientData)) NULL);

/* Initialize graphics package */

    plinit();

/* Make sure we are in text mode when interactively entering commands */

    Tcl_SetVar(interp, "tcl_prompt1", "pltext; puts -nonewline \"% \"", 0);

/* Also before an error is printed.  Can't use normal call mechanism here */
/* because it trashes the interp->result string. */

    tclErrorHandler = plErrorHandler;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * plErrorHandler --
 *
 *	Handles error conditions while parsing.  Modified from the version
 *	in tclMain.c to ensure we are on the text screen before issuing
 *	the error message, otherwise it may disappear.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Error info is printed to stdout, if interactive, otherwise stderr.
 *
 *----------------------------------------------------------------------
 */

static void
plErrorHandler(Tcl_Interp *interp, int code, int tty)
{
    pltext();
    if (tty)
	printf("%s\n", interp->result);
    else
	fprintf(stderr, "%s\n", interp->result);
}
