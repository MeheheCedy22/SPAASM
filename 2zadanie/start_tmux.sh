#!/bin/bash

# define the working directory
WORK_DIR="/mnt/c/Marek/Documents/Coding/SPAASM/2zadanie"

# ensure that script uses in the correct directory and builds the project
cd "$WORK_DIR"
make

# create a new tmux session named 'shell_test' detached with the specified working directory
tmux new-session -d -s shell_test -c "$WORK_DIR"

# create the basic layout - split into two columns
tmux split-window -h -t shell_test:0 -c "$WORK_DIR"

# now split the left column into 3 panes vertically
tmux split-window -v -t shell_test:0.0 -c "$WORK_DIR"
tmux split-window -v -t shell_test:0.1 -c "$WORK_DIR"

# now split the right column into 3 panes vertically
tmux split-window -v -t shell_test:0.3 -c "$WORK_DIR"
tmux split-window -v -t shell_test:0.4 -c "$WORK_DIR"

# the panes are now arranged in the following order:
# 0 3
# 1 4
# 2 5

# resize panes to make them more compact
# make pane 0
tmux resize-pane -t shell_test:0.0 -y 10

# make pane 1
tmux resize-pane -t shell_test:0.1 -y 10

# this will automatically make pane 2 (bottom left) bigger

# resize the left column to be wider (move the vertical divider to the right)
tmux resize-pane -t shell_test:0.0 -x 55

# make pane 3 and 4 (top and middle right) the same size
tmux resize-pane -t shell_test:0.3 -y 8
tmux resize-pane -t shell_test:0.4 -y 8

# pane 5 (bottom right) will automatically get the remaining space

# send commands to each pane
# pane 1 (top left) - watch processes
tmux send-keys -t shell_test:0.0 'watch -n 1 "ps aux | head -1; ps aux | grep shell | grep -v grep | grep -v watch | grep -v tmux"' C-m

# pane 2 (middle left) - watch network connections
tmux send-keys -t shell_test:0.1 'watch -n 1 "lsof -i | head -1; lsof -i | grep shell"' C-m

# pane 3 (bottom left) - start server
tmux send-keys -t shell_test:0.2 './shell -v -s -l out.log -p 6060 -u /tmp/sock' C-m

# wait for the server to start
sleep 1

# pane 4 (top right) - start client (network socket)
tmux send-keys -t shell_test:0.3 './shell -v -c -p 6060' C-m

# pane 5 (middle right) - start client (unix domain socket)
tmux send-keys -t shell_test:0.4 './shell -v -c -u /tmp/sock' C-m

# pane 6 (bottom right) - watch log file
tmux send-keys -t shell_test:0.5 'watch -n 1 "tail -10 out.log"' C-m

# attach to the session
tmux attach-session -t shell_test