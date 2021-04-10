FROM ubuntu:latest 

RUN DEBIAN_FRONTEND=noninteractive apt -yq update
RUN DEBIAN_FRONTEND=noninteractive apt -yq upgrade
RUN DEBIAN_FRONTEND=noninteractive apt -yq install nasm util-linux curl xz-utils wkhtmltopdf bison flex libdvdread-dev libfreetype-dev libxml2-dev
RUN DEBIAN_FRONTEND=noninteractive apt -yq install git autoconf automake build-essential libtool libjpeg62 libjpeg62-dev apt-get install libpng16-16 libpng-dev libpng-tools
 mjpegtools imagemagick 
RUN git clone --depth=1 https://github.com/fabnicol/dvda-author.git
WORKDIR dvda-author
RUN chmod +x autogen && /bin/bash autogen
RUN ./configure
RUN make PARALLEL=-j2
RUN cp -rf menu local/ 
RUN rm -rf /usr/local/* && cp -rf local/* /usr/local && rm -rf local
RUN make install
RUN find . -maxdepth 1 -type d -exec rm -rf {} \;
RUN ldconfig
RUN echo "Build completed."
