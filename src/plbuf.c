//  Handle plot buffer.
//
//  Copyright (C) 1992  Maurice LeBrun
//  Copyright (C) 2004-2014 Alan W. Irwin
//  Copyright (C) 2005  Thomas J. Duck
//  Copyright (C) 2006  Jim Dishaw
//
//  This file is part of PLplot.
//
//  PLplot is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Library General Public License as published
//  by the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  PLplot is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with PLplot; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//

#define NEED_PLDEBUG
#include "plplotP.h"
#include "drivers.h"
#include "metadefs.h"

#include <string.h>

// Function prototypes
void * plbuf_save( PLStream *pls, void *state );

static int      rd_command( PLStream *pls, U_CHAR *p_c );
static void     rd_data( PLStream *pls, void *buf, size_t buf_size );
static void     rd_data_no_copy( PLStream *pls, void **buf, size_t buf_size);
static void     wr_command( PLStream *pls, U_CHAR c );
static void     wr_data( PLStream *pls, void *buf, size_t buf_size );
static void     plbuf_control( PLStream *pls, U_CHAR c );
static void     check_buffer_size( PLStream *pls, size_t data_size );

static void     rdbuf_init( PLStream *pls );
static void     rdbuf_line( PLStream *pls );
static void     rdbuf_polyline( PLStream *pls );
static void     rdbuf_eop( PLStream *pls );
static void     rdbuf_bop( PLStream *pls );
static void     rdbuf_state( PLStream *pls );
static void     rdbuf_esc( PLStream *pls );

static void     plbuf_fill( PLStream *pls );
static void     rdbuf_fill( PLStream *pls );
static void     plbuf_swin( PLStream *pls, PLWindow *plwin );
static void     rdbuf_swin( PLStream *pls );

//--------------------------------------------------------------------------
// Plplot internal interface to the plot buffer
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// plbuf_init()
//
// Initialize device.
// Actually just disables writes if plot buffer is already open (occurs
// when one stream is cloned, as in printing).
//--------------------------------------------------------------------------

void
plbuf_init( PLStream *pls )
{
    dbug_enter( "plbuf_init" );

    pls->plbuf_read = FALSE;

    if ( pls->plbuf_buffer == NULL )
    {
        // We have not allocated a buffer, so do it now
        pls->plbuf_buffer_grow = 128 * 1024;

        if ( ( pls->plbuf_buffer = malloc( pls->plbuf_buffer_grow ) ) == NULL )
            plexit( "plbuf_init: Error allocating plot buffer." );

        pls->plbuf_buffer_size = pls->plbuf_buffer_grow;
        pls->plbuf_top         = 0;
        pls->plbuf_readpos     = 0;
    }
    else
    {
        // Buffer is allocated, move the top to the beginning
        pls->plbuf_top = 0;
    }
}

//--------------------------------------------------------------------------
// plbuf_line()
//
// Draw a line in the current color from (x1,y1) to (x2,y2).
//--------------------------------------------------------------------------

void
plbuf_line( PLStream *pls, short x1a, short y1a, short x2a, short y2a )
{
    short xpl[2], ypl[2];

    dbug_enter( "plbuf_line" );

    wr_command( pls, (U_CHAR) LINE );

    xpl[0] = x1a;
    xpl[1] = x2a;
    ypl[0] = y1a;
    ypl[1] = y2a;

    wr_data( pls, xpl, sizeof ( short ) * 2 );
    wr_data( pls, ypl, sizeof ( short ) * 2 );
}

//--------------------------------------------------------------------------
// plbuf_polyline()
//
// Draw a polyline in the current color.
//--------------------------------------------------------------------------

void
plbuf_polyline( PLStream *pls, short *xa, short *ya, PLINT npts )
{
    dbug_enter( "plbuf_polyline" );

    wr_command( pls, (U_CHAR) POLYLINE );

    wr_data( pls, &npts, sizeof ( PLINT ) );

    wr_data( pls, xa, sizeof ( short ) * (size_t) npts );
    wr_data( pls, ya, sizeof ( short ) * (size_t) npts );
}

//--------------------------------------------------------------------------
// plbuf_eop()
//
// End of page.
//--------------------------------------------------------------------------

void
plbuf_eop( PLStream * PL_UNUSED( pls ) )
{
    dbug_enter( "plbuf_eop" );
}

//--------------------------------------------------------------------------
// plbuf_bop()
//
// Set up for the next page.
// To avoid problems redisplaying partially filled pages, on each BOP the
// old data in the buffer is ignored by setting the top back to the
// beginning of the buffer.
//
// Also write state information to ensure the next page is correct.
//--------------------------------------------------------------------------

void
plbuf_bop( PLStream *pls )
{
    dbug_enter( "plbuf_bop" );

    plbuf_tidy( pls );

    // Move the top to the beginning
    pls->plbuf_top = 0;

    wr_command( pls, (U_CHAR) BOP );

    // Save the current configuration (e.g. colormap) to allow plRemakePlot
    // to correctly regenerate the plot
    plbuf_state(pls, PLSTATE_CMAP0);
    plbuf_state(pls, PLSTATE_CMAP1);

    // Initialize to a known state
    plbuf_state( pls, PLSTATE_COLOR0 );
    plbuf_state( pls, PLSTATE_WIDTH );
}

//--------------------------------------------------------------------------
// plbuf_tidy()
//
// Close graphics file
//--------------------------------------------------------------------------

void
plbuf_tidy( PLStream * PL_UNUSED( pls ) )
{
    dbug_enter( "plbuf_tidy" );
}

//--------------------------------------------------------------------------
// plbuf_state()
//
// Handle change in PLStream state (color, pen width, fill attribute, etc).
//--------------------------------------------------------------------------

