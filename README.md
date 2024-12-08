# FastDDSChat
A command line implementation of a chatting app using FastDDS.

This is just the executable for the program, separate from the source files.
Pull the repo or download the executable and keep it in the same folder as the ip_list.txt.

ip_list.txt is for configuring FastDDS for multiple ips to connect devices. Before running the program, manually put in whatever ips the program should expect to connect to.

To run this, two dependencies are needed:
- [Chocolatey](https://chocolatey.org/)
- OpenSSL, which can be installed through Chocolatey using the following command:

```console
foo@bar:~$ choco install openssl
```

Source files for building this app can be found [here.](https://github.com/TheCheeseMeister/FastDDS-Chat)
