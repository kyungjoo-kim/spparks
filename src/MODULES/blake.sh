#!/bin/bash

module purge
#module load devpack/20171203/openmpi/2.1.2/gcc/7.2.0
#module load devpack/latest/openmpi/2.1.2/intel/18.1.163 
module load devpack/20190329/openmpi/4.0.1/intel/19.3.199
module swap intel/compilers/19.3.199 intel/compilers/20.2.254
module swap openmpi/4.0.1/intel/19.3.199 openmpi/4.0.5/intel/20.2.254 
module swap cmake cmake/3.19.3
#module load boost/1.65.1/intel/18.1.163
#module load netcdf-exo/4.4.1.1/openmpi/2.1.2/intel/18.1.163
#module load yamlcpp/0.5.3

#module swap cmake cmake/3.12.3
