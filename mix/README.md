# Experiment config

See `config_doc.txt` for detail description of each configuration. 

## Topology

We use the following topology:

`leafspine-10ToR-4Core.topo` corresponding to `floodgate-config-leafspine-10ToR-4Core.cfg`

`fattree-k8.topo` corresponding to `floodgate-config-fattree-k8.cfg`

## Traffic Generation

This is the main traffic pattern that we used in paper.

We run experiments when `FLOW_CDF = 4/5/6/7/12` , corresponding to `Memcached/Google_SearchRPC/Google_AllRPC/Hadoop/AliStorage2019` workloads.

Set `USE_FLOODGATE = 0` to turn off Floodgate.

The configures related to traffic generation are set as following (note: remove the description when copy into config file):

```
FLOW_FROM_FILE 0
FLOW_CDF 4
FLOW_NUM 50000 {#poisson flows}
LOAD 0.8
INCAST_MIX 720 {#incast flows of one time incast}
INCAST_LOAD 0.5
INCAST_CDF 9
```

You can see `config-dcqcn.ini` for detail.
