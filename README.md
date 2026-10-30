# readjmp
R package to read data from .jmp table files

`readJMP()` is an R function to allow reading data from the `.jmp` file format

Files corresponding to format versions 1, 3, 7 and 8 are currently supported.

Columns with the following data types have been tested:

* Extended-precision floating point: version 1
* Double-precision floating point: versions 3, 7, 8
* Fixed-length string: versions 1, 3, 7, 8
* Variable-length string: version 7
* Labelled numeric (factor): version 8
* Date and time variables are converted to R Datetime, but any formatting is not preserved
* RowState variables are loaded in raw form and not currently converted into meaninful values

Disclaimer:
The details of these formats were mostly determined through the examination of
sample worksheets freely available on the Internet. This is therefore possible
that variants exist that have not been examined and therefore are not correctly
handled. Please take care to check any data after loading it.