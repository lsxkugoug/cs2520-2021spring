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



