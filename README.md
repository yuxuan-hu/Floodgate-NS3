# Weir NS-3 simulator
This is an NS-3 simulator for Weir. It is based on [the NS3 simulation of HPCC](https://github.com/alibaba-edu/High-Precision-Congestion-Control) and [the NS3 simulation of Floodgate](https://github.com/NASA-NJU/Floodgate-NS3). 

## Quick Start

### Build
`./waf -d optimized configure`

`./waf build`

Please note if gcc version > 5, compilation will fail due to some ns3 code style.  If this what you encounter, please use:

`CC='gcc-5' CXX='g++-5' ./waf configure`

### Run
The direct command to run is:
`./waf --run 'third mix/config-dcqcn.ini'`

### Experiment config

See `mix/README.md` for detailed examples of experiment config. 

## Important Files
The core logic of Weir was written in following files:

`src/point-to-point/model/qbb-net-device.cc/h`

`src/point-to-point/model/rdma-hw.cc/h`

`src/point-to-point/model/rdma-queue-pair.cc/h`

Others are in following files:

`src/point-to-point/model/settings.cc/h`

`scratch/third.cc`

`config-dcqcn.ini`

`distribution/check_message_size.ipynb`

`distribution/AliStorage2019.txt`
