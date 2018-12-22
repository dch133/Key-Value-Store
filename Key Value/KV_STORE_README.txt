Key Value Store by Daniel Chernis 260707258

Number of pods: 256
Each pod has KV pairs: 32
Each Key (size 32) contains 256 values (each of size 256)
Total size of KV-Store:

The size per pod and the number of values stored in the pod were chosen 
to easily associate with the allowed sizes for key and value.
Since value is a char* of size 256, allow 256 values
Since key is a char* of size 32, allow 32 key-value pairs (distinct) 

Each key has an array of values (duplicates permitted)
 
When all the space is filled the olderst value is evicted 
with the help if the headIndex int which keeps track of the index of
the oldest value for a given key.
Similarly for pods, there is a headIndex keeping track of the oldest key.

There is a read_counter stored in each pod: allows distict writes to 
different pods at the same time. Also allows readers to read the same
pod.

In essence, when a Writer writes, the individual pod is locked. Allowing Writes to only write to different pods at the same time.