void
plbuf_state( PLStream *pls, PLINT op )
{
    dbug_enter( "plbuf_state" );

    wr_command( pls, (U_CHAR) CHANGE_STATE );
    wr_command( pls, (U_CHAR) op );

    switch ( op )
    {
    case PLSTATE_WIDTH:
        wr_data( pls, &( pls->width ), sizeof ( pls->width ) );
        break;

    case PLSTATE_COLOR0:
        wr_data( pls, &( pls->icol0 ), sizeof ( pls->icol0 ) );
        if ( pls->icol0 == PL_RGB_COLOR )
        {
            wr_data( pls, &( pls->curcolor.r ), sizeof ( pls->curcolor.r ) );
            wr_data( pls, &( pls->curcolor.g ), sizeof ( pls->curcolor.g ) );
            wr_data( pls, &( pls->curcolor.b ), sizeof ( pls->curcolor.b ) );
        }
        break;

    case PLSTATE_COLOR1:
        wr_data( pls, &( pls->icol1 ), sizeof ( pls->icol1 ) );
        break;

    case PLSTATE_FILL:
        wr_data( pls, &( pls->patt ), sizeof ( pls->patt ) );
        break;

    case PLSTATE_CMAP0:
        // Save the number of colors in the palatte
        wr_data( pls, & ( pls->ncol0 ), sizeof ( pls->ncol0 ) );
        // Save the color palatte
        wr_data( pls, &( pls->cmap0[0] ), sizeof ( PLColor ) * pls->ncol0 );
        break;

    case PLSTATE_CMAP1:
        // Save the number of colors in the palatte
        wr_data( pls, & ( pls->ncol1 ), sizeof ( pls->ncol1 ) );
	// Save the color palatte
        wr_data( pls, &( pls->cmap1[0] ), sizeof ( PLColor ) * pls->ncol1 );
        break;

    case PLSTATE_CHR:
        //save the chrdef and chrht parameters
        wr_data( pls, & ( pls->chrdef ), sizeof ( pls->chrdef ) );
	wr_data( pls, & ( pls->chrht ), sizeof ( pls->chrht ) );
	break;

    case PLSTATE_SYM:
        //save the symdef and symht parameters
        wr_data( pls, & ( pls->symdef ), sizeof ( pls->symdef ) );
	wr_data( pls, & ( pls->symht ), sizeof ( pls->symht ) );
	break;
    }
}


//--------------------------------------------------------------------------
// plbuf_image()
//
// write image described in points pls->dev_x[], pls->dev_y[], pls->dev_z[].
//                      pls->nptsX, pls->nptsY.
//--------------------------------------------------------------------------

static void
plbuf_image( PLStream *pls, IMG_DT *img_dt )
{
    PLINT npts = pls->dev_nptsX * pls->dev_nptsY;

    dbug_enter( "plbuf_image" );

    wr_data( pls, &pls->dev_nptsX, sizeof ( PLINT ) );
    wr_data( pls, &pls->dev_nptsY, sizeof ( PLINT ) );

    wr_data( pls, &img_dt->xmin, sizeof ( PLFLT ) );
    wr_data( pls, &img_dt->ymin, sizeof ( PLFLT ) );
    wr_data( pls, &img_dt->dx, sizeof ( PLFLT ) );
    wr_data( pls, &img_dt->dy, sizeof ( PLFLT ) );

    wr_data( pls, &pls->dev_zmin, sizeof ( short ) );
    wr_data( pls, &pls->dev_zmax, sizeof ( short ) );

    wr_data( pls, pls->dev_ix, sizeof ( short ) * (size_t) npts );
    wr_data( pls, pls->dev_iy, sizeof ( short ) * (size_t) npts );
    wr_data( pls, pls->dev_z,
	     sizeof ( unsigned short )
	     * (size_t) ( ( pls->dev_nptsX - 1 ) * ( pls->dev_nptsY - 1 ) ) );
}

//--------------------------------------------------------------------------
// plbuf_text()
//
// Handle text call.
//--------------------------------------------------------------------------

static void
plbuf_text( PLStream *pls, EscText *text )
{
    PLUNICODE fci;

    dbug_enter( "plbuf_text" );

    // Retrieve the font characterization integer
    plgfci( &fci );

    // Write the text information

    wr_data( pls, &fci, sizeof ( PLUNICODE ) );

    wr_data( pls, &pls->chrht, sizeof ( PLFLT ) );
    wr_data( pls, &pls->diorot, sizeof ( PLFLT ) );
    wr_data( pls, &pls->clpxmi, sizeof ( PLFLT ) );
    wr_data( pls, &pls->clpxma, sizeof ( PLFLT ) );
    wr_data( pls, &pls->clpymi, sizeof ( PLFLT ) );
    wr_data( pls, &pls->clpyma, sizeof ( PLFLT ) );

    wr_data( pls, &text->base, sizeof ( PLINT ) );
    wr_data( pls, &text->just, sizeof ( PLFLT ) );
    wr_data( pls, text->xform, sizeof ( PLFLT ) * 4 );
    wr_data( pls, &text->x, sizeof ( PLINT ) );
    wr_data( pls, &text->y, sizeof ( PLINT ) );
    wr_data( pls, &text->refx, sizeof ( PLINT ) );
    wr_data( pls, &text->refy, sizeof ( PLINT ) );

    wr_data( pls, &text->unicode_array_len, sizeof ( PLINT ) );
    if ( text->unicode_array_len )
        wr_data( pls,
                 text->unicode_array,
                 sizeof ( PLUNICODE ) * text->unicode_array_len );
}

//--------------------------------------------------------------------------
// plbuf_text_unicode()
//
// Handle text buffering for the new unicode pathway.
//--------------------------------------------------------------------------

