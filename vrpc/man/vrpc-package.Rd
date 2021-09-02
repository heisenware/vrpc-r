\name{vrpc-r-agent}
\alias{vrpc}
\alias{start_vrpc_agent}
\docType{package}
\title{
  VRPC-R-Agent - Asynchronous RPC via MQTT for R
}
\description{
  The VRPC R agent provides remote access to the R language.
  Using a VRPC client (see \href{https://vrpc.io}{vrpc.io}), existing R code can
  be called using any other (VRPC-supported) programming language and from any
  remote location - e.g. directly from the browser within a React web app.
}
\details{
  This section should provide a more detailed overview of how to use the
  package, including the most important functions.
}
\author{
  Dr. Burkhard C. Heisen

  Maintainer: Burkhard Heisen <burkhard.heisen@heisenware.com>
}
\keyword{ package }
\seealso{
  \href{https://vrpc.io}{vrpc.io}
  \href{https://heisenware.com}{heisenware.com}
}
\examples{
  \dontrun{
     # Start a VRPC agent that will provide remote access to code written above
     start_vrpc_agent()
  }
}
