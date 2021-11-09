library(vrpc)

render_histogram <- function(n_bins = 10) {
  x <- faithful$waiting
  bins <- seq(min(x), max(x), length.out = n_bins + 1)
  hist(x, breaks = bins, col = "#75AADB", border = "white",
       xlab = "Waiting time to next eruption (in mins)",
       main = "Histogram of waiting times")
}

start_vrpc_agent(domain = "public.vrpc")