static void
plbuf_text_unicode( PLStream *pls, EscText *text )
{
    PLUNICODE fci;

    dbug_enter( "plbuf_text" );

    // Retrieve the font characterization integer
    plgfci( &fci );

    // Write the text information

    wr_data( pls, &fci, sizeof ( PLUNICODE ) );

    wr_data( pls, &pls->chrht, sizeof ( PLFLT ) );
    wr_data( pls, &pls->diorot, sizeof ( PLFLT ) );
    wr_data( pls, &pls->clpxmi, sizeof ( PLFLT ) );
    wr_data( pls, &pls->clpxma, sizeof ( PLFLT ) );
    wr_data( pls, &pls->clpymi, sizeof ( PLFLT ) );
    wr_data( pls, &pls->clpyma, sizeof ( PLFLT ) );

    wr_data( pls, &text->base, sizeof ( PLINT ) );
    wr_data( pls, &text->just, sizeof ( PLFLT ) );
    wr_data( pls, text->xform, sizeof ( PLFLT ) * 4 );
    wr_data( pls, &text->x, sizeof ( PLINT ) );
    wr_data( pls, &text->y, sizeof ( PLINT ) );
    wr_data( pls, &text->refx, sizeof ( PLINT ) );
    wr_data( pls, &text->refy, sizeof ( PLINT ) );

    wr_data( pls, &text->n_fci, sizeof ( PLUNICODE ) );
    wr_data( pls, &text->n_char, sizeof ( PLUNICODE ) );
    wr_data( pls, &text->n_ctrl_char, sizeof ( PLINT ) );

    wr_data( pls, &text->unicode_array_len, sizeof ( PLINT ) );
}


//--------------------------------------------------------------------------
// plbuf_esc()
//
// Escape function.  Note that any data written must be in device
// independent form to maintain the transportability of the metafile.
//
// Functions:
//
//	PLESC_FILL	    Fill polygon
//	PLESC_SWIN	    Set plot window parameters
//      PLESC_IMAGE         Draw image
//      PLESC_HAS_TEXT      Draw PostScript text
//	PLESC_CLEAR	    Clear Background
//	PLESC_START_RASTERIZE
//	PLESC_END_RASTERIZE Start and stop rasterization
//--------------------------------------------------------------------------

void
plbuf_esc( PLStream *pls, PLINT op, void *ptr )
{
    dbug_enter( "plbuf_esc" );

    wr_command( pls, (U_CHAR) ESCAPE );
    wr_command( pls, (U_CHAR) op );

    switch ( op )
    {
    case PLESC_FILL:
        plbuf_fill( pls );
        break;
    case PLESC_SWIN:
        plbuf_swin( pls, (PLWindow *) ptr );
        break;
    case PLESC_IMAGE:
        plbuf_image( pls, (IMG_DT *) ptr );
        break;
    case PLESC_HAS_TEXT:
        if ( ptr != NULL ) // Check required by GCW driver, please don't remove
            plbuf_text( pls, (EscText *) ptr );
        break;
    case PLESC_BEGIN_TEXT:
    case PLESC_TEXT_CHAR:
    case PLESC_CONTROL_CHAR:
    case PLESC_END_TEXT:
        plbuf_text_unicode( pls, (EscText *) ptr );
        break;
#if 0
    // These are a no-op.  They just need an entry in the buffer.
    case PLESC_CLEAR:
    case PLESC_START_RASTERIZE:
    case PLESC_END_RASTERIZE:
        break;
#endif
    }
}

//--------------------------------------------------------------------------
// plbuf_fill()
//
// Fill polygon described in points pls->dev_x[] and pls->dev_y[].
//--------------------------------------------------------------------------

static void
plbuf_fill( PLStream *pls )
{
    dbug_enter( "plbuf_fill" );

    wr_data( pls, &pls->dev_npts, sizeof ( PLINT ) );
    wr_data( pls, pls->dev_x, sizeof ( short ) * (size_t) pls->dev_npts );
    wr_data( pls, pls->dev_y, sizeof ( short ) * (size_t) pls->dev_npts );
}

//--------------------------------------------------------------------------
// plbuf_swin()
//
// Set up plot window parameters.
//--------------------------------------------------------------------------

static void
plbuf_swin( PLStream *pls, PLWindow *plwin )
{
    dbug_enter( "plbuf_swin" );

    wr_data( pls, &plwin->dxmi, sizeof ( plwin->dxmi ) );
    wr_data( pls, &plwin->dxma, sizeof ( plwin->dxma ) );
    wr_data( pls, &plwin->dymi, sizeof ( plwin->dymi ) );
    wr_data( pls, &plwin->dyma, sizeof ( plwin->dyma ) );

    wr_data( pls, &plwin->wxmi, sizeof ( plwin->wxmi ) );
    wr_data( pls, &plwin->wxma, sizeof ( plwin->wxma ) );
    wr_data( pls, &plwin->wymi, sizeof ( plwin->wymi ) );
    wr_data( pls, &plwin->wyma, sizeof ( plwin->wyma ) );
}

//--------------------------------------------------------------------------
// Routines to read from & process the plot buffer.
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// rdbuf_init()
//
// Initialize device.
//--------------------------------------------------------------------------

static void
rdbuf_init( PLStream * PL_UNUSED( pls ) )
{
    dbug_enter( "rdbuf_init" );
}

//--------------------------------------------------------------------------
// rdbuf_line()
//
// Draw a line in the current color from (x1,y1) to (x2,y2).
//--------------------------------------------------------------------------

static void
rdbuf_line( PLStream *pls )
{
    short *xpl, *ypl;
    PLINT npts = 2;

    dbug_enter( "rdbuf_line" );

    // Use the "no copy" version because the endpoint data array does
    // not need to persist outside of this function
    rd_data_no_copy( pls, (void **)&xpl, sizeof ( short ) * (size_t) npts );
    rd_data_no_copy( pls, (void **)&ypl, sizeof ( short ) * (size_t) npts );

    plP_line( xpl, ypl );
}

//--------------------------------------------------------------------------
// rdbuf_polyline()
//
// Draw a polyline in the current color.
//--------------------------------------------------------------------------

static void
rdbuf_polyline( PLStream *pls )
{
    short *xpl, *ypl;
    PLINT npts;

    dbug_enter( "rdbuf_polyline" );

    rd_data( pls, &npts, sizeof ( PLINT ) );

    // Use the "no copy" version because the node data array does
    // not need to persist outside of ths function
    rd_data_no_copy( pls, (void **) &xpl, sizeof ( short ) * (size_t) npts);
    rd_data_no_copy( pls, (void **) &ypl, sizeof ( short ) * (size_t) npts);

    plP_polyline( xpl, ypl, npts );
}

