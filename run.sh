#!/usr/bin/env bash

make && {

tmux send-keys -t main-7:1.3 C-c
tmux send-keys -t main-7:1.3 "sudo ./iftop -i en0" C-m
tmux send-keys -t main-7:1.3 t
tmux send-keys -t main-7:1.3 p
tmux send-keys -t main-7:1.3 L

sleep 2
make clean

}

