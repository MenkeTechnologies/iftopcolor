#!/usr/bin/env sh

pane=iftop:1.0

make && {
    tmux send-keys -t $pane C-c
    sleep 1

    tmux send-keys -t $pane "sudo ./src/iftop -i en0 -c $HOME/.iftop.conf" C-m
    sleep 2
    make clean

}