//--------------------------------------------------------------------------
// rdbuf_eop()
//
// End of page.
//--------------------------------------------------------------------------

static void
rdbuf_eop( PLStream * PL_UNUSED( pls ) )
{
    dbug_enter( "rdbuf_eop" );
}

//--------------------------------------------------------------------------
// rdbuf_bop()
//
// Set up for the next page.
//--------------------------------------------------------------------------

static void
rdbuf_bop( PLStream *pls )
{
    dbug_enter( "rdbuf_bop" );

    pls->nplwin = 0;
}

//--------------------------------------------------------------------------
// rdbuf_state()
//
// Handle change in PLStream state (color, pen width, fill attribute, etc).
//--------------------------------------------------------------------------

static void
rdbuf_state( PLStream *pls )
{
    U_CHAR op;

    dbug_enter( "rdbuf_state" );

    rd_data( pls, &op, sizeof ( U_CHAR ) );

    switch ( op )
    {
    case PLSTATE_WIDTH: {
        rd_data( pls, &( pls->width ), sizeof ( pls->width ) );
        plP_state( PLSTATE_WIDTH );

        break;
    }

    case PLSTATE_COLOR0: {
        U_CHAR r, g, b;
        PLFLT  a;

        rd_data( pls, &(pls->icol0), sizeof ( pls->icol0 ) );
        if ( pls->icol0 == PL_RGB_COLOR )
        {
            rd_data( pls, &r, sizeof ( U_CHAR ) );
            rd_data( pls, &g, sizeof ( U_CHAR ) );
            rd_data( pls, &b, sizeof ( U_CHAR ) );
            a = 1.0;
        }
        else
        {
            if ( pls->icol0 >= pls->ncol0 )
            {
                char buffer[256];
                snprintf( buffer, 256,
                          "rdbuf_state: Invalid color map entry: %d",
                          pls->icol0 );
                plabort( buffer );
                return;
            }
            r = pls->cmap0[pls->icol0].r;
            g = pls->cmap0[pls->icol0].g;
            b = pls->cmap0[pls->icol0].b;
            a = pls->cmap0[pls->icol0].a;
        }
        pls->curcolor.r = r;
        pls->curcolor.g = g;
        pls->curcolor.b = b;
        pls->curcolor.a = a;
        pls->curcmap = 0;

        plP_state( PLSTATE_COLOR0 );
        break;
    }

    case PLSTATE_COLOR1: {
        rd_data( pls, &(pls->icol1), sizeof ( pls->icol1 ) );

        pls->curcolor.r = pls->cmap1[pls->icol1].r;
        pls->curcolor.g = pls->cmap1[pls->icol1].g;
        pls->curcolor.b = pls->cmap1[pls->icol1].b;
        pls->curcolor.a = pls->cmap1[pls->icol1].a;
        pls->curcmap = 1;

        plP_state( PLSTATE_COLOR1 );
        break;
    }

    case PLSTATE_FILL: {
        rd_data( pls, &(pls->patt), sizeof ( pls->patt ) );

        plP_state( PLSTATE_FILL );
        break;
    }

    case PLSTATE_CMAP0: {
        PLINT ncol;
        size_t size;

        rd_data( pls, &ncol, sizeof ( ncol ) );

	// Calculate the memory size for this color palatte
        size = (size_t) ncol * sizeof ( PLColor );

        if ( pls->ncol0 == 0 || pls->ncol0 != ncol )
        {
            // The current palatte is empty or the current palatte is not
            // big enough, thus we need allocate a new one

            // If we have a colormap, discard it because we do not use
            // realloc().  We are going to read the colormap from the buffer
            if(pls->cmap0 != NULL)
                free(pls->cmap0);

            if ( ( pls->cmap0 = ( PLColor * )malloc( size ) ) == NULL )
            {
                plexit( "Insufficient memory for colormap 0" );
            }
        }

        // Now read the colormap from the buffer
        rd_data( pls, &( pls->cmap0[0] ), size );
	pls->ncol0 = ncol;

        plP_state( PLSTATE_CMAP0 );
        break;
    }

    case PLSTATE_CMAP1: {
        PLINT ncol;
        size_t size;

        rd_data( pls, &ncol, sizeof ( ncol ) );

        // Calculate the memory size for this color palatte
        size = (size_t) ncol * sizeof ( PLColor );

        if ( pls->ncol1 == 0 || pls->ncol1 != ncol )
        {
            // The current palatte is empty or the current palatte is not
            // big enough, thus we need allocate a new one

            // If we have a colormap, discard it because we do not use
            // realloc().  We are going to read the colormap from the buffer
            if(pls->cmap1 != NULL)
                free(pls->cmap1);

            if ( ( pls->cmap1 = ( PLColor * )malloc( size ) ) == NULL )
            {
                plexit( "Insufficient memory for colormap 1" );
            }
        }

        // Now read the colormap from the buffer
        rd_data( pls, &(pls->cmap1[0]), size );
        pls->ncol1 = ncol;

        plP_state( PLSTATE_CMAP1 );
        break;
    }

    case PLSTATE_CHR: {
        //read the chrdef and chrht parameters
        rd_data( pls, & ( pls->chrdef ), sizeof ( pls->chrdef ) );
        rd_data( pls, & ( pls->chrht ), sizeof ( pls->chrht ) );
        break;
    }

    case PLSTATE_SYM: {
        //read the symdef and symht parameters
        rd_data( pls, & ( pls->symdef ), sizeof ( pls->symdef ) );
        rd_data( pls, & ( pls->symht ), sizeof ( pls->symht ) );
        break;
    }

    }
}

//--------------------------------------------------------------------------
// rdbuf_esc()
//
// Escape function.
// Must fill data structure with whatever data that was written,
// then call escape function.
//
// Note: it is best to only call the escape function for op-codes that
// are known to be supported.
//
// Functions:
//
//	PLESC_FILL	    Fill polygon
//	PLESC_SWIN	    Set plot window parameters
//      PLESC_IMAGE         Draw image
//      PLESC_HAS_TEXT      Draw PostScript text
//      PLESC_BEGIN_TEXT    Commands for the alternative unicode text
//      PLESC_TEXT_CHAR     handling path
//      PLESC_CONTROL_CHAR
//      PLESC_END_TEXT
//	PLESC_CLEAR	    Clear Background
//--------------------------------------------------------------------------

