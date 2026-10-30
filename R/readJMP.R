#' Reads data from the .jmp file format.
#' @param filename path or url of file to open
#' @return an object of class "data.frame". 
#' @export
readJMP <- function(filename) {
  if (isFALSE(file.exists(filename))) {
    # If the file is not local then it may be a URL, attempt to download it if so
    tmpname <- tempfile(fileext=".jmp")
    download.file(filename, tmpname, quiet = TRUE, mode = "wb")
    if (file.exists(tmpname)) {
      # Check that the file was downloaded, and if so point to this
      filename <- tmpname
      on.exit(unlink(tmpname))
    }
  }

  result <- rcpp_readjmp(filename)
  
  invisible(result)
}