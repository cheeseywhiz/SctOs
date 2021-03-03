sudo-loop() {
    sudo true
    sudo-loop-impl &
}

sudo-loop-impl() {
    while sleep 275; do
        sudo true
    done
}
