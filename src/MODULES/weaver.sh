#!/bin/bash

module purge
module load devpack/20210226/openmpi/4.0.5/gcc/7.2.0/cuda/10.2.2
##module load devpack/20190814/openmpi/4.0.1/gcc/7.2.0/cuda/10.1.105
#module load openblas
module load yamlcpp/0.5.3/gcc/7.2.0
module swap cmake cmake/3.18.0
#module load ninja
module load anaconda/3/202105 
#module load python/3.7.3

export PAH=/ascldap/users/kyukim/.local/bin:$PATH
