#' Read .jmp format files
#'
#' \pkg{readJMP} is an R command to allow reading data from the .jmp file format
#'
#' Files corresponding to format versions 1, 3, 7 and 8 are currently supported.
#'
#' Columns with the following data types have been tested:
#'
#' \itemize{
#' \item Extended-precision floating point: version 1
#' \item Double-precision floating point: versions 3, 7
#' \item 1-byte integer: versions 3, 7
#' \item 2-byte integer: versions 3, 7
#' \item 4-byte integer: versions 3, 7
#' \item Fixed-length string: versions 1, 3, 7, 8
#' \item Variable-length string: version 7
#' \item Labelled numeric (factor): version 8
#' \item Date and time variables are converted to R Datetime, but any formatting is not preserved
#' \item 2-byte RowState: version 1, 3
#' \item 8-byte RowState: version 7
#' }
#'
#' RowState variables are loaded in raw form and not currently converted into meaningful values
#'
#' Disclaimer:
#' The details of the format were mostly determined through the examination of
#' sample worksheets freely available on the Internet. This is therefore possible
#' that variants exist that have not been examined and therefore are not correctly
#' handled. Please take care to check any data after loading it.
#'
#'
#' @importFrom Rcpp evalCpp
#' @importFrom utils download.file
#' @useDynLib readjmp, .registration = TRUE
#'
#' @examples
#' \donttest{
#' library(readjmp)
#' sampleurl <- "https://www.jmp.com/content/dam/jmp/documents/en/academic/instructional-data-sets"
#' paint <- readJMP(paste0(sampleurl, "/014_paint-wear-accelerated-testing/IDS_014.jmp"))
#' summary(paint)
#' 
#' filename <- tempfile(pattern = "global-temperature", fileext = ".jmp") 
#' download.file(
#'   paste0(sampleurl,"/008_global-temperature/IDS_008.jmp"), 
#'   filename, 
#'   mode = "wb"
#' )
#' temperature <- readJMP(filename)
#' unlink(filename) # delete the downloaded file after reading 
#' summary(temperature)
#' }
#' @section Maintainer:
#' Chris Charlton \email{c.charlton@@bristol.ac.uk}
#'
#' @author Charlton, C.M.J., (2026) Centre for Multilevel Modelling, University of Bristol.
#' 
#'
"_PACKAGE"