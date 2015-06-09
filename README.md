DistBFS
=======

This is an optimized implementation of master branch of Level-Synchronous Breath First Search[1]. 

The algorithm maintains two queues: FS and NS. FS contains vertices that are active in the current superstep, while NS includes vertices that will be active in the next superstep. Since each rank only has a portion of the entire graph, while you are traversing through the neighbours of vertices in FS, you need to check if it is a remote one. 

Case 1 (It is a remote vertex): you need to send a message to its owner telling him to put the vertex in NS if the vertex haven't been visited. (store all these newly visisted remote vertice id in a buffer and sent to their owner in the communication phase) 

Case 2 (It is a local vertex): insert it into NS if necessary.

If fact, in the implementation, each superstep is split into two phases: computation phase (visting neighbours of FS) and communication phase (notify owners of newly visit remote vertices).


As can be seen, the way you distribute the graph across MPI Ranks has great impact on the performance. In the literature, there are two classes of graph partitioning algorithms: edge-cut based and vertex-cut based. The fromer partitions the graph by assigning vertices across partitions, while the latter partitions the graph by assigning edges across partitions. My implementation of BFS requires the graph to be partitioned using edge-cut based algorithms. There are lots of open source edge-cut based graph partitioners, such as Metis[2] and Zoltan[3], we can use. The major issue of these graph partitioners is that the partitioning process may take a very long time to accomplish, especially for large graphs. To overcome this issue, several streaming graph partitioners[4, 5] have been proposed.    

Dependent Libraries: MPI, zoltan


References:

[1]. "Parallel breadth-first search on distributed memory systems" by Buluç, Aydin, and Kamesh Madduri. International Conference for High Performance Computing, Networking, Storage and Analysis, 2011

[2]. Metis. http://glaros.dtc.umn.edu/gkhome/metis/metis/overview

[3]. Zoltan. http://www.cs.sandia.gov/zoltan/.

[4]. "Streaming graph partitioning for large distributed graphs" by Isabelle Stanton, Gabriel Kliot. KDD, 2012.

[5]. "(Re)partitioning for stream-enabled computation" by Erwan Le Merrer, Yizhong Liang, Gilles Trédan. arXiv:1310.8211, 2013.
