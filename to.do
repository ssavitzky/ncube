			 NCUBE: Things To Do


Text:

  Move shareware banners into PC and Mac versions only.

  Put documentation into a separate man page instead of online.
  Start on LaTeXinfo version.


Matrix Module:

  Split matrix code out into a separate module.

  Make it general:  pass row and column counts as parameters.


X interface:

  Use the X argument-parser for standard args.

  Add color

  Add ability to draw in any given window (-window nnnn)


Figures:

  Add ability to read and write figure files:   
  (-f filename; -o filename)

    Name:	string
    Dimensions: n
    Vertices:	*n [
      1: (x y z w) ...
      ]
    Edges:	*2*n [
      [1 2] ...
      ]

    (Other attributes should be ignored)


File Module:

  The code for self-describing files really ought to be in a module of
  its own.  We're going to need this pretty soon anyway.

  Calls something like:

    GetAttr(f) -> name
    GetType(f) -> dscr
    GetTypedData(f, &expected_type, &dest)
    GetDataAndType(f, &actual_type) -> malloc'd data
    SkipData(f)
    PutAttr(f, &name)
    PutData(f, &dscr, &data)

    Maybe more, making type implicit in the routine name.

  And formats like:

    nnn		 int
    nn.nn	 float
    "text"	 string (C-ish escapes)

    [ ... ]	 list
    *n [ ... ]	 array (explicit count)
    *c*r [ ... ] array (explicit count, 2-dimensional)

    ( ... )	       vector (with implied leading count)
    ((...)(...)...)    matrix (implied leading counts)

    The difference between a list and a vector or matrix is that the
    latter's data structure includes a leading size; the size of a
    list is implicit.

    Lists may include "index: data" elements.

    Item separators (comma, semicolon, period) should be parameters,
    as should things like 1-origin vs. 0-origin indexing.
