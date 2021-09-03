FROM r-base:4.1.1
RUN apt-get -y update && apt-get -y install libfontconfig1-dev
RUN Rscript -e 'install.packages(c("Rcpp", "BH", "jsonlite", "svglite", "evaluate", "base64enc"))'

# for the tests to work (not needed for vrpc functionality)
RUN apt-get -y install libxml2-dev
RUN Rscript -e 'install.packages(c("vegawidget"))'

COPY vrpc /packages/vrpc
WORKDIR /packages
RUN R CMD INSTALL --preclean vrpc
