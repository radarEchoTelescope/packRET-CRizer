DIR_PATH="$(pwd)/$(dirname "${BASH_SOURCE[0]}")"

export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${DIR_PATH}/build
export PATH=${PATH}:${DIR_PATH}/build
