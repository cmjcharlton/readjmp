#' Read .jmp format files
#'
#' \pkg{readjmp} is an R command to allow reading data from the .jmp file format
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
#' download.file(
#'   paste0(sampleurl,"/008_global-temperature/IDS_008.jmp"), 
#'   "IDS_008.jmp", 
#'   mode = "wb"
#' )
#' temperature <- readJMP("IDS_008.jmp")
#' summary(temperature)
#' }
#' @section Maintainer:
#' Chris Charlton \email{c.charlton@@bristol.ac.uk}
#'
#' @author Charlton, C.M.J., (2026) Centre for Multilevel Modelling, University of Bristol.
#' 
#'
"_PACKAGE"