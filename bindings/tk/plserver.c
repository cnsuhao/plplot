/* $Id$
 * $Log$
 * Revision 1.18  1993/12/21 10:21:06  mjl
 * Changed -client arg name to -client_name to be more transparent (stands
 * for client program main window name).  Substantially rewrote
 * initialization to be better suited for Tcl-DP or TK style communication.
 * Created client_cmd function for handling all messages to the client.
 *
 * Revision 1.17  1993/12/15  08:59:03  mjl
 * Changes to support Tcl-DP.  Also moved functions Tcl_AppInit() and
 * set_autopath() to tkshell.c so they could be used by the TK driver
 * initialization as well.
 *
 * Revision 1.16  1993/12/09  21:29:47  mjl
 * Fixed another bug generated in the reorganization dealing with cleanup.
 *
 * Revision 1.15  1993/12/09  20:25:57  mjl
 * Completely reorganized, using the source for wish (tkMain.c) as a starting
 * point.  Will now be able to track changes to Tcl/TK more easily, and can
 * use plserver exactly like wish (including interactive usage).  Fixed a bug
 * occasionally encountered when shutting down.
 *
 * Revision 1.14  1993/11/19  07:31:35  mjl
 * Updated to new call syntax for tk_toplevel().
 *
 * Revision 1.13  1993/09/27  20:34:25  mjl
 * Eliminated some cases of freeing unallocated memory.
 *
 * Revision 1.12  1993/09/08  18:38:24  mjl
 * Changed conditional compile for Tk 3.2 to expand for any pre-3.3 version.
 *
 * Revision 1.11  1993/09/08  02:31:34  mjl
 * Fixes to work with TK 3.3b3.  Search path for autoloaded Tcl procs now
 * follows ".", ${PLPLOT_DIR}/tcl, ${HOME}/tcl, INSTALL_DIR/tcl, where
 * INSTALL_DIR is passed in from the makefile.
*/

/* 
 * plserver.c
 * Maurice LeBrun
 * 30-Apr-93
 *
 * Plplot graphics server.
 *
 * Is typically run as a child process from the plplot TK driver to render
 * output.  Can use either TK send or Tcl-DP RPC for communication,
 * depending on how it is invoked.
 *
 * Also plserver can be used the same way as wish or dpwish, as it
 * contains the functionality of each of these (except the -notk Tcl-DP
 * command-line option is not supported).  In the source code I've changed
 * as few lines as possible from the source for "wish" in order to make it
 * easier to track future changes to Tcl/TK and Tcl-DP.
 */

#include "plserver.h"

/* Externals */

extern int   plFrameCmd     	(ClientData, Tcl_Interp *, int, char **);

/*
 * Global variables used by the main program:
 */

static Tk_Window mainWindow;	/* The main window for the application.  If
				 * NULL then the application no longer
				 * exists. */
static Tcl_Interp *interp;	/* Interpreter for this application. */
char *tcl_RcFileName = NULL;	/* Name of a user-specific startup script
				 * to source if the application is being run
				 * interactively (e.g. "~/.wishrc").  Set
				 * by Tcl_AppInit.  NULL means don't source
				 * anything ever. */
static Tcl_DString command;	/* Used to assemble lines of terminal input
				 * into Tcl commands. */
static int tty;			/* Non-zero means standard input is a
				 * terminal-like device.  Zero means it's
				 * a file. */
static char errorExitCmd[] = "exit 1";

/*
 * Command-line options:
 */

static int synchronize = 0;
static char *fileName = NULL;
static char *name = NULL;
static char *display = NULL;
static char *geometry = NULL;

/* plserver additions */

static char *client;		/* Name of client main window */
static char *auto_path;		/* addition to auto_path */
static int child;		/* set if child of TK driver */
static int mkidx;		/* Create a new tclIndex file */
static int pass_thru;		/* Skip normal error termination when set */

/* These are for supporting Tcl-DP communication */

static int dp;			/* set if using Tcl-DP to communicate */
static char *client_host;	/* Host id for client */
static char *client_port;	/* Communications port id for client */

