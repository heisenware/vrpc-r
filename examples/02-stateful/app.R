library(vrpc)

dataset <- NULL

select_dataset <- function(name) {
  dataset <<- switch(name,
    "rock" = rock,
    "pressure" = pressure,
    "cars" = cars
  )
  return(TRUE)
}

get_summary <- function() {
  summary(dataset)
}

get_table <- function(obs) {
  head(dataset, n = obs)
}

start_vrpc_agent(domain = "public.vrpc")
