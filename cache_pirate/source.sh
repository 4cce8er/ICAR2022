MY_PCM_PATH=/home/acceber/git-repos/pcm     # Change this accordingly

export PCM_PATH=$MY_PCM_PATH
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PCM_PATH}/build/lib

export OMP_NUM_THREADS=2
export GOMP_CPU_AFFINITY="2, 3"