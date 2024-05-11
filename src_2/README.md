You may experience the following error on some **Linux-based** operating systems:

```
symbol lookup error: /snap/core20/current/lib/x86_64-linux-gnu/libpthread.so.0: undefined symbol: __libc_pthread_init, version GLIBC_PRIVATE
```

To solve the issue, **run the following command **below before **running** the c**ode**.

```sh
unset GTK_PATH
```

Click [here](https://askubuntu.com/questions/1462295/ubuntu-22-04-both-eye-of-gnome-and-gimp-failing-with-undefined-symbol-error/1462494) to learn more.