static Tk_ArgvInfo argTable[] = {
    {"-file", TK_ARGV_STRING, (char *) NULL, (char *) &fileName,
	"File from which to read commands"},
    {"-geometry", TK_ARGV_STRING, (char *) NULL, (char *) &geometry,
	"Initial geometry for window"},
    {"-display", TK_ARGV_STRING, (char *) NULL, (char *) &display,
	"Display to use"},
    {"-name", TK_ARGV_STRING, (char *) NULL, (char *) &name,
	"Name to use for application"},
    {"-sync", TK_ARGV_CONSTANT, (char *) 1, (char *) &synchronize,
	"Use synchronous mode for display server"},

/* plserver additions */

    {"-client", TK_ARGV_STRING, (char *) NULL, (char *) &client,
	 "Client to connect to at startup"},
    {"-client_host", TK_ARGV_STRING, (char *) NULL, (char *) &client_host,
	 "Host to connect to at startup"},
    {"-client_port", TK_ARGV_STRING, (char *) NULL, (char *) &client_port,
	 "Communications port to connect to at startup"},
    {"-auto_path", TK_ARGV_STRING, (char *) NULL, (char *) &auto_path,
	 "Additional directory(s) to autoload"},
    {"-mkidx", TK_ARGV_CONSTANT, (char *) 1, (char *) &mkidx,
	 "Create new tclIndex file"},
    {"-child", TK_ARGV_CONSTANT, (char *) 1, (char *) &child,
	 "Set ONLY when child of plplot TK driver"},
    {(char *) NULL, TK_ARGV_END, (char *) NULL, (char *) NULL,
	 (char *) NULL}
};

/* Support routines for plserver */