static void
rdbuf_image( PLStream *pls );

static void
rdbuf_text( PLStream *pls );

static void
rdbuf_text_unicode( PLINT op, PLStream *pls );

static void
rdbuf_esc( PLStream *pls )
{
    U_CHAR op;

    dbug_enter( "rdbuf_esc" );

    rd_data( pls, &op, sizeof ( U_CHAR ) );

    switch ( op )
    {
    case PLESC_FILL:
        rdbuf_fill( pls );
        break;
    case PLESC_SWIN:
        rdbuf_swin( pls );
        break;
    case PLESC_IMAGE:
        rdbuf_image( pls );
        break;
    case PLESC_HAS_TEXT:
        rdbuf_text( pls );
        break;
    case PLESC_BEGIN_TEXT:
    case PLESC_TEXT_CHAR:
    case PLESC_CONTROL_CHAR:
    case PLESC_END_TEXT:
        rdbuf_text_unicode( op, pls );
        break;
    case PLESC_CLEAR:
        plP_esc( PLESC_CLEAR, NULL );
        break;
    case PLESC_START_RASTERIZE:
        plP_esc( PLESC_START_RASTERIZE, NULL );
        break;
    case PLESC_END_RASTERIZE:
        plP_esc( PLESC_END_RASTERIZE, NULL );
        break;
    }
}

//--------------------------------------------------------------------------
// rdbuf_fill()
//
// Fill polygon described by input points.
//--------------------------------------------------------------------------

static void
rdbuf_fill( PLStream *pls )
{
    short *xpl, *ypl;
    PLINT npts;

    dbug_enter( "rdbuf_fill" );

    rd_data( pls, &npts, sizeof ( PLINT ) );

    rd_data_no_copy( pls, (void **) &xpl, sizeof ( short ) * (size_t) npts );
    rd_data_no_copy( pls, (void **) &ypl, sizeof ( short ) * (size_t) npts );

    plP_fill( xpl, ypl, npts );
}

//--------------------------------------------------------------------------
// rdbuf_image()
//
// .
//--------------------------------------------------------------------------

static void
rdbuf_image( PLStream *pls )
{
    // Unnecessarily initialize dev_iy and dev_z to quiet -O1
    // -Wuninitialized warnings which are false alarms.  (If something
    // goes wrong with the dev_ix malloc below any further use of
    // dev_iy and dev_z does not occur.  Similarly, if something goes
    // wrong with the dev_iy malloc below any further use of dev_z
    // does not occur.)
    short          *dev_ix, *dev_iy = NULL;
    unsigned short *dev_z = NULL, dev_zmin, dev_zmax;
    PLINT          nptsX, nptsY, npts;
    PLFLT          xmin, ymin, dx, dy;

    dbug_enter( "rdbuf_image" );

    rd_data( pls, &nptsX, sizeof ( PLINT ) );
    rd_data( pls, &nptsY, sizeof ( PLINT ) );
    npts = nptsX * nptsY;

    rd_data( pls, &xmin, sizeof ( PLFLT ) );
    rd_data( pls, &ymin, sizeof ( PLFLT ) );
    rd_data( pls, &dx, sizeof ( PLFLT ) );
    rd_data( pls, &dy, sizeof ( PLFLT ) );

    rd_data( pls, &dev_zmin, sizeof ( short ) );
    rd_data( pls, &dev_zmax, sizeof ( short ) );

    // NOTE:  Even though for memory buffered version all the data is in memory,
    // we still allocate and copy the data because I think that method works
    // better in a multithreaded environment.  I could be wrong.
    //
    if ( ( ( dev_ix = (short *) malloc( (size_t) npts * sizeof ( short ) ) ) == NULL ) ||
         ( ( dev_iy = (short *) malloc( (size_t) npts * sizeof ( short ) ) ) == NULL ) ||
         ( ( dev_z = (unsigned short *) malloc( (size_t) ( ( nptsX - 1 ) * ( nptsY - 1 ) ) * sizeof ( unsigned short ) ) ) == NULL ) )
        plexit( "rdbuf_image: Insufficient memory" );

    rd_data( pls, dev_ix, sizeof ( short ) * (size_t) npts );
    rd_data( pls, dev_iy, sizeof ( short ) * (size_t) npts );
    rd_data( pls, dev_z,
             sizeof ( unsigned short )
             * (size_t) ( ( nptsX - 1 ) * ( nptsY - 1 ) ) );

    //
    // COMMENTED OUT by Hezekiah Carty
    // Commented (hopefullly temporarily) until the dev_fastimg rendering
    // path can be updated to support the new plimage internals. In the
    // meantime this function is not actually used so the issue of how to
    // update the code to support the new interface can be ignored.
    //
    //plP_image(dev_ix, dev_iy, dev_z, nptsX, nptsY, xmin, ymin, dx, dy, dev_zmin, dev_zmax);

    free( dev_ix );
    free( dev_iy );
    free( dev_z );
}

//--------------------------------------------------------------------------
// rdbuf_swin()
//
// Set up plot window parameters.
//--------------------------------------------------------------------------

static void
rdbuf_swin( PLStream *pls )
{
    PLWindow plwin;

    dbug_enter( "rdbuf_swin" );

    rd_data( pls, &plwin.dxmi, sizeof ( plwin.dxmi ) );
    rd_data( pls, &plwin.dxma, sizeof ( plwin.dxma ) );
    rd_data( pls, &plwin.dymi, sizeof ( plwin.dymi ) );
    rd_data( pls, &plwin.dyma, sizeof ( plwin.dyma ) );

    rd_data( pls, &plwin.wxmi, sizeof ( plwin.wxmi ) );
    rd_data( pls, &plwin.wxma, sizeof ( plwin.wxma ) );
    rd_data( pls, &plwin.wymi, sizeof ( plwin.wymi ) );
    rd_data( pls, &plwin.wyma, sizeof ( plwin.wyma ) );

    plP_swin( &plwin );
}

