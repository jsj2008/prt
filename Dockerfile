FROM fedora:22
MAINTAINER Pavel Odvody <podvody@redhat.com>
ENV PACKAGES 'autoconf automake git make which clang libtool assimp-devel\
 glm-devel SDL2 SDL2-devel mesa-libGL mesa-libGLES mesa-libGL-devel\
 mesa-libGLES-devel gcc-c++'
RUN dnf install -y ${PACKAGES}\
 && dnf clean all

RUN (git clone https://github.com/shaded-enmity/prt.git\
 && cd prt/\
 && autoreconf -fi\
 && ./configure --enable-examples\
 && make all)

CMD ["bash"]
