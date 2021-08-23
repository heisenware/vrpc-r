FROM r-base:4.1.1
RUN apt-get -y update && apt-get -y install libfontconfig1-dev
RUN Rscript -e 'install.packages(c("Rcpp", "BH", "jsonlite", "svglite", "evaluate", "unix"))'
COPY vrpc /packages/vrpc
WORKDIR /packages
RUN R CMD INSTALL --preclean vrpc