//--------------------------------------------------------------------------
// rdbuf_text()
//
// Draw PostScript text.
//--------------------------------------------------------------------------

static void
rdbuf_text( PLStream *pls )
{
    PLUNICODE( fci );
    EscText  text;
    PLFLT    xform[4];
    PLUNICODE* unicode;

    dbug_enter( "rdbuf_text" );

    text.xform = xform;

    // Read in the data
    rd_data( pls, &fci, sizeof ( PLUNICODE ) );

    rd_data( pls, &pls->chrht, sizeof ( PLFLT ) );
    rd_data( pls, &pls->diorot, sizeof ( PLFLT ) );
    rd_data( pls, &pls->clpxmi, sizeof ( PLFLT ) );
    rd_data( pls, &pls->clpxma, sizeof ( PLFLT ) );
    rd_data( pls, &pls->clpymi, sizeof ( PLFLT ) );
    rd_data( pls, &pls->clpyma, sizeof ( PLFLT ) );

    rd_data( pls, &text.base, sizeof ( PLINT ) );
    rd_data( pls, &text.just, sizeof ( PLFLT ) );
    rd_data( pls, text.xform, sizeof ( PLFLT ) * 4 );
    rd_data( pls, &text.x, sizeof ( PLINT ) );
    rd_data( pls, &text.y, sizeof ( PLINT ) );
    rd_data( pls, &text.refx, sizeof ( PLINT ) );
    rd_data( pls, &text.refy, sizeof ( PLINT ) );

    rd_data( pls, &text.unicode_array_len, sizeof ( PLINT ) );

    // Initialize to NULL so we don't accidentally try to free the memory
    text.unicode_array = NULL;
    if ( text.unicode_array_len )
    {
        if ( ( unicode = (PLUNICODE *) malloc( text.unicode_array_len * sizeof ( PLUNICODE ) ) )
             == NULL )
            plexit( "rdbuf_text: Insufficient memory" );

        rd_data( pls, unicode, sizeof ( PLUNICODE ) * text.unicode_array_len );
        text.unicode_array = unicode;
    }
    else
        text.unicode_array = NULL;

    // Make the call for unicode devices
    if ( pls->dev_unicode )
    {
        plsfci( fci );
        plP_esc( PLESC_HAS_TEXT, &text );
    }
}

//--------------------------------------------------------------------------
// rdbuf_text_unicode()
//
// Draw text for the new unicode handling pathway.
//--------------------------------------------------------------------------

static void
rdbuf_text_unicode( PLINT op, PLStream *pls )
{
    PLUNICODE( fci );
    EscText text;
    PLFLT   xform[4];

    dbug_enter( "rdbuf_text_unicode" );

    text.xform = xform;

    // Read in the data

    rd_data( pls, &fci, sizeof ( PLUNICODE ) );

    rd_data( pls, &pls->chrht, sizeof ( PLFLT ) );
    rd_data( pls, &pls->diorot, sizeof ( PLFLT ) );
    rd_data( pls, &pls->clpxmi, sizeof ( PLFLT ) );
    rd_data( pls, &pls->clpxma, sizeof ( PLFLT ) );
    rd_data( pls, &pls->clpymi, sizeof ( PLFLT ) );
    rd_data( pls, &pls->clpyma, sizeof ( PLFLT ) );

    rd_data( pls, &text.base, sizeof ( PLINT ) );
    rd_data( pls, &text.just, sizeof ( PLFLT ) );
    rd_data( pls, text.xform, sizeof ( PLFLT ) * 4 );
    rd_data( pls, &text.x, sizeof ( PLINT ) );
    rd_data( pls, &text.y, sizeof ( PLINT ) );
    rd_data( pls, &text.refx, sizeof ( PLINT ) );
    rd_data( pls, &text.refy, sizeof ( PLINT ) );

    rd_data( pls, &text.n_fci, sizeof ( PLUNICODE ) );
    rd_data( pls, &text.n_char, sizeof ( PLUNICODE ) );
    rd_data( pls, &text.n_ctrl_char, sizeof ( PLINT ) );

    rd_data( pls, &text.unicode_array_len, sizeof ( PLINT ) );

    if ( pls->dev_unicode )
    {
        plsfci( fci );
        plP_esc( op, &text );
    }
}

//--------------------------------------------------------------------------
// plRemakePlot()
//
// Rebuilds plot from plot buffer, usually in response to a window
// resize or exposure event.
//--------------------------------------------------------------------------

void
plRemakePlot( PLStream *pls )
{
    U_CHAR   c;
    int      plbuf_status;

    dbug_enter( "plRemakePlot" );

    // Change the status of the flags before checking for a buffer.
    // Actually, more thought is needed if we want to support multithreaded
    // code correctly, specifically the case where two threads are using
    // the same plot stream (e.g. one thread is drawing the plot and another
    // thread is processing window manager messages).
    //
    plbuf_status     = pls->plbuf_write;
    pls->plbuf_write = FALSE;
    pls->plbuf_read  = TRUE;

    if ( pls->plbuf_buffer )
    {
        // State saving variables
        PLStream *save_current_pls;

        pls->plbuf_readpos = 0;

        // Save state

        // Need to change where plsc (current plot stream) points to before
        // processing the commands.  If we have multiple plot streams, this
        // will prevent the commands from going to the wrong plot stream.
        //
        save_current_pls = plsc;

        // Make the current plot stream the one passed by the caller
        plsc     = pls;

        // Replay the plot command buffer
        while ( rd_command( pls, &c ) )
        {
            plbuf_control( pls, c );
        }

        // Restore the original current plot stream
        plsc = save_current_pls;
    }

    // Restore the state of the passed plot stream
    pls->plbuf_read  = FALSE;
    pls->plbuf_write = plbuf_status;
}

//--------------------------------------------------------------------------
// plbuf_control()
//
// Processes commands read from the plot buffer.
//--------------------------------------------------------------------------

