(** {1 PLplot - A library for creating scientific plots} *)

(** {e Note:} The API for the {!Plot} and {!Quick_plot} modules is not yet
    fixed and may change.

    {!core} is {b not} expected to change. *)

(** {3 A higher-level OCaml interface to PLplot} *)
module Plot :
  sig

    (** {b THIS API IS NOT FIXED AND MAY CHANGE} *)

    (** The different sides of a (2D) plot.  These values are used to
        represent, e.g., offsets along axes and text placement. *)
    type 'a plot_side_t =
      | Top of 'a
      | Bottom of 'a
      | Left of 'a
      | Right of 'a

    (** These are the available options for drawing plot axes.  These values
        map to individual letters provided to {!Plplot.plbox} and similar
        functions. *)
    type axis_options_t =
      | Axis
      | Frame0
      | Frame1
      | Time
      | Fixed_point
      | Major_grid
      | Minor_grid
      | Invert_ticks
      | Log
      | Unconventional_label
      | Label
      | Custom_label
      | Minor_ticks
      | Major_ticks
      | Vertical_label

    (** A plot stream. *)
    type stream_t = {
      stream : int; (** The plot stream index as used by PLplot *)
      x_axis : axis_options_t list; (** Axis drawing options for the X-axis *)
      y_axis : axis_options_t list; (** Axis drawing options for the Y-axis *)
    }

    (** Colors from the indexed color map (color map 0), from the
        [cmapo_alternate.pal] file!  Note that these will NOT match up with
        colors from the default color palette! *)
    type color_t =
      | White
      | Red
      | Yellow
      | Green
      | Grey
      | Blue
      | Light_blue
      | Purple
      | Pink
      | Black
      | Brown
      | Index_color of int

    (** Different map outlines available for {!map} *)
    type map_t =
      | Globe_outline
      | USA_outline
      | Countries_outline
      | All_outline

    (** Type signature for coordinate transform functions *)
    type pltr_t = float -> float -> float * float

    (** A plottable element, usable by the function {!plot}. *)
    type plot_t

    (** The different available plot device families.  This is not a complete
        representation of all of the available devices provided by PLplot. *)
    type plot_device_family_t =
      | Cairo
      | Qt
      | Core
      | Wx

    (** Plot devices.  The constructor argument allows one to define which
        device family is used. *)
    type plot_device_t =
      | Pdf of plot_device_family_t
      | Png of plot_device_family_t
      | Ps of plot_device_family_t
      | Svg of plot_device_family_t
      | Window of plot_device_family_t
      | External of int (** Pre-defined plot streams *)
      | Other_device of string (** PLplot plotting devices otherwise not
                                   available from this list *)

    (** Type of scaling to use for plot axes, as in {!Plplot.plenv}. *)
    type plot_scaling_t = Preset | Greedy | Equal | Equal_square

    (** PLplot has two color palettes - indexed (color map 0) and
        continuous (color map 1). *)
    type color_palette_t =
      | Indexed of string
      | Continuous of (string * bool)

    (** The default list of axis rendering options, used for all plots generated
        with {!init} if no custom options are provided. *)
    val default_axis_options : axis_options_t list

    (** Convert {!color_t} values to [r, g, b] integer values, each ranging in
        value from [0 - 255].*)
    val rgb_of_color : color_t -> int * int * int

    (** A recommended/standard file name extension, given a particular plot
        device. *)
    val recommended_extension : plot_device_t -> string

    (** Make a new {!stream_t}, possibly using an existing plot stream index. *)
    val make_stream : ?stream:int -> unit -> stream_t

    (** [with_stream ?stream f] performs [f] with [stream] as the active plot
        stream.  If [stream] is not provided then the currently active plot
        stream is left active.  Once [f] is complete, the previously active plot
        stream is restored. *)
    val with_stream : ?stream:stream_t -> (unit -> 'a) -> 'a

    (** [set_color ?stream color] sets the current active plot color to
        [color]. *)
    val set_color : ?stream:stream_t -> color_t -> unit

    (** [set_color_scale ?stream reverse colors] sets the continuous color map
        (color map 1) to a scale determined by interpolating between [colors].
        {!Plplot.plscmap1l} is used internally to set the color scale. *)
    val set_color_scale : ?stream:stream_t -> bool -> color_t array -> unit

    (** [start_page ?stream x0 y0 x1 y1 scaling] starts a new plot page with the
        given dimensions and scaling. *)
    val start_page :
      ?stream:stream_t ->
      float -> float -> float -> float -> plot_scaling_t -> unit

    (** [load_palette ?stream palette] loads either indexed or continuous
        color palette information from a file on disk. *)
    val load_palette : ?stream:stream_t -> color_palette_t -> unit

    (** [init ?filename ?size ?pages x0 x1 y0 y1 scale device] - Start a new
        plot stream for a 2D plot with plot axis extents given by [(x0, y0)] to
        [(x1, y1)].
        @param filename Plot output filename.  A suitable file extension
                        will be added if it does not already exist.
        @param size Dimension of the plot in physical units (e.g., pixels
                    for bitmap outputs (PNG, X) and 1/72 inch for vector
                    outputs (PS, PDF, SVG)
        @param pages Layout of multiple plots on a single page
        @param pre This function, if provided, is called just before the plot
                   instance is initialized.  It can be used to, for example,
                   load custom color palettes. *)
    val init :
      ?filename:string ->
      ?size:int * int ->
      ?pages:int * int ->
      ?pre:(unit -> unit) ->
      float ->
      float -> float -> float -> plot_scaling_t -> plot_device_t -> stream_t

    (** [make_stream_active stream] makes [stream] in to the currently active
        plot stream. *)
    val make_stream_active : stream_t -> unit

    (** {4 Plot elements} *)

    (** [arc ?fill color x y a b angle0 angle1] *)
    val arc :
      ?fill:bool ->
      color_t -> float -> float -> float -> float -> float -> float -> plot_t

    (** [circle ?fill color x y r] *)
    val circle : ?fill:bool -> color_t -> float -> float -> float -> plot_t

    (** [contours color tranform_func contours data] *)
    val contours :
      color_t -> pltr_t -> float array -> float array array -> plot_t

    (** [image ?range (x0, y0) (x1, y1) data] *)
    val image :
      ?range:float * float ->
      float * float -> float * float -> float array array -> plot_t

    (** [imagefr ?range ~scale (x0, y0) (x1, y1) data] *)
    val imagefr :
      ?range:float * float ->
      scale:float * float ->
      float * float -> float * float -> float array array -> plot_t

    (** [join color x0 y1 x1 y1] *)
    val join : color_t -> float -> float -> float -> float -> plot_t

    (** [lines ?label color xs ys] *)
    val lines :
      ?label:string -> color_t -> float array -> float array -> plot_t

    (** [map ?sw ?ne color outline_type] *)
    val map :
      ?sw:float * float -> ?ne:float * float -> color_t -> map_t -> plot_t

    (** [points ?label ?scale color xs ys symbol] *)
    val points :
      ?label:string ->
      ?scale:float -> color_t -> float array -> float array -> int -> plot_t

    (** [polygon ?fill color xs ys] *)
    val polygon :
      ?fill:bool -> color_t -> float array -> float array -> plot_t

    (** [text ?dx ?dy ?just ?color string x y] writes the text [string] inside
        the plot window, at an optional angle defined by the offsets [dx] and
        [dy]. *)
    val text :
      ?dx:float ->
      ?dy:float ->
      ?just:float -> ?color:color_t -> string -> float -> float -> plot_t

    (** [text_outside ?just ?perp ?color side offset string] writes text
        outside of the plot window, along [side], displaced from the axis by
        [offset] * character height. *)
    val text_outside :
      ?just:float ->
      ?perp:bool ->
      ?color:color_t -> float plot_side_t -> float -> string -> plot_t

    (** [transform f] Set the coordinate transformation function used by
        {!imagefr} and other functions affected by {!Plplot.plset_pltr}. *)
    val transform : pltr_t -> plot_t

    (** [clear_transform] clears any currently defined plot transform
        function. *)
    val clear_transform : plot_t

    (** [custom f] will call [f ()] when the plot element is used.  This
        function can be used to create customized plot elements for use with
        {!plot}. *)
    val custom : (unit -> unit) -> plot_t

    (** [list l] makes a plot element from a list of plot elements, allowing,
        for example, a plot to be created piece-by-piece before being
        rendered. *)
    val list : plot_t list -> plot_t

    (** Plot a list of plottable elements *)
    val plot : ?stream:stream_t -> plot_t list -> unit

    (** {4 Support functions} *)

    (** Character height in world coordinate units *)
    val character_height : ?stream:stream_t -> unit -> float

    (** Draw a legend, given a list of titles and colors *)
    val draw_legend :
      ?stream:stream_t ->
      ?line_length:'a ->
      ?x:float -> ?y:float -> string list -> color_t list -> unit

    (** [label x_label y_label title] adds axis labels and a title to the given
        plot [stream]. *)
    val label : ?stream:stream_t -> string -> string -> string -> unit

    (** [colorbar ?stream ?label ?log ?pos ?width values] addd a color bar to a
        plot using the current color scale.  This function should be called
        after the rest of the plot is complete. *)
    val colorbar :
      ?stream:stream_t ->
      ?label:string plot_side_t ->
      ?log:bool ->
      ?pos:float plot_side_t -> ?width:float -> float array -> unit

    (** Draw the plot axes on the current plot page *)
    val plot_axes :
      xtick:float ->
      xsub:int ->
      ytick:float ->
      ysub:int ->
      xoptions:axis_options_t list -> yoptions:axis_options_t list -> unit

    (** {4 Finishing up a plot page} *)

    val default_finish : ?stream:stream_t -> float -> float -> unit -> unit
    val finish_page :
      ?stream:stream_t ->
      ?f:(unit -> unit) -> ?post:(unit -> unit) -> float -> float -> unit
    val next_page :
      ?stream:stream_t ->
      ?f:(unit -> unit) ->
      ?post:(unit -> unit) ->
      float -> float -> float -> float -> plot_scaling_t -> unit

    (** [finish ?stream xstep ystep] finishes up the plot [stream], using
        [xstep] and [ystep] for the axis tick. *)
    val finish :
      ?stream:stream_t ->
      ?f:(unit -> unit) -> ?post:(unit -> unit) -> float -> float -> unit
  end

(** {3 A module for quick, "throw-away" plots} *)
module Quick_plot :
  sig

    (** {b THIS API IS NOT FIXED AND MAY CHANGE} *)

    (** [points xs ys] plots the points described by the coordinates [xs]
        and [ys]. *)
    val points :
      ?filename:string ->
      ?device:Plot.plot_device_t ->
      ?labels:string * string * string ->
      ?log:bool * bool -> float array list -> float array list -> unit

    (** [lines xs ys] plots the line segments described by the coordinates
        [xs] and [ys]. *)
    val lines :
      ?filename:string ->
      ?device:Plot.plot_device_t ->
      ?labels:string * string * string ->
      ?names:string list ->
      ?log:bool * bool -> float array list -> float array list -> unit

    (** [image ?log m] plots the image [m] with a matching colorbar.  If [log]
        is true then the data in [m] are assumed to be [log10(x)] values. *)
    val image :
      ?filename:string ->
      ?device:Plot.plot_device_t ->
      ?labels:string * string * string ->
      ?log:bool -> ?palette:string -> float array array -> unit
  end

(** {3:core The standard PLplot API} *)

(** See the main PLplot documentation for the description and documentation of
    these functions. *)

type plplot3d_style_enum =
    PL_DIFFUSE
  | PL_DRAW_LINEX
  | PL_DRAW_LINEY
  | PL_DRAW_LINEXY
  | PL_MAG_COLOR
  | PL_BASE_CONT
  | PL_TOP_CONT
  | PL_SURF_CONT
  | PL_DRAW_SIDES
  | PL_FACETED
  | PL_MESH
and plplot3d_style = plplot3d_style_enum list
and plplot_bin_enum =
    PL_BIN_DEFAULT
  | PL_BIN_CENTRED
  | PL_BIN_NOEXPAND
  | PL_BIN_NOEMPTY
and plplot_bin_style = plplot_bin_enum list
and plplot_hist_enum =
    PL_HIST_DEFAULT
  | PL_HIST_NOSCALING
  | PL_HIST_IGNORE_OUTLIERS
  | PL_HIST_NOEXPAND
  | PL_HIST_NOEMPTY
and plplot_hist_style = plplot_hist_enum list
external pl_setcontlabelformat : int -> int -> unit
  = "camlidl_plplot_core_c_pl_setcontlabelformat"
external pl_setcontlabelparam : float -> float -> float -> int -> unit
  = "camlidl_plplot_core_c_pl_setcontlabelparam"
external pladv : int -> unit = "camlidl_plplot_core_c_pladv"
external plarc :
  float -> float -> float -> float -> float -> float -> bool -> unit
  = "camlidl_plplot_core_c_plarc_bytecode" "camlidl_plplot_core_c_plarc"
external plaxes :
  float -> float -> string -> float -> int -> string -> float -> int -> unit
  = "camlidl_plplot_core_c_plaxes_bytecode" "camlidl_plplot_core_c_plaxes"
external plbin : float array -> float array -> plplot_bin_style -> unit
  = "camlidl_plplot_core_c_plbin"
external plbtime : float -> int * int * int * int * int * float
  = "camlidl_plplot_core_c_plbtime"
external plbop : unit -> unit = "camlidl_plplot_core_c_plbop"
external plbox : string -> float -> int -> string -> float -> int -> unit
  = "camlidl_plplot_core_c_plbox_bytecode" "camlidl_plplot_core_c_plbox"
external plbox3 :
  string ->
  string ->
  float ->
  int ->
  string ->
  string -> float -> int -> string -> string -> float -> int -> unit
  = "camlidl_plplot_core_c_plbox3_bytecode" "camlidl_plplot_core_c_plbox3"
external plcalc_world : float -> float -> float * float * int
  = "camlidl_plplot_core_c_plcalc_world"
external plclear : unit -> unit = "camlidl_plplot_core_c_plclear"
external plcol0 : int -> unit = "camlidl_plplot_core_c_plcol0"
external plcol1 : float -> unit = "camlidl_plplot_core_c_plcol1"
external plconfigtime :
  float ->
  float ->
  float -> int -> bool -> int -> int -> int -> int -> int -> float -> unit
  = "camlidl_plplot_core_c_plconfigtime_bytecode"
  "camlidl_plplot_core_c_plconfigtime"
external plcpstrm : int -> bool -> unit = "camlidl_plplot_core_c_plcpstrm"
external plctime : int -> int -> int -> int -> int -> float -> float
  = "camlidl_plplot_core_c_plctime_bytecode" "camlidl_plplot_core_c_plctime"
external plend : unit -> unit = "camlidl_plplot_core_c_plend"
external plend1 : unit -> unit = "camlidl_plplot_core_c_plend1"
external plenv : float -> float -> float -> float -> int -> int -> unit
  = "camlidl_plplot_core_c_plenv_bytecode" "camlidl_plplot_core_c_plenv"
external plenv0 : float -> float -> float -> float -> int -> int -> unit
  = "camlidl_plplot_core_c_plenv0_bytecode" "camlidl_plplot_core_c_plenv0"
external pleop : unit -> unit = "camlidl_plplot_core_c_pleop"
external plerrx : float array -> float array -> float array -> unit
  = "camlidl_plplot_core_c_plerrx"
external plerry : float array -> float array -> float array -> unit
  = "camlidl_plplot_core_c_plerry"
external plfamadv : unit -> unit = "camlidl_plplot_core_c_plfamadv"
external plfill : float array -> float array -> unit
  = "camlidl_plplot_core_c_plfill"
external plfill3 : float array -> float array -> float array -> unit
  = "camlidl_plplot_core_c_plfill3"
external plflush : unit -> unit = "camlidl_plplot_core_c_plflush"
external plfont : int -> unit = "camlidl_plplot_core_c_plfont"
external plfontld : int -> unit = "camlidl_plplot_core_c_plfontld"
external plgchr : unit -> float * float = "camlidl_plplot_core_c_plgchr"
external plgcol0 : int -> int * int * int = "camlidl_plplot_core_c_plgcol0"
external plgcol0a : int -> int * int * int * float
  = "camlidl_plplot_core_c_plgcol0a"
external plgcolbg : unit -> int * int * int
  = "camlidl_plplot_core_c_plgcolbg"
external plgcolbga : unit -> int * int * int * float
  = "camlidl_plplot_core_c_plgcolbga"
external plgcompression : unit -> int
  = "camlidl_plplot_core_c_plgcompression"
external plgdev : unit -> string = "camlidl_plplot_core_c_plgdev"
external plgdidev : unit -> float * float * float * float
  = "camlidl_plplot_core_c_plgdidev"
external plgdiori : unit -> float = "camlidl_plplot_core_c_plgdiori"
external plgdiplt : unit -> float * float * float * float
  = "camlidl_plplot_core_c_plgdiplt"
external plgfci : unit -> int64 = "camlidl_plplot_core_c_plgfci"
external plgfam : unit -> int * int * int = "camlidl_plplot_core_c_plgfam"
external plgfnam : unit -> string = "camlidl_plplot_core_c_plgfnam"
external plgfont : unit -> int * int * int = "camlidl_plplot_core_c_plgfont"
external plglevel : unit -> int = "camlidl_plplot_core_c_plglevel"
external plgpage : unit -> float * float * int * int * int * int
  = "camlidl_plplot_core_c_plgpage"
external plgra : unit -> unit = "camlidl_plplot_core_c_plgra"
external plgspa : unit -> float * float * float * float
  = "camlidl_plplot_core_c_plgspa"
external plgstrm : unit -> int = "camlidl_plplot_core_c_plgstrm"
external plgver : unit -> string = "camlidl_plplot_core_c_plgver"
external plgvpd : unit -> float * float * float * float
  = "camlidl_plplot_core_c_plgvpd"
external plgvpw : unit -> float * float * float * float
  = "camlidl_plplot_core_c_plgvpw"
external plgxax : unit -> int * int = "camlidl_plplot_core_c_plgxax"
external plgyax : unit -> int * int = "camlidl_plplot_core_c_plgyax"
external plgzax : unit -> int * int = "camlidl_plplot_core_c_plgzax"
external plhist :
  float array -> float -> float -> int -> plplot_hist_style -> unit
  = "camlidl_plplot_core_c_plhist"
external plhlsrgb : float -> float -> float -> float * float * float
  = "camlidl_plplot_core_c_plhlsrgb"
external plinit : unit -> unit = "camlidl_plplot_core_c_plinit"
external pljoin : float -> float -> float -> float -> unit
  = "camlidl_plplot_core_c_pljoin"
external pllab : string -> string -> string -> unit
  = "camlidl_plplot_core_c_pllab"
external pllightsource : float -> float -> float -> unit
  = "camlidl_plplot_core_c_pllightsource"
external plline : float array -> float array -> unit
  = "camlidl_plplot_core_c_plline"
external plline3 : float array -> float array -> float array -> unit
  = "camlidl_plplot_core_c_plline3"
external pllsty : int -> unit = "camlidl_plplot_core_c_pllsty"
external plmesh :
  float array -> float array -> float array array -> plplot3d_style -> unit
  = "camlidl_plplot_core_c_plmesh"
external plmeshc :
  float array ->
  float array -> float array array -> plplot3d_style -> float array -> unit
  = "camlidl_plplot_core_c_plmeshc"
external plmkstrm : unit -> int = "camlidl_plplot_core_c_plmkstrm"
external plmtex : string -> float -> float -> float -> string -> unit
  = "camlidl_plplot_core_c_plmtex"
external plmtex3 : string -> float -> float -> float -> string -> unit
  = "camlidl_plplot_core_c_plmtex3"
external plot3d :
  float array ->
  float array -> float array array -> plplot3d_style -> bool -> unit
  = "camlidl_plplot_core_c_plot3d"
external plot3dc :
  float array ->
  float array -> float array array -> plplot3d_style -> float array -> unit
  = "camlidl_plplot_core_c_plot3dc"
external plpat : int array -> int array -> unit
  = "camlidl_plplot_core_c_plpat"
external plpoin : float array -> float array -> int -> unit
  = "camlidl_plplot_core_c_plpoin"
external plpoin3 : float array -> float array -> float array -> int -> unit
  = "camlidl_plplot_core_c_plpoin3"
external plprec : int -> int -> unit = "camlidl_plplot_core_c_plprec"
external plpsty : int -> unit = "camlidl_plplot_core_c_plpsty"
external plptex : float -> float -> float -> float -> float -> string -> unit
  = "camlidl_plplot_core_c_plptex_bytecode" "camlidl_plplot_core_c_plptex"
external plptex3 :
  float ->
  float ->
  float ->
  float ->
  float -> float -> float -> float -> float -> float -> string -> unit
  = "camlidl_plplot_core_c_plptex3_bytecode" "camlidl_plplot_core_c_plptex3"
external plrandd : unit -> float = "camlidl_plplot_core_c_plrandd"
external plreplot : unit -> unit = "camlidl_plplot_core_c_plreplot"
external plrgbhls : float -> float -> float -> float * float * float
  = "camlidl_plplot_core_c_plrgbhls"
external plschr : float -> float -> unit = "camlidl_plplot_core_c_plschr"
external plscmap0 : int array -> int array -> int array -> unit
  = "camlidl_plplot_core_c_plscmap0"
external plscmap0a :
  int array -> int array -> int array -> float array -> unit
  = "camlidl_plplot_core_c_plscmap0a"
external plscmap0n : int -> unit = "camlidl_plplot_core_c_plscmap0n"
external plscmap1 : int array -> int array -> int array -> unit
  = "camlidl_plplot_core_c_plscmap1"
external plscmap1a :
  int array -> int array -> int array -> float array -> unit
  = "camlidl_plplot_core_c_plscmap1a"
external plscmap1l :
  bool ->
  float array ->
  float array -> float array -> float array -> bool array option -> unit
  = "camlidl_plplot_core_c_plscmap1l_bytecode"
  "camlidl_plplot_core_c_plscmap1l"
external plscmap1la :
  bool ->
  float array ->
  float array ->
  float array -> float array -> float array -> bool array option -> unit
  = "camlidl_plplot_core_c_plscmap1la_bytecode"
  "camlidl_plplot_core_c_plscmap1la"
external plscmap1n : int -> unit = "camlidl_plplot_core_c_plscmap1n"
external plscol0 : int -> int -> int -> int -> unit
  = "camlidl_plplot_core_c_plscol0"
external plscol0a : int -> int -> int -> int -> float -> unit
  = "camlidl_plplot_core_c_plscol0a"
external plscolbg : int -> int -> int -> unit
  = "camlidl_plplot_core_c_plscolbg"
external plscolbga : int -> int -> int -> float -> unit
  = "camlidl_plplot_core_c_plscolbga"
external plscolor : int -> unit = "camlidl_plplot_core_c_plscolor"
external plscompression : int -> unit
  = "camlidl_plplot_core_c_plscompression"
external plsdev : string -> unit = "camlidl_plplot_core_c_plsdev"
external plsdidev : float -> float -> float -> float -> unit
  = "camlidl_plplot_core_c_plsdidev"
external plsdimap : int -> int -> int -> int -> float -> float -> unit
  = "camlidl_plplot_core_c_plsdimap_bytecode"
  "camlidl_plplot_core_c_plsdimap"
external plsdiori : float -> unit = "camlidl_plplot_core_c_plsdiori"
external plsdiplt : float -> float -> float -> float -> unit
  = "camlidl_plplot_core_c_plsdiplt"
external plsdiplz : float -> float -> float -> float -> unit
  = "camlidl_plplot_core_c_plsdiplz"
external plseed : int64 -> unit = "camlidl_plplot_core_c_plseed"
external plsesc : char -> unit = "camlidl_plplot_core_c_plsesc"
external plsfam : int -> int -> int -> unit = "camlidl_plplot_core_c_plsfam"
external plsfci : int64 -> unit = "camlidl_plplot_core_c_plsfci"
external plsfnam : string -> unit = "camlidl_plplot_core_c_plsfnam"
external plsfont : int -> int -> int -> unit
  = "camlidl_plplot_core_c_plsfont"
external plsmaj : float -> float -> unit = "camlidl_plplot_core_c_plsmaj"
external plsmin : float -> float -> unit = "camlidl_plplot_core_c_plsmin"
external plsori : int -> unit = "camlidl_plplot_core_c_plsori"
external plspage : float -> float -> int -> int -> int -> int -> unit
  = "camlidl_plplot_core_c_plspage_bytecode" "camlidl_plplot_core_c_plspage"
external plspal0 : string -> unit = "camlidl_plplot_core_c_plspal0"
external plspal1 : string -> bool -> unit = "camlidl_plplot_core_c_plspal1"
external plspause : bool -> unit = "camlidl_plplot_core_c_plspause"
external plsstrm : int -> unit = "camlidl_plplot_core_c_plsstrm"
external plssub : int -> int -> unit = "camlidl_plplot_core_c_plssub"
external plssym : float -> float -> unit = "camlidl_plplot_core_c_plssym"
external plstar : int -> int -> unit = "camlidl_plplot_core_c_plstar"
external plstart : string -> int -> int -> unit
  = "camlidl_plplot_core_c_plstart"
external plstripa : int -> int -> float -> float -> unit
  = "camlidl_plplot_core_c_plstripa"
external plstripd : int -> unit = "camlidl_plplot_core_c_plstripd"
external plimage :
  float array array ->
  float ->
  float ->
  float ->
  float -> float -> float -> float -> float -> float -> float -> unit
  = "camlidl_plplot_core_c_plimage_bytecode" "camlidl_plplot_core_c_plimage"
external plstyl : int array -> int array -> unit
  = "camlidl_plplot_core_c_plstyl"
external plsurf3d :
  float array ->
  float array -> float array array -> plplot3d_style -> float array -> unit
  = "camlidl_plplot_core_c_plsurf3d"
external plsvect : float array -> float array -> bool -> unit
  = "camlidl_plplot_core_c_plsvect"
external plsvpa : float -> float -> float -> float -> unit
  = "camlidl_plplot_core_c_plsvpa"
external plsxax : int -> int -> unit = "camlidl_plplot_core_c_plsxax"
external plsxwin : int -> unit = "camlidl_plplot_core_plsxwin"
external plsyax : int -> int -> unit = "camlidl_plplot_core_c_plsyax"
external plsym : float array -> float array -> int -> unit
  = "camlidl_plplot_core_c_plsym"
external plszax : int -> int -> unit = "camlidl_plplot_core_c_plszax"
external pltext : unit -> unit = "camlidl_plplot_core_c_pltext"
external pltimefmt : string -> unit = "camlidl_plplot_core_c_pltimefmt"
external plvasp : float -> unit = "camlidl_plplot_core_c_plvasp"
external plvpas : float -> float -> float -> float -> float -> unit
  = "camlidl_plplot_core_c_plvpas"
external plvpor : float -> float -> float -> float -> unit
  = "camlidl_plplot_core_c_plvpor"
external plvsta : unit -> unit = "camlidl_plplot_core_c_plvsta"
external plw3d :
  float ->
  float ->
  float ->
  float ->
  float -> float -> float -> float -> float -> float -> float -> unit
  = "camlidl_plplot_core_c_plw3d_bytecode" "camlidl_plplot_core_c_plw3d"
external plwid : int -> unit = "camlidl_plplot_core_c_plwid"
external plwind : float -> float -> float -> float -> unit
  = "camlidl_plplot_core_c_plwind"
external plxormod : bool -> bool = "camlidl_plplot_core_c_plxormod"
external plsetopt : string -> string -> int
  = "camlidl_plplot_core_c_plsetopt"
external plMinMax2dGrid : float array array -> float * float
  = "camlidl_plplot_core_plMinMax2dGrid"
external plcont :
  float array array -> int -> int -> int -> int -> float array -> unit
  = "camlidl_plplot_core_ml_plcont_bytecode" "camlidl_plplot_core_ml_plcont"
external plshade :
  float array array ->
  float ->
  float ->
  float ->
  float ->
  float ->
  float -> int -> float -> int -> int -> int -> int -> int -> int -> unit
  = "camlidl_plplot_core_ml_plshade_bytecode"
  "camlidl_plplot_core_ml_plshade"
external plshades :
  float array array ->
  float ->
  float -> float -> float -> float array -> int -> int -> int -> int -> unit
  = "camlidl_plplot_core_ml_plshades_bytecode"
  "camlidl_plplot_core_ml_plshades"
external plimagefr :
  float array array ->
  float ->
  float -> float -> float -> float -> float -> float -> float -> unit
  = "camlidl_plplot_core_ml_plimagefr_bytecode"
  "camlidl_plplot_core_ml_plimagefr"
external plvect : float array array -> float array array -> float -> unit
  = "camlidl_plplot_core_ml_plvect"
external plmap : string -> float -> float -> float -> float -> unit
  = "camlidl_plplot_core_ml_plmap"
external plmeridians :
  float -> float -> float -> float -> float -> float -> unit
  = "camlidl_plplot_core_ml_plmeridians_bytecode"
  "camlidl_plplot_core_ml_plmeridians"
external plpoly3 :
  float array -> float array -> float array -> int array -> int -> unit
  = "camlidl_plplot_core_ml_plpoly3"
external pltr0 : float -> float -> float * float
  = "camlidl_plplot_core_ml_pltr0"
external plg_current_col0 : unit -> int
  = "camlidl_plplot_core_plg_current_col0"
external plg_current_col1 : unit -> float
  = "camlidl_plplot_core_plg_current_col1"
external plgwid : unit -> int = "camlidl_plplot_core_plgwid"
external plgchrht : unit -> float = "camlidl_plplot_core_plgchrht"
external plstripc :
  string ->
  string ->
  float ->
  float ->
  float ->
  float ->
  float ->
  float ->
  float ->
  bool ->
  bool ->
  int ->
  int ->
  int array -> int array -> string array -> string -> string -> string -> int
  = "ml_plstripc_byte" "ml_plstripc"
external pltr1 :
  float -> float -> float array -> float array -> float * float = "ml_pltr1"
external pltr2 :
  float -> float -> float array array -> float array array -> float * float
  = "ml_pltr2"
val plset_pltr : (float -> float -> float * float) -> unit
val plunset_pltr : unit -> unit
val plset_mapform : (float -> float -> float * float) -> unit
val plunset_mapform : unit -> unit
val plset_defined : (float -> float -> int) -> unit
val plunset_defined : unit -> unit
type plplot_grid_method_type =
    PL_GRID_CSA
  | PL_GRID_DTLI
  | PL_GRID_NNI
  | PL_GRID_NNIDW
  | PL_GRID_NNLI
  | PL_GRID_NNAIDW
type plplot_parse_method_type =
    PL_PARSE_PARTIAL
  | PL_PARSE_FULL
  | PL_PARSE_QUIET
  | PL_PARSE_NODELETE
  | PL_PARSE_SHOWALL
  | PL_PARSE_OVERRIDE
  | PL_PARSE_NOPROGRAM
  | PL_PARSE_NODASH
  | PL_PARSE_SKIP
type plplot_axis_type =
    PL_X_AXIS
  | PL_Y_AXIS
  | PL_Z_AXIS
val plslabelfunc : (plplot_axis_type -> float -> string) -> unit
val plunset_labelfunc : unit -> unit
external plgriddata :
  float array ->
  float array ->
  float array ->
  float array ->
  float array -> plplot_grid_method_type -> float -> float array array
  = "ml_plgriddata_bytecode" "ml_plgriddata"
external plparseopts : string array -> plplot_parse_method_type list -> int
  = "ml_plparseopts"

(** {3 Non-standard convenience functions (not part of the standard PLplot
    API.} *)

(** [plcalc_device x y] will give the device position, in normalized device
    coordinates, of the world coordinates (x, y). *)
val plcalc_device : float -> float -> float * float

(** [plfullcanvas ()] maximizes the plot viewport and window.  Dimensions are
    set to (0.0, 0.0) to (1.0, 1.0). *)
val plfullcanvas : unit -> unit

(** Draw an unfilled polygon.  The only functional difference between this and
    {!plline} is that this function will close the given polygon, so there is no
    need to duplicate points to have a closed figure. *)
val plpolyline : float array -> float array -> unit
