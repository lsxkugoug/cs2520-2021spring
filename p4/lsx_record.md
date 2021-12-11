12.07 



====availability ,2hours, 100w packages, k = 1 ====

strange loss rate is 2% high?

```
sp_bflooder Receiver:

- Num Pkts Received:    980931 out of 1000000
- Num Loss:     19069
- Pkt Size:     100
- Throughput:   78.474413 kbs
- Availability: 98.080000%
- Unavailability:       1.920000%
```

```
sp_bflooder Receiver:

- Num Pkts Received:    980304 out of 1000000
- Num Loss:     19696
- Pkt Size:     100
- Throughput:   78.436407 kbs
- Availability: 97.989598%
- Unavailability:       2.010402%

```



==== 20w package, k =1 =====

route = 5 10 1 0 0 0 0 0 

1.

```
sp_bflooder Receiver:

- Num Pkts Received:    199505 out of 200000
- Num Loss:     495
- Pkt Size:     100
- Throughput:   79.801617 kbs

- Availability: 99.750000%
- Unavailability:       0.250000%


```

2.

sp_bflooder Receiver:
- Num Pkts Received:    199505 out of 200000
- Num Loss:     495
- Pkt Size:     100
- Throughput:   79.801617 kbs
- Availability: 99.750000%
- Unavailability:       0.250000%



dynamic k = 2

路线： 5 3 1 0 0 0 0 0      5 10 1 0 0 0 0 0 

1.

sp_bflooder Receiver:
- Num Pkts Received:    198093 out of 200000
- Num Loss:     1907
- Pkt Size:     100
- Throughput:   79.237235 kbs
- Availability: 99.099550%
- Unavailability:       0.900450%



2.

sp_bflooder Receiver:
- Num Pkts Received:    199675 out of 200000
- Num Loss:     325
- Pkt Size:     100
- Throughput:   79.870324 kbs
- Availability: 99.849925%



3.

sp_bflooder Receiver:
- Num Pkts Received:    199659 out of 200000
- Num Loss:     341
- Pkt Size:     100
- Throughput:   79.863658 kbs
- Availability: 99.849925%
- Unavailability:       0.150075%



flooding

sp_bflooder Receiver:
- Num Pkts Received:    199728 out of 200000
- Num Loss:     272
- Pkt Size:     100
- Throughput:   79.891167 kbs
- Availability: 99.850000%
- Unavailability:       0.150000%



sp_bflooder Receiver:
- Num Pkts Received:    199972 out of 200000
- Num Loss:     28
- Pkt Size:     100
- Throughput:   79.988230 kbs
- Availability: 100.000000%
- Unavailability:       0.000000%



sp_bflooder Receiver:
- Num Pkts Received:    198701 out of 200000
- Num Loss:     1299
- Pkt Size:     100
- Throughput:   79.480647 kbs
- Availability: 99.349675%
- Unavailability:       0.650325%





static two lins

1.

sp_bflooder Receiver:
- Num Pkts Received:    190254 out of 200000
- Num Loss:     9746
- Pkt Size:     100
- Throughput:   76.101562 kbs
- Availability: 94.750000%
- Unavailability:       5.250000%
Printing path statistics:
5 3 1 0 0 0 0 0 : 108483
5 10 1 0 0 0 0 0 : 81771

2.

sp_bflooder Receiver:
- Num Pkts Received:    195051 out of 200000
- Num Loss:     4949
- Pkt Size:     100
- Throughput:   78.020482 kbs
- Availability: 97.398699%
- Unavailability:       2.601301%



3.

sp_bflooder Receiver:
- Num Pkts Received:    185920 out of 200000
- Num Loss:     14080
- Pkt Size:     100
- Throughput:   74.367996 kbs
- Availability: 92.600000%
- Unavailability:       7.400000%
Printing path statistics:
5 3 1 0 0 0 0 0 : 126473
5 10 1 0 0 0 0 0 : 59447



**targeted redundancy**

不要连接 7 11 13 14

 -v -P 2 -D 3 -k 6 -n 200000

On receiver run: ./availability_traffic  -v -P 2 -D 3 -k 6 -n 200000
On sender run: ./availability_traffic   -s -v -P 2 -D 3 -k 6 -a 164.67.126.54 -n 200000



sp_bflooder Receiver:

- Num Pkts Received:    200000 out of 200000

- Num Loss:     0

- Pkt Size:     100

- Throughput:   80.000128 kbs

- Availability: 100.000000%

- Unavailability:       0.000000%

  

sp_bflooder Receiver:

- Num Pkts Received:    199986 out of 200000
- Num Loss:     14
- Pkt Size:     100
- Throughput:   79.994523 kbs
- Availability: 100.000000%
- Unavailability:       0.000000%



sp_bflooder Receiver:

- Num Pkts Received:    200000 out of 200000
- Num Loss:     0
- Pkt Size:     100
- Throughput:   80.000141 kbs

- Availability: 100.000000%
- Unavailability:       0.000000%



**Dynamic Two Disjoint Path**

sp_bflooder Receiver:
- Num Pkts Received:    199049 out of 200000
- Num Loss:     951
- Pkt Size:     100
- Throughput:   79.619686 kbs
- Availability: 99.449725%
- Unavailability:       0.550275%



sp_bflooder Receiver:
- Num Pkts Received:    193610 out of 200000
- Num Loss:     6390
- Pkt Size:     100
- Throughput:   77.444063 kbs

- Availability: 96.698349%
- Unavailability:       3.301651%

