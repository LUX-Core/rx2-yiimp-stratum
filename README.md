To run the RX2 stratum you need to activate large pages, to do so you'll need to add:

- vm.nr_hugepages=3840 to /etc/sysctl.conf
- reboot