static void
plbuf_control( PLStream *pls, U_CHAR c )
{
    static U_CHAR c_old = 0;

    dbug_enter( "plbuf_control" );

    //#define CLOSE              2
    //#define LINETO             10
    //#define END_OF_FIELD       255

    switch ( (int) c )
    {
    case INITIALIZE:
        rdbuf_init( pls );
        break;

    case EOP:
        rdbuf_eop( pls );
        break;

    case BOP0:
    case BOP:
        rdbuf_bop( pls );
        break;

    case CHANGE_STATE:
        rdbuf_state( pls );
        break;

    case LINE:
        rdbuf_line( pls );
        break;

    case POLYLINE:
        rdbuf_polyline( pls );
        break;

    case ESCAPE:
        rdbuf_esc( pls );
        break;

    // Obsolete commands, left here to maintain compatibility with previous
    // version of plot metafiles
    case SWITCH_TO_TEXT:    // Obsolete, replaced by ESCAPE
    case SWITCH_TO_GRAPH:   // Obsolete, replaced by ESCAPE
    case NEW_COLOR:         // Obsolete, replaced by CHANGE_STATE
    case NEW_COLOR1:
    case NEW_WIDTH:         // Obsolete, replaced by CHANGE_STATE
    case ADVANCE:           // Obsolete, BOP/EOP used instead
        pldebug( "plbuf_control", "Obsolete command %d, ignoring\n", c );
        break;

    default:
        pldebug( "plbuf_control", "Unrecognized command %d, previous %d\n",
                 c, c_old );
        plexit("Unrecognized command");
    }
    c_old = c;
}

//--------------------------------------------------------------------------
// rd_command()
//
// Read & return the next command
//--------------------------------------------------------------------------

static int
rd_command( PLStream *pls, U_CHAR *p_c )
{
    int count;

    if ( pls->plbuf_readpos < pls->plbuf_top )
    {
        *p_c = *(U_CHAR *) ((U_CHAR *) pls->plbuf_buffer + pls->plbuf_readpos);

        // Advance the buffer position to maintain two-byte alignment
        pls->plbuf_readpos += sizeof ( short );

        count = sizeof ( U_CHAR );
    }
    else
    {
        count = 0;
    }

    return ( count );
}

//--------------------------------------------------------------------------
// rd_data()
//
// Read the data associated with the command
//--------------------------------------------------------------------------

static void
rd_data( PLStream *pls, void *buf, size_t buf_size )
{
    // If U_CHAR is not the same size as what memcpy() expects (typically
    // 1 byte) then this code will have problems.  A better approach might be
    // to use uint8_t from <stdint.h> but I do not know how portable that
    // approach is
    //
    memcpy( buf, (U_CHAR *) pls->plbuf_buffer + pls->plbuf_readpos, buf_size );

    // Advance position but maintain alignment
    pls->plbuf_readpos += (buf_size + (buf_size % sizeof(short)));
}

//--------------------------------------------------------------------------
// rd_data_no_copy()
//
// Read the data associated with the command by setting a pointer to the
// position in the plot buffer.  This avoids having to allocate space
// and doing a memcpy.  Useful for commands that do not need the data
// to persist (like LINE and POLYLINE).  Do not use for commands that
// has data that needs to persist or are freed elsewhere (like COLORMAPS).
//--------------------------------------------------------------------------

static void
rd_data_no_copy( PLStream *pls, void **buf, size_t buf_size)
{
    (*buf) = (U_CHAR *) pls->plbuf_buffer + pls->plbuf_readpos;

    // Advance position but maintain alignment
    pls->plbuf_readpos += (buf_size + (buf_size % sizeof(short)));
}

//--------------------------------------------------------------------------
// check_buffer_size()
//
// Checks that the buffer has space to store the desired amount of data.
// If not, the buffer is resized to accomodate the request
//--------------------------------------------------------------------------
static void
check_buffer_size( PLStream *pls, size_t data_size )
{
    size_t required_size;

    required_size = pls->plbuf_top + data_size;

    if ( required_size >= pls->plbuf_buffer_size )
    {
        // Not enough space, need to grow the buffer before memcpy
        // Must make sure the increase is enough for this data, so
        // Determine the amount of space required and grow in multiples
        // of plbuf_buffer_grow
        pls->plbuf_buffer_size += pls->plbuf_buffer_grow *
                                  ( ( required_size
                                      - pls->plbuf_buffer_size )
                                    / pls->plbuf_buffer_grow
                                    + 1 );

        if ( pls->verbose )
            printf( "Growing buffer to %d KB\n",
                    (int) ( pls->plbuf_buffer_size / 1024 ) );

        if ( ( pls->plbuf_buffer
               = realloc( pls->plbuf_buffer, pls->plbuf_buffer_size )
               ) == NULL )
            plexit( "plbuf buffer grow:  Plot buffer grow failed" );
    }
}

//--------------------------------------------------------------------------
// wr_command()
//
// Write the next command
//--------------------------------------------------------------------------

static void
wr_command( PLStream *pls, U_CHAR c )
{
    check_buffer_size(pls, sizeof( U_CHAR ));

    *(U_CHAR *) ( (U_CHAR *) pls->plbuf_buffer + pls->plbuf_top ) = c;

    // Advance buffer position to maintain two-byte alignment.  This
    // will waste a little bit of space, but it prevents memory
    // alignment problems
    pls->plbuf_top += sizeof ( short );
}

//--------------------------------------------------------------------------
// wr_data()
//
// Write the data associated with a command
//--------------------------------------------------------------------------

static void
wr_data( PLStream *pls, void *buf, size_t buf_size )
{
    check_buffer_size(pls, buf_size);

    // If U_CHAR is not the same size as what memcpy() expects (typically 1
    // byte) then this code will have problems.  A better approach might be
    // to use uint8_t from <stdint.h> but I do not know how portable that
    // approach is
    //
    memcpy( (U_CHAR *) pls->plbuf_buffer + pls->plbuf_top, buf, buf_size );

    // Advance position but maintain alignment
    pls->plbuf_top += (buf_size + (buf_size % sizeof(short)));
}


