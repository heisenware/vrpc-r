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

get_table <- function(n) {
  head(dataset, n = n)
}

test_sys_sleep <- function(s = 1) {
  Sys.sleep(s)
  return(s)
}

test_foreign_package <- function() {
  spec_category <-
    vegawidget::as_vegaspec(list(
      `$schema` = vegawidget::vega_schema(),
      data = list(values = vegawidget::data_category),
      mark = "bar",
      encoding = list(
        x = list(field = "category", type = "nominal"),
        y = list(field = "number", type = "quantitative")
      )
    ))
  return(spec_category)
}

test_plot <- function() {
  plot(c(1, 2), c(3, 4))
}

vrpc::start_vrpc_agent(
  broker = "mqtt://broker:1883",
  domain = "test",
  agent = "agent1"
)
