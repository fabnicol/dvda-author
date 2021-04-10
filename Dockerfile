FROM ubuntu:latest 

RUN DEBIAN_FRONTEND=noninteractive apt -yq update
RUN DEBIAN_FRONTEND=noninteractive apt -yq upgrade
RUN DEBIAN_FRONTEND=noninteractive apt -yq install nasm util-linux curl xz-utils wkhtmltopdf bison flex libdvdread-dev
RUN chmod +x autogen && /bin/bash autogen
RUN ./configure
RUN make PARALLEL=-j2
RUN make install
RUN cp -rf menu local/ 
RUN mv local/ /usr
RUN ldconfig
RUN echo "Build completed."