static void  configure		(void);
static void  InitLink_tk	(ClientData);
static void  InitLink_dp	(ClientData);
static void  abort_session	(char *);
static void  client_cmd		(char *);
static void  tcl_cmd		(char *);
static int   tcl_eval		(char *);

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		Prompt _ANSI_ARGS_((Tcl_Interp *interp, int partial));
static void		StdinProc _ANSI_ARGS_((ClientData clientData,
			    int mask));

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for Wish.
 *
 * Results:
 *	None. This procedure never returns (it exits the process when
 *	it's done
 *
 * Side effects:
 *	This procedure initializes the wish world and then starts
 *	interpreting commands;  almost anything could happen, depending
 *	on the script being interpreted.
 *
 *----------------------------------------------------------------------
 */

#define DEBUG

int
main(argc, argv)
    int argc;				/* Number of arguments. */
    char **argv;			/* Array of argument strings. */
{
    char *args, *p, *msg;
    char buf[20];
    int code;

#ifdef DEBUG
{
    int i;
    fprintf(stderr, "Program %s called with arguments :\n", argv[0]);
    for (i = 1; i < argc; i++) 
	fprintf(stderr, "%s ", argv[i]);
    fprintf(stderr, "\n");
}
#endif
    interp = Tcl_CreateInterp();
#ifdef TCL_MEM_DEBUG
    Tcl_InitMemory(interp);
#endif
    Tcl_SetVar(interp, "plsend", "send", 0);

    /*
     * Parse command-line arguments.
     */

    if (Tk_ParseArgv(interp, (Tk_Window) NULL, &argc, argv, argTable, 0)
	    != TCL_OK) {
	abort_session(interp->result);
	exit(1);
    }
    if (name == NULL) {
	if (fileName != NULL) {
	    p = fileName;
	} else {
	    p = argv[0];
	}
	name = strrchr(p, '/');
	if (name != NULL) {
	    name++;
	} else {
	    name = p;
	}
    }

    /*
     * If a display was specified, put it into the DISPLAY
     * environment variable so that it will be available for
     * any sub-processes created by us.
     */

    if (display != NULL) {
	Tcl_SetVar2(interp, "env", "DISPLAY", display, TCL_GLOBAL_ONLY);
    }

    /*
     * Initialize the Tk application.
     */

    mainWindow = Tk_CreateMainWindow(interp, display, name, "Tk");
    if (mainWindow == NULL) {
	abort_session(interp->result);
	exit(1);
    }
    if (synchronize) {
	XSynchronize(Tk_Display(mainWindow), True);
    }
    Tk_GeometryRequest(mainWindow, 200, 200);

    /*
     * Make command-line arguments available in the Tcl variables "argc"
     * and "argv".  Also set the "geometry" variable from the geometry
     * specified on the command line.
     */

    args = Tcl_Merge(argc-1, argv+1);
    Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
    ckfree(args);
    sprintf(buf, "%d", argc-1);
    Tcl_SetVar(interp, "argc", buf, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "argv0", (fileName != NULL) ? fileName : argv[0],
	    TCL_GLOBAL_ONLY);
    if (geometry != NULL) {
	Tcl_SetVar(interp, "geometry", geometry, TCL_GLOBAL_ONLY);
    }

    /*
     * Set the "tcl_interactive" variable.
     */

    tty = isatty(0);
    Tcl_SetVar(interp, "tcl_interactive",
	    ((fileName == NULL) && tty) ? "1" : "0", TCL_GLOBAL_ONLY);

    /*
     * Invoke application-specific initialization.
     */

    configure();
    if (Tcl_AppInit(interp) != TCL_OK) {
	abort_session(interp->result);
	Tcl_Eval(interp, "exit");
    }

/* Create new tclIndex file -- a convenience */

    if (mkidx != 0) {
	tcl_cmd("auto_mkindex . *.tcl");
	client = NULL;
	abort_session("");
	exit(1);
    }

    /*
     * Set the geometry of the main window, if requested.
     */

    if (geometry != NULL) {
	code = Tcl_VarEval(interp, "wm geometry . ", geometry, (char *) NULL);
	if (code != TCL_OK) {
	    fprintf(stderr, "%s\n", interp->result);
	}
    }

    /*
     * Invoke the script specified on the command line, if any.
     * If invoked as a child process of the plplot/tk driver, configure
     * main window.
     */

    if (client_port != NULL) {
#ifdef TCL_DP
	tcl_cmd("$plserver_init_proc");
	Tk_DoWhenIdle(InitLink_dp, (ClientData) client);
	tty = 0;
#else
	abort_session("no Tcl-DP support in this version of plserver");
	exit(1);
#endif
    }
    else if (client != NULL) {
	tcl_cmd("$plserver_init_proc");
	Tk_DoWhenIdle(InitLink_tk, (ClientData) client);
	tty = 0;
    }
    else if (fileName != NULL) {
	code = Tcl_VarEval(interp, "source ", fileName, (char *) NULL);
	if (code != TCL_OK) {
	    goto error;
	}
	tty = 0;
    } else {
	/*
	 * Commands will come from standard input, so set up an event
	 * handler for standard input.  If the input device is aEvaluate the
	 * .rc file, if one has been specified, set up an event handler
	 * for standard input, and print a prompt if the input
	 * device is a terminal.
	 */

	if (tcl_RcFileName != NULL) {
	    Tcl_DString buffer;
	    char *fullName;
	    FILE *f;
    
	    fullName = Tcl_TildeSubst(interp, tcl_RcFileName, &buffer);
	    if (fullName == NULL) {
		fprintf(stderr, "%s\n", interp->result);
	    } else {
		f = fopen(fullName, "r");
		if (f != NULL) {
		    code = Tcl_EvalFile(interp, fullName);
		    if (code != TCL_OK) {
			fprintf(stderr, "%s\n", interp->result);
		    }
		    fclose(f);
		}
	    }
	    Tcl_DStringFree(&buffer);
	}
	Tk_CreateFileHandler(0, TK_READABLE, StdinProc, (ClientData) 0);
	if (tty) {
	    Prompt(interp, 0);
	}
    }
    fflush(stdout);
    Tcl_DStringInit(&command);

    /*
     * Loop infinitely, waiting for commands to execute.  When there
     * are no windows left, Tk_MainLoop returns and we exit.
     */

    Tk_MainLoop();

    /*
     * Don't exit directly, but rather invoke the Tcl "exit" command.
     * This gives the application the opportunity to redefine "exit"
     * to do additional cleanup.
     */

    Tcl_Eval(interp, "exit");
    exit(1);

error:
    msg = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    if (msg == NULL) {
	msg = interp->result;
    }
    abort_session(msg);
    Tcl_Eval(interp, errorExitCmd);
    return 1;			/* Needed only to prevent compiler warnings. */
}

/*
 *----------------------------------------------------------------------
 *
 * StdinProc --
 *
 *	This procedure is invoked by the event dispatcher whenever
 *	standard input becomes readable.  It grabs the next line of
 *	input characters, adds them to a command being assembled, and
 *	executes the command if it's complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Could be almost arbitrary, depending on the command that's
 *	typed.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
static void
StdinProc(clientData, mask)
    ClientData clientData;		/* Not used. */
    int mask;				/* Not used. */
{
#define BUFFER_SIZE 4000
    char input[BUFFER_SIZE+1];
    static int gotPartial = 0;
    char *cmd;
    int code, count;

    count = read(fileno(stdin), input, BUFFER_SIZE);
    if (count <= 0) {
	if (!gotPartial) {
	    if (tty) {
		abort_session("");
		Tcl_Eval(interp, "exit");
	    } else {
		Tk_DeleteFileHandler(0);
	    }
	    return;
	} else {
	    count = 0;
	}
    }
    cmd = Tcl_DStringAppend(&command, input, count);
    if (count != 0) {
	if ((input[count-1] != '\n') && (input[count-1] != ';')) {
	    gotPartial = 1;
	    goto prompt;
	}
	if (!Tcl_CommandComplete(cmd)) {
	    gotPartial = 1;
	    goto prompt;
	}
    }
    gotPartial = 0;

    /*
     * Disable the stdin file handler while evaluating the command;
     * otherwise if the command re-enters the event loop we might
     * process commands from stdin before the current command is
     * finished.  Among other things, this will trash the text of the
     * command being evaluated.
     */

    Tk_CreateFileHandler(0, 0, StdinProc, (ClientData) 0);
    code = Tcl_RecordAndEval(interp, cmd, 0);
    Tk_CreateFileHandler(0, TK_READABLE, StdinProc, (ClientData) 0);
    Tcl_DStringFree(&command);
    if (*interp->result != 0) {
	if ((code != TCL_OK) || (tty)) {
	    printf("%s\n", interp->result);
	}
    }

    /*
     * Output a prompt.
     */

    prompt:
    if (tty) {
	Prompt(interp, gotPartial);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Prompt --
 *
 *	Issue a prompt on standard output, or invoke a script
 *	to issue the prompt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A prompt gets output, and a Tcl script may be evaluated
 *	in interp.
 *
 *----------------------------------------------------------------------
 */

static void
Prompt(interp, partial)
    Tcl_Interp *interp;			/* Interpreter to use for prompting. */
    int partial;			/* Non-zero means there already
					 * exists a partial command, so use
					 * the secondary prompt. */
{
    char *promptCmd;
    int code;

    promptCmd = Tcl_GetVar(interp,
	partial ? "tcl_prompt2" : "tcl_prompt1", TCL_GLOBAL_ONLY);
    if (promptCmd == NULL) {
	defaultPrompt:
	if (!partial) {
	    fputs("% ", stdout);
	}
    } else {
	code = Tcl_Eval(interp, promptCmd);
	if (code != TCL_OK) {
	    Tcl_AddErrorInfo(interp,
		    "\n    (script that generates prompt)");
	    fprintf(stderr, "%s\n", interp->result);
	    goto defaultPrompt;
	}
    }
    fflush(stdout);
}

/*----------------------------------------------------------------------*\
* abort_session
*
* Terminates with an error, doing whatever cleanup appears to be necessary.
\*----------------------------------------------------------------------*/

static void
abort_session(char *errmsg)
{
    dbug_enter("abort_session");

/* Safety check for out of control code */

    if (pass_thru)
	return;

    pass_thru = 1;

/* Print error message if one given */

    if (*errmsg != '\0')
	fprintf(stderr, "%s\n", errmsg);

/* If client exists, tell it to self destruct */

    if (client != NULL)
	client_cmd("abort");

    Tcl_Eval(interp, "exit");
}

/*----------------------------------------------------------------------*\
* configure
*
* Does global variable & command initialization for interpreter.
\*----------------------------------------------------------------------*/

static void
configure(void)
{
    dbug_enter("configure");

/* Tell interpreter about commands. */

    Tcl_CreateCommand(interp, "plframe", plFrameCmd,
                      (ClientData) mainWindow, (void (*)(ClientData)) NULL);

/* Default initialization proc */

    Tcl_SetVar(interp, "plserver_init_proc", "plserver_init", 0);

/* Now variable information. */
/* For uninitialized variables it's better to skip the Tcl_SetVar. */

/* Set program name as main window name and initialize send command */

    Tcl_SetVar(interp, "plserver", Tk_Name(mainWindow), 0);

/* Pass client, host, port, and child variables to interpreter if set. */

    if (client != NULL)
	Tcl_SetVar(interp, "client", client, 0);

    if (client_host != NULL)
	Tcl_SetVar(interp, "client_host", client_host, 0);
    else
	Tcl_SetVar(interp, "client_host", "localhost", 0);

    if (client_port != NULL)
	Tcl_SetVar(interp, "client_port", client_port, 0);

    if (child != 0)
	Tcl_SetVar(interp, "child", "1", 0);
}

/*----------------------------------------------------------------------*\
* InitLink_tk
*
* Sets up link to client program using tk send
\*----------------------------------------------------------------------*/

static void
InitLink_tk(ClientData clientData)
{
    client_cmd("set plserver [list $plserver]");
    client_cmd("set server_is_ready 1");
}

/*----------------------------------------------------------------------*\
* InitLink_dp
*
* Sets up link to client program using Tcl-DP
\*----------------------------------------------------------------------*/

static void
InitLink_dp(ClientData clientData)
{
#ifdef TCL_DP
    tcl_cmd("set plsend dp_RPC");
    tcl_cmd("set client [dp_MakeRPCClient $client_host $client_port]");
    tcl_cmd("set server_host localhost");
    tcl_cmd("set server_port [dp_MakeRPCServer]");

    client_cmd("[list set client $client]");
    client_cmd("[list set server_host $server_host]");
    client_cmd("[list set server_port $server_port]");
    client_cmd("set server_is_ready 1");
#endif
}

/*----------------------------------------------------------------------*\
* server_cmd
*
* Sends specified command to server, aborting on an error.
\*----------------------------------------------------------------------*/

static void
server_cmd(char *cmd)
{
    int result;

    dbug_enter("server_cmd");
#ifdef DEBUG_ENTER
    fprintf(stderr, "Sending command: %s\n", cmd);
#endif

    if (tcl_dp)
	result = Tcl_VarEval(interp,
			     "dp_RPC $plserver -events all ", cmd,
			     (char **) NULL);
    else
	result = Tcl_VarEval(interp,
			     "send $plserver after 1 ", cmd,
			     (char **) NULL);

    if (result) {
	fprintf(stderr, "Server command \"%s\" failed:\n\t %s\n",
		cmd, interp->result);
	abort_session("");
    }
}

/*----------------------------------------------------------------------*\
* tcl_cmd
*
* Evals the specified command, aborting on an error.
\*----------------------------------------------------------------------*/

static void
tcl_cmd(char *cmd)
{
    dbug_enter("tcl_cmd");
#ifdef DEBUG_ENTER
    fprintf(stderr, "plserver: evaluating command %s\n", cmd);
#endif

    if (tcl_eval(cmd)) {
	fprintf(stderr, "plserver: TCL command \"%s\" failed:\n\t %s\n",
		cmd, interp->result);
	abort_session("");
	Tcl_Eval(interp, "exit");
    }
}

/*----------------------------------------------------------------------*\
* tcl_eval
*
* Evals the specified string, returning the result.
* Use a static string buffer to hold the command, to ensure it's in
* writable memory (grrr...).
\*----------------------------------------------------------------------*/

static char *cmdbuf = NULL;
static int cmdbuf_len = 100;

static int
tcl_eval(char *cmd)
{
    if (cmdbuf == NULL) 
	cmdbuf = (char *) malloc(cmdbuf_len);

    if (strlen(cmd) >= cmdbuf_len) {
	free((void *) cmdbuf);
	cmdbuf_len = strlen(cmd) + 20;
	cmdbuf = (char *) malloc(cmdbuf_len);
    }

    strcpy(cmdbuf, cmd);
    return(Tcl_VarEval(interp, cmdbuf, (char **) NULL));
}
