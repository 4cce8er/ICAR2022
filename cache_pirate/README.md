# Cache Pirate

Remember:

1) Enable huge pages:

```
sudo sysctl -w vm.nr_hugepages=1024
```

2) Disable prefetching (only for core 0, works on Intel Core Nehalem based microarchitectures, such as i5, i7)

```
sudo wrmsr -p 0 0x1a4 0x000000000000000F
sudo rdmsr 0x1a4 -0 --all # And verify
```

3) Disable hyperthreading globally

```
echo off | sudo tee /sys/devices/system/cpu/smt/control
```