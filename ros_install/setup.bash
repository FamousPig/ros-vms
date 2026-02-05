distrobox assemble create

cp .rosboxrc $HOME
echo "alias ros='distrobox enter ros-t -- bash --rcfile .rosboxrc' " >> $HOME/.bash_aliases
