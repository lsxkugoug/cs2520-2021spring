test commands

cd ~/cs2520-2021spring/p4/spines-src-5.5/daemon

cd ~/cs2520-2021spring/p4/spines-src-5.5/testprogs

### 登陆sender(atl) receiver(lax)

```bash
sender: shl199@pcvm2-29.instageni.rnoc.gatech.edu

receiver:  shl199@pcvm5-3.instageni.idre.ucla.edu      ip:164.67.126.54
```



### 1.Time-Constrained Flooding 

```bash
cd ~/cs2520-2021spring/p4/spines-src-5.5/testprogs

On sender run: ./sp_bflooder -p 8100 -s -v -P 2 -D 3 -k 0 -a 164.67.126.54 -n 200000 -R 2000

On receiver run: ./sp_bflooder -p 8100 -v -P 2 -D 3 -k 0 -n 200000
```



flooding

On receiver run: ./availability_traffic -v -P 2 -D 3 -k 0 -n 200000
On sender run: ./availability_traffic   -s -v -P 2 -D 3 -k 0 -a 164.67.126.54 -n 200000





single path

On receiver run: ./availability_traffic -v -P 2 -D 3 -k 1 -n 200000
On sender run: ./availability_traffic   -s -v -P 2 -D 3 -k 1 -a 164.67.126.54 -n 200000



static two path 只开5 10 1 ，  5 3 1 

On receiver run: ./availability_traffic -v -P 2 -D 3 -k 2 -n 200000
On sender run: ./availability_traffic   -s -v -P 2 -D 3 -k 2 -a 164.67.126.54 -n 200000



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