//--------------------------------------------------------------------------
// Plot buffer state saving
//--------------------------------------------------------------------------

// plbuf_save(state)
//
// Saves the current state of the plot into a save buffer.
// This code was originally in gcw.c and gcw-lib.c.  The original
// code used a temporary file for the plot buffer and memory
// to perserve colormaps.  That method does not offer a clean
// break between using memory buffers and file buffers.  This
// function preserves the same functionality by returning a data
// structure that saves the plot buffer and colormaps seperately.
//
// The caller passes an existing save buffer for reuse or NULL
// to force the allocation of a new buffer.  Since one malloc()
// is used for everything, the entire save buffer can be freed
// with one free() call.
//
//
struct _state
{
    size_t            size;  // Size of the save buffer
    int               valid; // Flag to indicate a valid save state
    void              *plbuf_buffer;
    size_t            plbuf_buffer_size;
    size_t            plbuf_top;
    size_t            plbuf_readpos;
};

void * plbuf_save( PLStream *pls, void *state )
{
    size_t        save_size;
    struct _state *plot_state = (struct _state *) state;
    PLINT         i;
    U_CHAR        *buf; // Assume that this is byte-sized

    dbug_enter( "plbuf_save" );

    if ( pls->plbuf_write )
    {
        pls->plbuf_write = FALSE;
        pls->plbuf_read  = TRUE;

        // Determine the size of the buffer required to save everything.
        save_size = sizeof ( struct _state );

        // Only copy as much of the plot buffer that is being used
        save_size += pls->plbuf_top;

        // If a buffer exists, determine if we need to resize it
        if ( state != NULL )
        {
            // We have a save buffer, is it smaller than the current size
	    // requirement?
            if ( plot_state->size < save_size )
            {
                // Yes, reallocate a larger one
                if ( ( plot_state = (struct _state *) realloc( state, save_size ) ) == NULL )
                {
                    // NOTE: If realloc fails, then plot_state will be NULL.
                    // This will leave the original buffer untouched, thus we
                    // mark it as invalid and return it back to the caller.
                    //
                    plwarn( "plbuf: Unable to reallocate sufficient memory to save state" );
                    plot_state->valid = 0;

                    return state;
                }
                plot_state->size = save_size;
            }
        }
        else
        {
            // A buffer does not exist, so we need to allocate one
            if ( ( plot_state = (struct _state *) malloc( save_size ) ) == NULL )
            {
                plwarn( "plbuf: Unable to allocate sufficient memory to save state" );

                return NULL;
            }
            plot_state->size = save_size;
        }

        // At this point we have an appropriately sized save buffer.
        // We need to invalidate the state of the save buffer, since it
        // will not be valid until after everything is copied.  We use
        // this approach vice freeing the memory and returning a NULL pointer
        // in order to prevent allocating and freeing memory needlessly.
        //
        plot_state->valid = 0;

        // Point buf to the space after the struct _state
        buf = (U_CHAR *) ( plot_state + 1 );

        // Again, note, that we only copy the portion of the plot buffer that
	// is being used
        plot_state->plbuf_buffer_size = pls->plbuf_top;
        plot_state->plbuf_top         = pls->plbuf_top;
        plot_state->plbuf_readpos     = 0;

        // Create a pointer that points in the space we allocated after
        // struct _state
        plot_state->plbuf_buffer = (void *) buf;
        buf += pls->plbuf_top;

        // Copy the plot buffer to our new buffer.  Again, I must stress, that
        // we only are copying the portion of the plot buffer that is being used
        //
        if ( memcpy( plot_state->plbuf_buffer, pls->plbuf_buffer, pls->plbuf_top ) == NULL )
        {
            // This should never be NULL
            plwarn( "plbuf: Got a NULL in memcpy!" );
            return (void *) plot_state;
        }

        pls->plbuf_write = TRUE;
        pls->plbuf_read  = FALSE;

        plot_state->valid = 1;
        return (void *) plot_state;
    }

    return NULL;
}

// plbuf_restore(PLStream *, state)
//
// Restores the passed state
//
void plbuf_restore( PLStream *pls, void *state )
{
    struct _state *new_state = (struct _state *) state;

    dbug_enter( "plbuf_restore" );

    pls->plbuf_buffer      = new_state->plbuf_buffer;
    pls->plbuf_buffer_size = new_state->plbuf_buffer_size;
    pls->plbuf_top         = new_state->plbuf_top;
    pls->plbuf_readpos     = new_state->plbuf_readpos;
}

// plbuf_switch(PLStream *, state)
//
// Makes the passed state the current one.  Preserves the previous state
// by returning a save buffer.
//
// NOTE:  The current implementation can cause a memory leak under the
// following scenario:
//    1) plbuf_save() is called
//    2) plbuf_switch() is called
//    3) Commands are called which cause the plot buffer to grow
//    4) plbuf_swtich() is called
//
void * plbuf_switch( PLStream *pls, void *state )
{
    struct _state *new_state = (struct _state *) state;
    struct _state *prev_state;
    size_t        save_size;

    dbug_enter( "plbuf_switch" );

    // No saved state was passed, return a NULL--we hope the caller
    // is smart enough to notice
    //
    if ( state == NULL )
        return NULL;

    if ( !new_state->valid )
    {
        plwarn( "plbuf: Attempting to switch to an invalid saved state" );
        return NULL;
    }

    save_size = sizeof ( struct _state );

    if ( ( prev_state = (struct _state *) malloc( save_size ) ) == NULL )
    {
        plwarn( "plbuf: Unable to allocate memory to save state" );
        return NULL;
    }

    // Set some housekeeping variables
    prev_state->size  = save_size;
    prev_state->valid = 1;

    // Preserve the existing state
    prev_state->plbuf_buffer      = pls->plbuf_buffer;
    prev_state->plbuf_buffer_size = pls->plbuf_buffer_size;
    prev_state->plbuf_top         = pls->plbuf_top;
    prev_state->plbuf_readpos     = pls->plbuf_readpos;

    plbuf_restore( pls, new_state );

    return (void *) prev_state;
}
