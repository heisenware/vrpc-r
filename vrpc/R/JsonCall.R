error_object <- NULL

try_print_plot <- function(object, gfx_file) {
  out <- try(print(object), silent = TRUE)
  if (inherits(out, "try-error") &&
    !grepl("invalid graphics state", attr(out, "condition")$message)) {
    stop(sprintf("Failed to print plot: %s", attr(out, "condition")$message))
  }
  if (!file.exists(gfx_file)) {
    stop("This call did not generate any plot.")
  }
}

extract_png <- function(object) {
  gfx_file <- tempfile()
  png(
    file = gfx_file, width = 800, height = 600, pointsize = 12, type = "cairo"
  )
  try_print_plot(object, gfx_file)
  dev.off()
  buffer <- base64enc::base64encode(gfx_file)
  file.remove(gfx_file)
  return(buffer)
}

extract_svg <- function(object) {
  gfx_file <- tempfile()
  svglite::svglite(
    file = gfx_file, width = 10, height = 8, pointsize = 12
  )
  try_print_plot(object, gfx_file)
  dev.off()
  buffer <- base64enc::base64encode(gfx_file)
  file.remove(gfx_file)
  return(buffer)
}

extract_graphics <- function(evaluation, args) {
  index <- vapply(evaluation, inherits, logical(1), "recordedplot")
  plots <- evaluation[index]
  if (length(plots) < 1) {
    return(NULL)
  }
  last_plot <- plots[[length(plots)]]
  return(extract_svg(last_plot))
}

handler <- evaluate::new_output_handler(value = function(myval, visible = TRUE) {
  if (is.null(error_object)) {
    assign(".val", myval, globalenv())
  }
  if (isTRUE(visible)) {
    # note: print can be really, really slow
    if (identical(class(myval), "list")) {
      cat("List of length ", length(myval), "\n")
      cat(paste("[", names(myval), "]", sep = "", collapse = "\n"))
    } else {
      getFromNamespace("render", "evaluate")(myval)
    }
  }
  invisible()
}, error = function(e) {
  error_object <<- e
})

json_call <- function(function_name, json_string_args = "{}") {
  args <- jsonlite::fromJSON(json_string_args, simplifyVector = FALSE)
  prepared_call <- as.call(
    c(list(as.name(function_name)), args)
  )
  res <- evaluate::evaluate(
    prepared_call,
    envir = globalenv(), output_handler = handler
  )
  if (!is.null(error_object)) {
    return(paste0("__err__", toString(error_object)))
  }
  if (is.null(.val)) {
    return(jsonlite::toJSON(extract_graphics(res), auto_unbox = TRUE))
  }
  out <- try(jsonlite::toJSON(.val, auto_unbox = TRUE), silent = TRUE)
  if (inherits(out, "try-error")) {
    return(jsonlite::toJSON(extract_graphics(res), auto_unbox = TRUE))
  }
  return(jsonlite::toJSON(.val, auto_unbox = TRUE))
}
