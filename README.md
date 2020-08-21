# FPMAS

FPMAS is an HPC (High Performance Computing) Multi-Agent simulation platform
designed to run on massively parallel and distributed memory environments,
such as computing clusters.

The main advantage of FPMAS is that it is designed to abstract as much as
possible MPI and parallel features to the final user. Concretely, no
parallel or distributed computing skills are required to run Multi-Agent
simulations on hundreds of processor cores.

Moreover, FPMAS offers powerful features to allow **concurrent write
operations accross distant processors**. This allows users to easily write
general purpose code without struggling with distributed computing issues,
while still allowing an implicit large scale distributed execution.

FPMAS also automatically handles load balancing accross processors to
distribute the user defined Multi-Agent model on the available computing
resources.

The underlying synchronized distributed graph structure used to represent
Multi-Agent models might also be used in applications that are not related
to Multi-Agent Systems to take advantage of other features such as load
balancing or distant write operations.

![Simple model automatic distribution example on 4 cores](docs/img/mior_dist.png)

# Installation
## Dependencies

FPMAS requires the following tools, see the corresponding documentations to
install them :
- [CMake](https://cmake.org/) 3.10 or higher
- [Open MPI](https://www.open-mpi.org/) 3 or 4
- [nlohmann/json C++ library](https://github.com/nlohmann/json) 3.7 or higher
- [zoltan C/C++ library](https://cs.sandia.gov/Zoltan/) 3.81 or higher

## FPMAS

To use the latest FPMAS version, you can directly clone this repository :
```
git clone https://https://github.com/FPMAS/FPMAS
```

FPMAS can then be built and installed using CMake :
```
mkdir FPMAS/build
cd FPMAS/build
cmake ..
cmake --build .
sudo cmake --install .
```

# Get started

Once installed, the `fpmas` header files should be available from any C++
project.
Detailed instructions about how to set up an FPMAS simulation are available on
the corresponding wiki page.

# Contacts

FPMAS is currently under active development in the context of a PhD thesis at
[FEMTO-ST, DISC
Department](https://www.femto-st.fr/en/Research-departments/DISC/Presentation),
Besançon (France).

For any information, please contact :
- Paul Breugnot : paul.breugnot@univ-fcomte.fr
