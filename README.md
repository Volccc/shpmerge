**shpmerge** - small console utility to merge ESRI Shape files produced by ReefMaster 2.

Command:
*shpmerge name*

**Description**

When we export shapes from RM2 with group selection, it produces files as:
*[name]_[Isobaths, Major Contours, Minor Contours].[shp,shx,dbf,prj]*
adding digit starting from 2 to each next file.
To simplify and speed up import to another program, files must be merged.
This very simple utility do it

**Output**

It merges files, producing set of files with fixed name:
*merged_[Isobaths, Major Contours, Minor Contours].[shp,shx,dbf,prj]*.
