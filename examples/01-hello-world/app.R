library("vrpc")

greet <- function(whom) {
  paste("Hello,", whom)
}

start_vrpc_agent(domain = "public.vrpc")
