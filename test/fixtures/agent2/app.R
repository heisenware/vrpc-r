library("vrpc")

dataset <- NULL

select_dataset <- function(name) {
  dataset <<- switch(name,
    "rock" = rock,
    "pressure" = pressure,
    "cars" = cars
  )
}

get_table <- function(obs) {
  head(dataset, n = obs)
}


vrpc::start_vrpc_agent(
  agent = "glossy-1"
)
