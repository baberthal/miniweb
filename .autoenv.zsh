echo 'Using Custom LLVM'
export original_path=( $path[@] )
export LLVM_PATH=/usr/local/opt/llvm/bin
autostash PATH
path=( $LLVM_PATH $path[@] )
