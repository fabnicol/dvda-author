FROM ubuntu:latest 

RUN sudo apt -yq update
RUN sudo apt -yq upgrade
RUN sudo apt -yq install nasm util-linux curl xz-utils wkhtmltopdf bison flex libdvdread-dev
RUN chmod +x autogen && /bin/bash autogen
RUN ./configure
RUN sudo make PARALLEL=-j2
RUN sudo make install
RUN sudo cp -rf menu local/ 
RUN sudo mv local/ /usr
RUN ldconfig
RUN echo "Build completed."
