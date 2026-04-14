distrobox assemble create

cp .rosboxrc $HOME
echo "alias rosbox='distrobox enter ros-t --verbose -- bash --rcfile .rosboxrc' " >> $HOME/.bash_aliases
