FROM ubuntu:latest 

RUN DEBIAN_FRONTEND=noninteractive apt -yq update
RUN DEBIAN_FRONTEND=noninteractive apt -yq upgrade
RUN DEBIAN_FRONTEND=noninteractive apt -yq install nasm util-linux curl xz-utils wkhtmltopdf bison flex libdvdread-dev
RUN DEBIAN_FRONTEND=noninteractive apt -yq install git autoconf automake build-essential libtool libjpeg6b imagemagick 
RUN git clone --depth=1 https://github.com/fabnicol/dvda-author.git
WORKDIR dvda-author
RUN chmod +x autogen && /bin/bash autogen
RUN ./configure
RUN make PARALLEL=-j2
RUN make install
RUN cp -rf menu local/ 
RUN mv local/ /usr
RUN ldconfig
RUN echo "Build completed."
