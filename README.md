# ACA_Project
Project - TSimulating the cache coherance protocol MSI, MESI and MOESI. <br>
Setting the Parameters for Cache Simulators:
1. Set L2_SETS and L2_WAYS in L2.h
2. Set L3_SETS and L3_WAYS in LLC.h
3. Set queue_size and res_queue_size in L2.h
4. Number of Cores can be given while running executable.
<br>
Steps to run the project:<br>
1. To build the project run <I>make</I> command in the terminal. This will generate 3 executables msi, mesi and moesi.<br>
2. Run the required executable like <I>./executable CORE_COUNT TRACE_FILE </I><br>
    for ex: > ./msi 8 trace_file
